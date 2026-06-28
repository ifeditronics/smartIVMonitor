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
static int lastRawValue = -1;

void initBuzzer() {
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);
}

void beep(uint16_t duration) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(duration);
    digitalWrite(PIN_BUZZER, LOW);
}

void playDripChirp() {
    beep(BUZZER_DUR_DRIP);
}

void playAlarmChirp() {
    beep(BUZZER_DUR_ALARM);
}

void drawStartupScreen() {
    tft.fillScreen(COLOR_BLACK);
    
    beep(BUZZER_DUR_STARTUP_1);
    delay(100);
    beep(BUZZER_DUR_STARTUP_2);
    
    tft.setTextSize(2);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor((DISPLAY_WIDTH - 108) / 2, 30);
    tft.print("IMAXEUNO");
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor((DISPLAY_WIDTH - 126) / 2, 55);
    tft.print("Smart IV Drip Monitor");
    
    int barX = 14;
    int barY = 80;
    int barW = 100;
    int barH = 8;
    tft.drawRoundRect(barX, barY, barW, barH, 2, COLOR_DARKGRAY);
    
    int maxFillW = barW - 4;
    for (int w = 0; w <= maxFillW; w++) {
        tft.fillRect(barX + 2, barY + 2, w, barH - 4, COLOR_CYAN);
        delay(15);
    }
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_MEDGRAY);
    String verStr = "Version: ";
    verStr += FIRMWARE_VERSION;
    tft.setCursor((DISPLAY_WIDTH - 84) / 2, 105);
    tft.print(verStr.c_str());
    
    delay(600);
    tft.fillScreen(COLOR_BLACK);
}

void drawMainUIFrame() {
    tft.fillScreen(COLOR_BLACK);
    
    // Top header banner
    tft.setTextSize(1);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(2, 3);
    tft.print("Smart IV");
    
    tft.drawFastHLine(0, 14, DISPLAY_WIDTH, COLOR_DARKGRAY);
    
    // Label "Flow Rate" for nurses
    tft.setTextSize(1);
    tft.setTextColor(COLOR_MEDGRAY);
    tft.setCursor(10, 20);
    tft.print("Flow Rate (DPM):");
    
    tft.drawFastHLine(0, 106, DISPLAY_WIDTH, COLOR_DARKGRAY);
}

static void drawBatteryIcon(int pct, uint16_t color) {
    int x = 104;
    int y = 3;
    int w = 16;
    int h = 8;
    
    tft.drawRoundRect(x, y, w, h, 1, COLOR_WHITE);
    tft.fillRect(x + w, y + 2, 2, h - 4, COLOR_WHITE);
    
    int fillW = (pct * 12) / 100;
    
    tft.fillRect(x + 2, y + 2, 12, h - 4, COLOR_BLACK);
    if (fillW > 0) {
        tft.fillRect(x + 2, y + 2, fillW, h - 4, color);
    }
}

void updateBatteryCharge() {
    batteryPct -= (100.0f / (float)BATTERY_DRAIN_TOTAL_MIN);
    if (batteryPct < 0.0f) {
        batteryPct = 100.0f;
    }
}

void updateUI(bool forceRedraw) {
    int currentBattery = (int)batteryPct;
    int currentDripCount = getDripCount();
    float currentDripRate = getDripRateDPM();
    FlowStatus currentFlowStatus = getFlowStatus();
    int currentRaw = getRawValue();
    
    // 1. Update Battery Icon & percentage
    if (forceRedraw || currentBattery != lastBatteryPct) {
        lastBatteryPct = currentBattery;
        
        tft.setTextSize(1);
        tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
        tft.setCursor(76, 3);
        char batStr[5];
        sprintf(batStr, "%3d%%", currentBattery);
        tft.print(batStr);
        
        uint16_t batColor = COLOR_GREEN;
        if (currentBattery <= 20) {
            batColor = COLOR_RED;
        } else if (currentBattery <= 50) {
            batColor = COLOR_ORANGE;
        }
        drawBatteryIcon(currentBattery, batColor);
    }
    
    // 2. HERO DISPLAY: Flow Rate (DPM) enlarged for easy reading by nurses
    if (forceRedraw || abs(currentDripRate - lastDripRate) > 0.05f) {
        lastDripRate = currentDripRate;
        
        tft.setTextSize(3);
        tft.setTextColor(COLOR_GREEN, COLOR_BLACK);
        tft.setCursor(10, 32);
        
        char rateStr[10];
        sprintf(rateStr, "%3d DPM", (int)currentDripRate);
        tft.print(rateStr);
    }
    
    // 3. Flow Status Pill
    if (forceRedraw || currentFlowStatus != lastFlowStatus) {
        lastFlowStatus = currentFlowStatus;
        
        int pillX = 10;
        int pillY = 62;
        int pillW = 108;
        int pillH = 18;
        
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
                textOffset = 34;
                break;
            case FLOW_FLOWING:
                outlineColor = COLOR_ORANGE;
                textColor = COLOR_ORANGE;
                statusText = "FLOWING";
                textOffset = 22;
                break;
            case FLOW_STOPPED:
                outlineColor = COLOR_RED;
                fillColor = COLOR_RED;
                textColor = COLOR_WHITE;
                statusText = "STOPPED";
                textOffset = 22;
                playAlarmChirp();
                break;
        }
        
        tft.fillRoundRect(pillX, pillY, pillW, pillH, 4, fillColor);
        tft.drawRoundRect(pillX, pillY, pillW, pillH, 4, outlineColor);
        
        tft.setTextSize(2);
        tft.setTextColor(textColor, fillColor);
        tft.setCursor(pillX + textOffset, pillY + 1);
        tft.print(statusText);
    }

    // 4. Smaller Drip Count below status pill
    if (forceRedraw || currentDripCount != lastDripCount) {
        lastDripCount = currentDripCount;
        
        tft.setTextSize(1);
        tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
        tft.setCursor(10, 88);
        
        char countStr[20];
        sprintf(countStr, "Drops: %06d", currentDripCount);
        tft.print(countStr);
    }
    
    // 5. Raw Sensor Reading at bottom line (Threshold removed per request)
    if (forceRedraw || currentRaw != lastRawValue) {
        lastRawValue = currentRaw;
        
        tft.setTextSize(1);
        tft.setTextColor(COLOR_MEDGRAY, COLOR_BLACK);
        tft.setCursor(10, 114);
        
        char rawStr[20];
        sprintf(rawStr, "Raw:%4d          ", currentRaw);
        tft.print(rawStr);
    }
}

void resetUICache() {
    lastDripCount = -1;
    lastDripRate = -1.0f;
    lastFlowStatus = (FlowStatus)-1;
}

void drawOTAModeScreen(const char* ssid, const char* pass, const char* ip) {
    tft.fillScreen(COLOR_BLACK);
    
    tft.fillRect(0, 0, DISPLAY_WIDTH, 24, COLOR_ORANGE);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_BLACK, COLOR_ORANGE);
    tft.setCursor(12, 8);
    tft.print("OTA MODE ENABLED");
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
    tft.setCursor(10, 45);
    tft.print("Firmware Update");
    tft.setCursor(10, 60);
    tft.print("Active...");
    
    tft.fillRect(0, 92, DISPLAY_WIDTH, 36, COLOR_DARKGRAY);
    tft.setTextColor(COLOR_GREEN, COLOR_DARKGRAY);
    tft.setCursor(12, 104);
    tft.print("Uploading via VSCode");
}
