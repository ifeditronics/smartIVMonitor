#include "BLEManager.h"
#include "WiFiSettings.h"
#include "wifi_monitor.h"
#include "sensor.h"
#include "config.h"
#include "ui.h"
#include <esp_system.h>

// Server Connection Tracker
static bool deviceConnected = false;
static bool oldDeviceConnected = false;

// Global BLE Objects
static BLEServer* pServer = nullptr;
static BLEService* pService = nullptr;

// Characteristics pointers
static BLECharacteristic* pCharDeviceInfo = nullptr;
static BLECharacteristic* pCharFirmwareVer = nullptr;
static BLECharacteristic* pCharDeviceName = nullptr;
static BLECharacteristic* pCharDripCount = nullptr;
static BLECharacteristic* pCharDropsPerMin = nullptr;
static BLECharacteristic* pCharFlowStatus = nullptr;
static BLECharacteristic* pCharWifiStatus = nullptr;
static BLECharacteristic* pCharIpAddress = nullptr;
static BLECharacteristic* pCharResetCounter = nullptr;
static BLECharacteristic* pCharWifiSSID = nullptr;
static BLECharacteristic* pCharWifiPass = nullptr;
static BLECharacteristic* pCharReconnectWifi = nullptr;
static BLECharacteristic* pCharBatteryLevel = nullptr;

// Cache variables for notifications
static int lastBleDripCount = -1;
static int lastBleDPM = -1;
static FlowStatus lastBleFlowStatus = (FlowStatus)-1;
static String lastBleWifiStatus = "";
static int lastBleBattery = -1;

// Server Callbacks
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("\n==========================================");
        Serial.println("[BLE Event] Mobile Client Connected!");
        Serial.println("==========================================\n");
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("\n==========================================");
        Serial.println("[BLE Event] Mobile Client Disconnected.");
        Serial.println("==========================================\n");
    }
};

// Characteristic Write Callbacks
class CharacteristicCallbacks: public BLECharacteristicCallbacks {
    void processWrite(BLECharacteristic *pCharacteristic, const uint8_t* data, size_t len) {
        String valStr = "";
        if (data != nullptr && len > 0) {
            char* temp = new char[len + 1];
            memcpy(temp, data, len);
            temp[len] = '\0';
            valStr = String(temp);
            delete[] temp;
        } else {
            std::string rxValue = pCharacteristic->getValue();
            valStr = String(rxValue.c_str());
        }
        valStr.trim();
        
        String charUUID = String(pCharacteristic->getUUID().toString().c_str());
        
        Serial.println("\n------------------------------------------");
        Serial.println("[BLE WRITE RECEIVED]");
        Serial.printf(" Characteristic UUID : %s\n", charUUID.c_str());
        Serial.printf(" Raw Payload Length  : %d bytes\n", len);
        Serial.printf(" Payload Content     : '%s'\n", valStr.c_str());

        // 1. Reset Counter Characteristic
        if (pCharacteristic == pCharResetCounter || charUUID.equalsIgnoreCase(CHAR_UUID_RESET_COUNTER)) {
            Serial.println(" Command Matched     : RESET COUNTER");
            if (valStr.equalsIgnoreCase("RESET") || valStr == "1") {
                Serial.println(" Action Executed     : Resetting internal firmware drip counter...");
                
                // Firmware Action: Reset internal counters and UI cache
                resetDripCounter();
                resetUICache();
                
                // Reset notification cache variables to force fresh synchronization
                lastBleDripCount = 0;
                lastBleDPM = 0;
                lastBleFlowStatus = FLOW_READY;
                
                // Update TFT Display immediately
                updateUI(true);
                
                // Update BLE characteristic values & emit immediate notifications
                pCharDripCount->setValue("0");
                if (deviceConnected) pCharDripCount->notify();
                
                pCharDropsPerMin->setValue("0.0");
                if (deviceConnected) pCharDropsPerMin->notify();
                
                pCharFlowStatus->setValue("READY");
                if (deviceConnected) pCharFlowStatus->notify();
                
                Serial.println(" Result Status       : SUCCESS (Counter=0, DPM=0.0, Flow=READY, TFT Updated, BLE Notified)");
            } else {
                Serial.printf(" Result Status       : IGNORED (Unrecognized payload '%s')\n", valStr.c_str());
            }
        }
        // 2. WiFi SSID Characteristic
        else if (pCharacteristic == pCharWifiSSID || charUUID.equalsIgnoreCase(CHAR_UUID_WIFI_SSID)) {
            Serial.println(" Command Matched     : UPDATE WIFI SSID");
            Serial.printf(" Action Executed     : Writing SSID '%s' to NVS Preferences...\n", valStr.c_str());
            setStoredSSID(valStr);
            pCharWifiSSID->setValue(valStr.c_str());
            Serial.println(" Result Status       : SUCCESS (SSID stored in NVS)");
        }
        // 3. WiFi Password Characteristic
        else if (pCharacteristic == pCharWifiPass || charUUID.equalsIgnoreCase(CHAR_UUID_WIFI_PASS)) {
            Serial.println(" Command Matched     : UPDATE WIFI PASSWORD");
            Serial.println(" Action Executed     : Writing WPA2 Password to NVS Preferences...");
            setStoredPassword(valStr);
            Serial.println(" Result Status       : SUCCESS (Password stored in NVS)");
        }
        // 4. Reconnect WiFi Characteristic
        else if (pCharacteristic == pCharReconnectWifi || charUUID.equalsIgnoreCase(CHAR_UUID_RECONNECT_WIFI)) {
            Serial.println(" Command Matched     : RECONNECT WIFI");
            Serial.println(" Action Executed     : Triggering WiFi station reconnection...");
            reconnectWiFi();
            
            String currentStatus = getWiFiIPStr();
            pCharWifiStatus->setValue(currentStatus.c_str());
            if (deviceConnected) pCharWifiStatus->notify();
            Serial.println(" Result Status       : SUCCESS (WiFi reconnection initiated)");
        }
        else {
            Serial.println(" Result Status       : UNKNOWN CHARACTERISTIC");
        }
        Serial.println("------------------------------------------\n");
    }

public:
    void onWrite(BLECharacteristic *pCharacteristic) override {
        processWrite(pCharacteristic, nullptr, 0);
    }

