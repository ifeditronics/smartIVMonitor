#include <Arduino.h>
#include "config.h"
#include "display.h"
#include "sensor.h"
#include "wifi_monitor.h"
#include "ota.h"
#include "ui.h"
#include "BLEManager.h"

// Task Scheduler Timing Variables (all in milliseconds)
static uint32_t lastSensorSampleTime = 0;
static uint32_t lastUIUpdateTime = 0;
static uint32_t lastBatteryUpdateTime = 0;
static uint32_t lastWiFiCheckTime = 0;
static uint32_t lastBLEUpdateTime = 0;

/**
 * @brief System Initialization.
 * Runs once upon microcontroller boot.
 */
void setup() {
    // 1. Initialize serial port for debug telemetry
    Serial.begin(115200);
    Serial.println("\n==================================");
    Serial.println("IMAXEUNO Smart IV Monitor booting...");
    Serial.println("==================================");
    
    // 2. Initialize Hardware Buzzer
    initBuzzer();
        
    // 3. Initialize Display Subsystem
    initDisplay();
    
    // 4. Draw Startup splash screen & play chirp (includes blocking delays as allowed)
    drawStartupScreen();
    
    // 5. Initialize IR Sensor and Peak Detection State
    initSensor();
    
    // 6. Initialize WiFi Station Connection (Loads credentials from NVS)
    initWiFi();
    
    // 7. Initialize Bluetooth Low Energy Stack
    initBLE();
    
    // 8. Paint the persistent UI frame layout
    drawMainUIFrame();
    
    // 8. Force initial redraw of all dynamic values
    updateUI(true);
    
    Serial.println("[System] Initialization complete. Entering main loop.");
}

/**
 * @brief Core Cooperative Loop.
 * Runs continuously, orchestrating operations using non-blocking scheduling.
 */
void loop() {
    uint32_t now = millis();
    
    // Task 1: Sample Sensor and Run Peak Detection (Periodic: 10ms / 100Hz)
    if (now - lastSensorSampleTime >= SENSOR_SAMPLE_INTERVAL_MS) {
        lastSensorSampleTime = now;
        
        // Track drip count before updating to check for new drops
        int prevDripCount = getDripCount();
        
        // Read sensor and process algorithms
        updateSensor();
        
        // If a new drip is registered, print logs (drip count and beep are handled in sensor.cpp)
        if (getDripCount() > prevDripCount) {
            Serial.printf("[Drip] Detected! Total Count: %d, Flow Rate: %.1f DPM\n", 
                          getDripCount(), getDripRateDPM());
        }
    }
    
    // Task 2: Update WiFi Connection and Reconnection Watchdog (Periodic: 100ms)
    // The internal WiFi monitor handles its own 5s timing, but we poll it regularly.
    if (now - lastWiFiCheckTime >= 100) {
        lastWiFiCheckTime = now;
        updateWiFi();
        
        // Asynchronously initialize OTA once WiFi has established a connection
        if (isWiFiConnected() && !isOTAReady()) {
            initOTA();
        }
    }
    
    // Task 3: Handle ArduinoOTA Wireless Requests (Continuous, non-blocking)
    updateOTA();
    
    // Task 4: Refresh UI Dashboard Elements (Periodic: 100ms for responsiveness)
    if (now - lastUIUpdateTime >= 100) {
        lastUIUpdateTime = now;
        updateUI(false); // Flicker-free update
    }
    
    // Task 5: Simulate Battery Drainage (Periodic: 1 minute / 60000ms)
    if (now - lastBatteryUpdateTime >= BATTERY_UPDATE_INTERVAL_MS) {
        lastBatteryUpdateTime = now;
        updateBatteryCharge();
    }
    
    // Task 6: Update BLE Telemetry & Notifications (Periodic: 100ms)
    if (now - lastBLEUpdateTime >= 100) {
        lastBLEUpdateTime = now;
        updateBLE();
    }
    
    // Task 6: Developer Serial Command Parser
    // Allows manual test triggers without physical drops
    if (Serial.available() > 0) {
        char rxChar = Serial.read();
        if (rxChar == 'd' || rxChar == 'D') {
            simulateDrip();
            playDripChirp();
            Serial.printf("[Sim Drip] Manual drip injected! Total: %d, Flow Rate: %.1f DPM\n",
                          getDripCount(), getDripRateDPM());
        }
    }
}
