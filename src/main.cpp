#include <Arduino.h>
#include "config.h"
#include "display.h"
#include "sensor.h"
#include "wifi_monitor.h"
#include "ota.h"
#include "ui.h"
#include "WebManager.h"

// Task Scheduler Timing Variables (all in milliseconds)
static uint32_t lastSensorSampleTime = 0;
static uint32_t lastUIUpdateTime = 0;
static uint32_t lastBatteryUpdateTime = 0;

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
    
    // 4. Draw Startup splash screen & play chirp
    drawStartupScreen();
    
    // 5. Initialize IR Sensor and Peak Detection State
    initSensor();
    
    // 6. Initialize Wi-Fi Connection and Silent ArduinoOTA
    initWiFi();
    initOTA();
    
    // 7. Initialize HTTP REST & WebSocket Telemetry Server
    initWebManager();
    
    // 8. Paint the persistent UI frame layout
    drawMainUIFrame();
    
    // 9. Force initial redraw of all dynamic values
    updateUI(true);
    
    Serial.println("[System] Initialization complete. Booted in Wi-Fi Telemetry Mode.");
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
        
        int prevDripCount = getDripCount();
        updateSensor();
        
        if (getDripCount() > prevDripCount) {
            Serial.printf("[Drip] Detected! Total Count: %d, Flow Rate: %.1f DPM\n", 
                          getDripCount(), getDripRateDPM());
            broadcastWebTelemetry();
        }
    }
    
    // Task 2: Refresh UI Dashboard Elements (Periodic: 100ms for responsiveness)
    if (now - lastUIUpdateTime >= 100) {
        lastUIUpdateTime = now;
        updateUI(false);
    }
    
    // Task 3: Simulate Battery Drainage (Periodic: 1 minute / 60000ms)
    if (now - lastBatteryUpdateTime >= BATTERY_UPDATE_INTERVAL_MS) {
        lastBatteryUpdateTime = now;
        updateBatteryCharge();
    }
    
    // Task 4: Process ArduinoOTA background requests
    updateOTA();
    
    // Task 5: Handle HTTP REST client requests & WebSockets broadcasts
    updateWebManager();
    
    // Task 6: Developer Serial Command Parser
    if (Serial.available() > 0) {
        char rxChar = Serial.read();
        if (rxChar == 'd' || rxChar == 'D') {
            simulateDrip();
            playDripChirp();
            broadcastWebTelemetry();
            Serial.printf("[Sim Drip] Manual drip injected! Total: %d, Flow Rate: %.1f DPM\n",
                          getDripCount(), getDripRateDPM());
        }
    }
}