    void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) override {
        if (param != nullptr && param->write.len > 0) {
            processWrite(pCharacteristic, param->write.value, param->write.len);
        } else {
            processWrite(pCharacteristic, nullptr, 0);
        }
    }
};

static CharacteristicCallbacks writeCallbacks;

/**
 * @brief Builds a descriptive string containing detailed hardware & firmware information.
 */
static String buildDeviceInfoString() {
    uint64_t chipid = ESP.getEfuseMac();
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            (uint8_t)(chipid >> 40), (uint8_t)(chipid >> 32),
            (uint8_t)(chipid >> 24), (uint8_t)(chipid >> 16),
            (uint8_t)(chipid >> 8), (uint8_t)chipid);
            
    String info = "Model: ";
    info += ESP.getChipModel();
    info += ", Flash: ";
    info += String(ESP.getFlashChipSize() / (1024 * 1024));
    info += "MB, MAC: ";
    info += macStr;
    info += ", FW: ";
    info += FIRMWARE_VERSION;
    info += ", Build: ";
    info += __DATE__;
    return info;
}

/**
 * @brief Initializes the BLE stack, creates services, characteristics, descriptors, and starts advertising.
 */
void initBLE() {
    Serial.println("[BLE] Initializing Bluetooth Low Energy server...");
    
    // 1. Create BLE Device
    BLEDevice::init("Smart IV Monitor");
    
    // 2. Create Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    // 3. Create Service
    pService = pServer->createService(SERVICE_UUID);
    
    // 4. Create Characteristics
    
    // Device Information (Read)
    pCharDeviceInfo = pService->createCharacteristic(CHAR_UUID_DEVICE_INFO, BLECharacteristic::PROPERTY_READ);
    pCharDeviceInfo->setValue(buildDeviceInfoString().c_str());
    
    // Firmware Version (Read)
    pCharFirmwareVer = pService->createCharacteristic(CHAR_UUID_FIRMWARE_VER, BLECharacteristic::PROPERTY_READ);
    pCharFirmwareVer->setValue(FIRMWARE_VERSION);
    
    // Device Name (Read)
    pCharDeviceName = pService->createCharacteristic(CHAR_UUID_DEVICE_NAME, BLECharacteristic::PROPERTY_READ);
    pCharDeviceName->setValue("Smart IV Monitor");
    
    // Drip Count (Read, Notify)
    pCharDripCount = pService->createCharacteristic(CHAR_UUID_DRIP_COUNT, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pCharDripCount->addDescriptor(new BLE2902());
    pCharDripCount->setValue("0");
    
    // Drops Per Minute (Read, Notify)
    pCharDropsPerMin = pService->createCharacteristic(CHAR_UUID_DROPS_PER_MIN, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pCharDropsPerMin->addDescriptor(new BLE2902());
    pCharDropsPerMin->setValue("0.0");
    
    // Flow Status (Read, Notify)
    pCharFlowStatus = pService->createCharacteristic(CHAR_UUID_FLOW_STATUS, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pCharFlowStatus->addDescriptor(new BLE2902());
    pCharFlowStatus->setValue("READY");
    
    // WiFi Status (Read, Notify)
    pCharWifiStatus = pService->createCharacteristic(CHAR_UUID_WIFI_STATUS, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pCharWifiStatus->addDescriptor(new BLE2902());
    pCharWifiStatus->setValue("Offline");
    
    // IP Address (Read)
    pCharIpAddress = pService->createCharacteristic(CHAR_UUID_IP_ADDRESS, BLECharacteristic::PROPERTY_READ);
    pCharIpAddress->setValue("0.0.0.0");
    
    // Reset Counter (Write, Write NR)
    pCharResetCounter = pService->createCharacteristic(CHAR_UUID_RESET_COUNTER, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
    pCharResetCounter->setCallbacks(&writeCallbacks);
    
    // WiFi SSID (Read, Write, Write NR)
    pCharWifiSSID = pService->createCharacteristic(CHAR_UUID_WIFI_SSID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
    pCharWifiSSID->setCallbacks(&writeCallbacks);
    pCharWifiSSID->setValue(getStoredSSID().c_str());
    
    // WiFi Password (Write, Write NR)
    pCharWifiPass = pService->createCharacteristic(CHAR_UUID_WIFI_PASS, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
    pCharWifiPass->setCallbacks(&writeCallbacks);
    
    // Reconnect WiFi (Write, Write NR)
    pCharReconnectWifi = pService->createCharacteristic(CHAR_UUID_RECONNECT_WIFI, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
    pCharReconnectWifi->setCallbacks(&writeCallbacks);
    
    // Battery Level (Read, Notify)
    pCharBatteryLevel = pService->createCharacteristic(CHAR_UUID_BATTERY_LEVEL, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pCharBatteryLevel->addDescriptor(new BLE2902());
    pCharBatteryLevel->setValue("100");
    
    // 5. Start Service
    pService->start();
    
    // 6. Start Advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    
    Serial.println("[BLE] BLE Service started and advertising as 'Smart IV Monitor'.");
}

/**
 * @brief Handles periodic BLE updates and pushes notifications when live telemetry values change.
 */
void updateBLE() {
    // Handle disconnection restart advertising
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("[BLE] Restarted advertising.");
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    // Always update static/read fields
    if (isWiFiConnected()) {
        pCharIpAddress->setValue(WiFi.localIP().toString().c_str());
    } else {
        pCharIpAddress->setValue("0.0.0.0");
    }
    
    // Read current telemetry
    int currentDripCount = getDripCount();
    float currentDPM = getDripRateDPM();
    FlowStatus currentFlow = getFlowStatus();
    String currentWifiStr = getWiFiIPStr();
    
    // 1. Drip Count Notification
    if (currentDripCount != lastBleDripCount) {
        lastBleDripCount = currentDripCount;
        char buf[12];
        sprintf(buf, "%d", currentDripCount);
        pCharDripCount->setValue(buf);
        if (deviceConnected) pCharDripCount->notify();
    }
    
    // 2. DPM Notification
    int dpmInt = (int)(currentDPM * 10); // Check precision change
    if (dpmInt != lastBleDPM) {
        lastBleDPM = dpmInt;
        char buf[12];
        sprintf(buf, "%.1f", currentDPM);
        pCharDropsPerMin->setValue(buf);
        if (deviceConnected) pCharDropsPerMin->notify();
    }
    
    // 3. Flow Status Notification
    if (currentFlow != lastBleFlowStatus) {
        lastBleFlowStatus = currentFlow;
        const char* statusStr = getFlowStatusStr();
        pCharFlowStatus->setValue(statusStr);
        if (deviceConnected) pCharFlowStatus->notify();
    }
    
    // 4. WiFi Status Notification
    if (currentWifiStr != lastBleWifiStatus) {
        lastBleWifiStatus = currentWifiStr;
        pCharWifiStatus->setValue(currentWifiStr.c_str());
        if (deviceConnected) pCharWifiStatus->notify();
    }
}

/**
 * @brief Checks if a mobile client is currently connected via BLE.
 */
bool isBLEClientConnected() {
    return deviceConnected;
}
