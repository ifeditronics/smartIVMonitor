#include "WiFiSettings.h"
#include "wifi_credentials.h"

static String ssidCache = "";
static String passwordCache = "";
static bool isInitialized = false;

/**
 * @brief Initializes NVS preferences and loads stored credentials into memory.
 */
void initWiFiSettings() {
    Preferences preferences;
    preferences.begin("wifi_config", true); // Open in read-only mode
    
    ssidCache = preferences.getString("ssid", DEFAULT_WIFI_SSID);
    passwordCache = preferences.getString("password", DEFAULT_WIFI_PASS);
    
    preferences.end();
    isInitialized = true;
    
    Serial.printf("[NVS] Loaded WiFi credentials. SSID: '%s'\n", ssidCache.c_str());
}

/**
 * @brief Checks if valid non-empty WiFi credentials exist in NVS.
 * @return True if SSID is stored and non-empty.
 */
bool hasWiFiCredentials() {
    if (!isInitialized) initWiFiSettings();
    return (ssidCache.length() > 0);
}

/**
 * @brief Gets the currently stored WiFi SSID.
 */
String getStoredSSID() {
    if (!isInitialized) initWiFiSettings();
    return ssidCache;
}

/**
 * @brief Gets the currently stored WiFi Password.
 */
String getStoredPassword() {
    if (!isInitialized) initWiFiSettings();
    return passwordCache;
}

/**
 * @brief Saves new SSID to NVS and updates cache.
 */
void setStoredSSID(const String& newSSID) {
    saveWiFiCredentials(newSSID, passwordCache);
}

/**
 * @brief Saves new Password to NVS and updates cache.
 */
void setStoredPassword(const String& newPassword) {
    saveWiFiCredentials(ssidCache, newPassword);
}

/**
 * @brief Saves both SSID and Password to NVS and updates memory cache.
 */
void saveWiFiCredentials(const String& newSSID, const String& newPassword) {
    Preferences preferences;
    preferences.begin("wifi_config", false); // Open in read/write mode
    
    preferences.putString("ssid", newSSID);
    preferences.putString("password", newPassword);
    
    preferences.end();
    
    ssidCache = newSSID;
    passwordCache = newPassword;
    isInitialized = true;
    
    Serial.printf("[NVS] Saved new WiFi credentials. SSID: '%s'\n", ssidCache.c_str());
}

/**
 * @brief Clears stored WiFi credentials from NVS.
 */
void clearWiFiCredentials() {
    Preferences preferences;
    preferences.begin("wifi_config", false);
    preferences.clear();
    preferences.end();
    
    ssidCache = "";
    passwordCache = "";
    Serial.println("[NVS] Cleared WiFi credentials.");
}
