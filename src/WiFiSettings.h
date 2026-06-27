#ifndef WIFI_SETTINGS_H
#define WIFI_SETTINGS_H

#include <Arduino.h>
#include <Preferences.h>

// Public Interface Functions for NVS WiFi Storage
void initWiFiSettings();
bool hasWiFiCredentials();
String getStoredSSID();
String getStoredPassword();
void setStoredSSID(const String& newSSID);
void setStoredPassword(const String& newPassword);
void saveWiFiCredentials(const String& newSSID, const String& newPassword);
void clearWiFiCredentials();

#endif // WIFI_SETTINGS_H
