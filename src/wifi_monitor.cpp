#include "wifi_monitor.h"
#include "WiFiSettings.h"
#include "wifi_credentials.h"
#include "config.h"
#include <ESPmDNS.h>

static String targetSSID = "";

/**
 * @brief Initializes WiFi in Station (STA) mode for telemetry and silent OTA.
 */
void initWiFi() {
    Serial.println("[WiFi] Initializing Station (STA) mode...");
    
    initWiFiSettings();
    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(WIFI_POWER_15dBm);
    
    String ssid = getStoredSSID();
    String pass = getStoredPassword();
    
    if (ssid.isEmpty()) {
        ssid = DEFAULT_WIFI_SSID;
        pass = DEFAULT_WIFI_PASS;
    }
    
    targetSSID = ssid;
    Serial.printf("[WiFi] Connecting to router SSID: %s...\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), pass.c_str());
    
    if (MDNS.begin("dripcounter")) {
        Serial.println("[mDNS] Responder started. Hostname: dripcounter.local");
    }
}

void updateWiFi() {
    // Handled asynchronously by WiFi stack
}

bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String getWiFiIPStr() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return "Connecting...";
}

String getAPSSIDStr() {
    return targetSSID.isEmpty() ? DEFAULT_WIFI_SSID : targetSSID;
}

int getConnectedClientCount() {
    return isWiFiConnected() ? 1 : 0;
}
