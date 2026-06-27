#ifndef UI_H
#define UI_H

#include <Arduino.h>

// Buzzer Alerting Functions
void initBuzzer();
void beep(uint16_t duration);
void playDripChirp();
void playAlarmChirp();

// UI Drawing & State Management Functions
void drawStartupScreen();
void drawMainUIFrame();
void updateUI(bool forceRedraw = false); // Periodic UI updates (non-blocking)
void updateBatteryCharge();              // Simulated battery drainage (runs every 1 minute)
void resetUICache();                     // Clears UI draw cache to force immediate TFT refresh

#endif // UI_H
