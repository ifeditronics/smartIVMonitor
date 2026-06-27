#include "ui.h"
#include "display.h"
#include "sensor.h"
#include "wifi_monitor.h"
#include "ota.h"
#include "config.h"

// Simulated Battery Percentage
static float batteryPct = 100.0f;

// UI Cache to prevent flicker by only redrawing modified values
static int lastDripCount = -1;
static float lastDripRate = -1.0f;
static FlowStatus lastFlowStatus = (FlowStatus)-1;
static int lastBatteryPct = -1;
static bool lastWiFiConnected = false;
static int lastRawValue = -1;
static int lastThreshold = -1;
static String lastIPStr = "";

/**
 * @brief Configures the buzzer pin.
 */
void initBuzzer() {
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW); // Active-high buzzer: LOW is off
}

/**
 * @brief Triggers the active buzzer to sound for a specified duration using a simple delay.
 * @param duration Duration in milliseconds.
 */
void beep(uint16_t duration) {
    digitalWrite(PIN_BUZZER, HIGH); // Turn buzzer on
    delay(duration);                // Blocking delay
    digitalWrite(PIN_BUZZER, LOW);  // Turn buzzer off
}

/**
 * @brief Plays a short chirp on drip detection.
 */
void playDripChirp() {
    beep(BUZZER_DUR_DRIP);
}

/**
 * @brief Plays a longer warning beep for flow stopped alarms.
 */
void playAlarmChirp() {
    beep(BUZZER_DUR_ALARM);
}

/**
 * @brief Draws the premium startup splash screen with animated loading bar.
 */
void drawStartupScreen() {
    tft.fillScreen(COLOR_BLACK);
    
    // Play dual startup chirps using reusable active buzzer beep()
    beep(BUZZER_DUR_STARTUP_1);
    delay(100);
    beep(BUZZER_DUR_STARTUP_2);
    
    // Draw "IMAXEUNO" title (size 2 text)
    tft.setTextSize(2);
    tft.setTextColor(COLOR_CYAN);
    // Center alignment: "IMAXEUNO" (9 chars). Size 2 is 12px width. 9 * 12 = 108px.
    tft.setCursor((DISPLAY_WIDTH - 108) / 2, 30);
    tft.print("IMAXEUNO");
    
    // Draw "Smart IV Drip Monitor" subtitle (size 1 text)
    tft.setTextSize(1);
    tft.setTextColor(COLOR_WHITE);
    // Center alignment: 21 chars. Size 1 is 6px. 21 * 6 = 126px.
    tft.setCursor((DISPLAY_WIDTH - 126) / 2, 55);
    tft.print("Smart IV Drip Monitor");
    
    // Draw loading bar frame
    int barX = 14;
    int barY = 80;
    int barW = 100;
    int barH = 8;
    tft.drawRoundRect(barX, barY, barW, barH, 2, COLOR_DARKGRAY);
    
    // Animate loading bar (growing filled rectangle)
    int maxFillW = barW - 4; // 96 pixels
    for (int w = 0; w <= maxFillW; w++) {
        tft.fillRect(barX + 2, barY + 2, w, barH - 4, COLOR_CYAN);
        delay(15); // Fine during startup screen animation
    }
    
    // Draw version string at bottom
    tft.setTextSize(1);
    tft.setTextColor(COLOR_MEDGRAY);
    String verStr = "Version: ";
    verStr += FIRMWARE_VERSION;
    // 14 chars. 14 * 6 = 84px.
    tft.setCursor((DISPLAY_WIDTH - 84) / 2, 105);
    tft.print(verStr.c_str());
    
    delay(600); // Hold splash screen briefly
    tft.fillScreen(COLOR_BLACK);
}

/**
 * @brief Draws the persistent UI dashboard frames and static texts.
 */
void drawMainUIFrame() {
    tft.fillScreen(COLOR_BLACK);
    
    // Top bar header
    tft.setTextSize(1);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(2, 3);
    tft.print("Smart IV");
    
    // Header divider line
    tft.drawFastHLine(0, 14, DISPLAY_WIDTH, COLOR_DARKGRAY);
    
    // Label "Drops" below the giant drip count
    tft.setTextSize(1);
    tft.setTextColor(COLOR_MEDGRAY);
    tft.setCursor((DISPLAY_WIDTH - 30) / 2, 46); // "Drops" is 5 chars * 6 = 30px
    tft.print("Drops");
    
    // Bottom status divider line
    tft.drawFastHLine(0, 106, DISPLAY_WIDTH, COLOR_DARKGRAY);
}

/**
 * @brief Helper to draw the custom WiFi network status icon.
 */
