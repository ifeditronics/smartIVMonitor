#include "ota.h"
#include "config.h"
#include "ui.h"
#include "wifi_monitor.h"

static bool otaInitialized = false;

/**
 * @brief Configures OTA callbacks and starts the ArduinoOTA service.
 * Should be called once WiFi is connected and has a valid IP.
 */
void initOTA() {
    if (otaInitialized) return;
    
    Serial.println("[OTA] Initializing OTA Service...");
    
    // Set Hostname
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    
    // Configure event callbacks
    ArduinoOTA.onStart([]() {
        drawOTAModeScreen(getAPSSIDStr().c_str(), "", getWiFiIPStr().c_str());
        Serial.println("[OTA] Firmware flashing in progress...");
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] Update Complete");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("[OTA] Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("[OTA] Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("[OTA] Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("[OTA] Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("[OTA] End Failed");
        }
    });
    
    // Start listening
    ArduinoOTA.begin();
    otaInitialized = true;
    
    Serial.println("[OTA] OTA Service Ready. Hostname: DripCounter");
}

/**
 * @brief Handles incoming OTA connection requests.
 * This is non-blocking and should be called in the main loop.
 */
void updateOTA() {
    if (otaInitialized) {
        ArduinoOTA.handle();
    }
}

/**
 * @brief Checks if the OTA service has been successfully initialized.
 * @return True if initialized.
 */
bool isOTAReady() {
    return otaInitialized;
}
