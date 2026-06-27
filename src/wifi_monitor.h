#ifndef WIFI_MONITOR_H
#define WIFI_MONITOR_H

#include <Arduino.h>
#include <WiFi.h>

// Public Interface Functions
void initWiFi();
void updateWiFi(); // Non-blocking check called in the main loop
bool isWiFiConnected();
String getWiFiIPStr();

#endif // WIFI_MONITOR_H