static void drawWiFiIcon(bool connected, uint16_t color) {
    int x = 68; // Anchor point
    int y = 10;
    
    if (connected) {
        // Clear old slash if WiFi connected
        tft.drawLine(x - 2, y - 9, x + 12, y + 1, COLOR_BLACK);
        
        // Draw WiFi signal bars using primitives
        tft.fillCircle(x + 5, y, 1, color); // Dot
        
        tft.drawPixel(x + 3, y - 3, color); // Medium Arc
        tft.drawPixel(x + 4, y - 4, color);
        tft.drawPixel(x + 5, y - 4, color);
        tft.drawPixel(x + 6, y - 4, color);
        tft.drawPixel(x + 7, y - 3, color);
        
        tft.drawPixel(x + 1, y - 6, color); // Outer Arc
        tft.drawPixel(x + 2, y - 7, color);
        tft.drawPixel(x + 3, y - 7, color);
        tft.drawPixel(x + 4, y - 8, color);
        tft.drawPixel(x + 5, y - 8, color);
        tft.drawPixel(x + 6, y - 8, color);
        tft.drawPixel(x + 7, y - 7, color);
        tft.drawPixel(x + 8, y - 7, color);
        tft.drawPixel(x + 9, y - 6, color);
    } 
    else {
        // Draw inactive base icon in dark gray
        tft.fillCircle(x + 5, y, 1, COLOR_DARKGRAY);
        tft.drawFastHLine(x + 3, y - 4, 5, COLOR_DARKGRAY);
        tft.drawFastHLine(x + 1, y - 7, 9, COLOR_DARKGRAY);
        
        // Draw red slash crossing it out
        tft.drawLine(x - 2, y - 9, x + 12, y + 1, COLOR_RED);
    }
}

/**
 * @brief Helper to draw the custom battery icon outline and filled charge state.
 */
static void drawBatteryIcon(int pct, uint16_t color) {
    int x = 104;
    int y = 3;
    int w = 16;
    int h = 8;
    
    // Battery Body
    tft.drawRoundRect(x, y, w, h, 1, COLOR_WHITE);
    // Battery Tip
    tft.fillRect(x + w, y + 2, 2, h - 4, COLOR_WHITE);
    
    // Calculate filled area (maximum internal width is 12 pixels)
    int fillW = (pct * 12) / 100;
    
    // Clear old charge bar inside
    tft.fillRect(x + 2, y + 2, 12, h - 4, COLOR_BLACK);
    // Draw new charge bar
    if (fillW > 0) {
        tft.fillRect(x + 2, y + 2, fillW, h - 4, color);
    }
}

/**
 * @brief Simulates battery drainage. Decrements charge state over ~3 hours.
 * Called once per minute from the main coordinator.
 */
void updateBatteryCharge() {
    // 3 hours = 180 minutes. Total drain step = 100 / 180 = 0.555% per minute
    batteryPct -= (100.0f / (float)BATTERY_DRAIN_TOTAL_MIN);
    
    // Restart simulation loop when depleted to keep simulation running
    if (batteryPct < 0.0f) {
        batteryPct = 100.0f;
    }
}

/**
 * @brief Redraws the dynamic fields on the screen if they have changed.
 * This function guarantees no screen flickering.
 * @param forceRedraw If true, refreshes all screen components regardless of cache.
 */
void updateUI(bool forceRedraw) {
    int currentBattery = (int)batteryPct;
    bool currentWiFi = isWiFiConnected();
    int currentDripCount = getDripCount();
    float currentDripRate = getDripRateDPM();
    FlowStatus currentFlowStatus = getFlowStatus();
    int currentRaw = getRawValue();
    int currentTh = getDynamicThreshold();
    String currentIP = getWiFiIPStr();
    
    // 1. Update Battery Percentage text & icon
    if (forceRedraw || currentBattery != lastBatteryPct) {
        lastBatteryPct = currentBattery;
        
        // Print battery percentage (size 1)
        tft.setTextSize(1);
        tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
        tft.setCursor(80, 3);
        char batStr[5];
        sprintf(batStr, "%3d%%", currentBattery);
        tft.print(batStr);
        
        // Determine color based on charge level
        uint16_t batColor = COLOR_GREEN;
        if (currentBattery <= 20) {
            batColor = COLOR_RED;
        } else if (currentBattery <= 50) {
            batColor = COLOR_ORANGE;
        }
        
        drawBatteryIcon(currentBattery, batColor);
    }
    
    // 2. Update WiFi Icon
    if (forceRedraw || currentWiFi != lastWiFiConnected) {
        lastWiFiConnected = currentWiFi;
        uint16_t wifiColor = currentWiFi ? COLOR_GREEN : COLOR_DARKGRAY;
        drawWiFiIcon(currentWiFi, wifiColor);
    }
    
    // 3. Update Giant Drip Count (6 digits, size 3 text)
    if (forceRedraw || currentDripCount != lastDripCount) {
        lastDripCount = currentDripCount;
        
        tft.setTextSize(3);
        tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
        tft.setCursor((DISPLAY_WIDTH - 108) / 2, 20); // 6 chars * 18px = 108px wide
        
        char countStr[8];
        sprintf(countStr, "%06d", currentDripCount);
        tft.print(countStr);
    }
    
    // 4. Update Flow Status Pill (size 2 text, with custom borders)
    if (forceRedraw || currentFlowStatus != lastFlowStatus) {
        lastFlowStatus = currentFlowStatus;
        
        int pillX = 10;
        int pillY = 56;
        int pillW = 108;
        int pillH = 20;
        
        uint16_t outlineColor = COLOR_DARKGRAY;
        uint16_t fillColor = COLOR_BLACK;
        uint16_t textColor = COLOR_WHITE;
        const char* statusText = "";
        int textOffset = 0;
        
        switch (currentFlowStatus) {
            case FLOW_READY:
                outlineColor = COLOR_GREEN;
                textColor = COLOR_GREEN;
                statusText = "READY";
                textOffset = 34; // (108 - (5 chars * 12px)) / 2 = 24 + 10px = 34
                break;
            case FLOW_FLOWING:
                outlineColor = COLOR_ORANGE;
                textColor = COLOR_ORANGE;
                statusText = "FLOWING";
                textOffset = 22; // (108 - (7 chars * 12px)) / 2 = 12 + 10px = 22
                break;
            case FLOW_STOPPED:
                outlineColor = COLOR_RED;
                fillColor = COLOR_RED;
                textColor = COLOR_WHITE;
                statusText = "STOPPED";
                textOffset = 22; // (108 - (7 chars * 12px)) / 2 = 12 + 10px = 22
                // Play alarm alert beep when state switches to stopped
                playAlarmChirp();
                break;
        }
        
        // Draw the pill frame and background
        tft.fillRoundRect(pillX, pillY, pillW, pillH, 4, fillColor);
        tft.drawRoundRect(pillX, pillY, pillW, pillH, 4, outlineColor);
        
        // Print the status text inside
        tft.setTextSize(2);
        tft.setTextColor(textColor, fillColor);
        tft.setCursor(pillX + textOffset, pillY + 2);
        tft.print(statusText);
    }
    
    // 5. Update Drip Rate (Drops/min)
    if (forceRedraw || abs(currentDripRate - lastDripRate) > 0.05f) {
        lastDripRate = currentDripRate;
        
        tft.setTextSize(1);
        tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
        tft.setCursor(10, 82);
        
        char rateStr[20];
        sprintf(rateStr, "Rate: %3d dpm   ", (int)currentDripRate);
        tft.print(rateStr);
    }
    
    // 6. Update Analog Reading and Threshold values
    if (forceRedraw || currentRaw != lastRawValue || currentTh != lastThreshold) {
        lastRawValue = currentRaw;
        lastThreshold = currentTh;
        
        tft.setTextSize(1);
        tft.setTextColor(COLOR_MEDGRAY, COLOR_BLACK);
        tft.setCursor(10, 94);
        
        char rawStr[22];
        sprintf(rawStr, "Raw:%4d Th:%4d", currentRaw, currentTh);
        tft.print(rawStr);
    }
    
    // 7. Update bottom connectivity status (IP Address / OTA Ready status)
    if (forceRedraw || currentIP != lastIPStr) {
        lastIPStr = currentIP;
        
        tft.setTextSize(1);
        int textY = 114;
        
        // Clear bottom text line
        tft.fillRect(0, textY, DISPLAY_WIDTH, 12, COLOR_BLACK);
        
        if (currentWiFi) {
            tft.setTextColor(COLOR_GREEN);
            String label = "IP: " + currentIP;
            int textW = label.length() * 6;
            tft.setCursor(max(0, (DISPLAY_WIDTH - textW) / 2), textY);
            tft.print(label);
        } else {
            // Draw standby message "OTA Ready" as requested
            tft.setTextColor(COLOR_CYAN);
            String label = "OTA Ready";
            int textW = label.length() * 6;
            tft.setCursor((DISPLAY_WIDTH - textW) / 2, textY);
            tft.print(label);
        }
    }
}

/**
 * @brief Clears UI draw cache to force an immediate refresh of TFT display components.
 */
void resetUICache() {
    lastDripCount = -1;
    lastDripRate = -1.0f;
    lastFlowStatus = (FlowStatus)-1;
}
