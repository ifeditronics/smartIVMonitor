#include "wifi_monitor.h"
#include "wifi_credentials.h"
#include "config.h"

// Connection State Tracking
static bool isConnected = false;
static uint32_t lastCheckTime = 0;
static uint32_t disconnectStartTime = 0;
static const uint32_t CHECK_INTERVAL_MS = 5000;      // Poll status every 5 seconds
static const uint32_t RECONNECT_TIMEOUT_MS = 15000;  // Force restart connection after 15 seconds of offline

/**
 * @brief Initializes WiFi in Station mode and begins connection asynchronously.
 */
void initWiFi() {
    Serial.println("[WiFi] Initializing station mode...");
    
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true); // Tell ESP32 SDK to auto-reconnect at the driver level
    
    // Start connection
    WiFi.begin(ssid, password);
    
    lastCheckTime = millis();
    disconnectStartTime = millis();
    isConnected = false;
    
    Serial.printf("[WiFi] Connecting to SSID: %s\n", ssid);
}

/**
 * @brief Periodically checks WiFi status and triggers reconnection if needed.
 * This is non-blocking and uses millis().
 */
void updateWiFi() {
    uint32_t now = millis();
    
    if (now - lastCheckTime >= CHECK_INTERVAL_MS) {
        lastCheckTime = now;
        wl_status_t status = WiFi.status();
        
        if (status == WL_CONNECTED) {
            if (!isConnected) {
                isConnected = true;
                Serial.print("[WiFi] Connected! IP Address: ");
                Serial.println(WiFi.localIP());
            }
            disconnectStartTime = 0; // Reset disconnect stopwatch
        } 
        else {
            // We are not connected
            if (isConnected) {
                isConnected = false;
                disconnectStartTime = now;
                Serial.println("[WiFi] Lost connection!");
            }
            
            // Watchdog: If we remain disconnected for longer than RECONNECT_TIMEOUT_MS,
            // force a new connection cycle to clear any stuck driver states.
            if (disconnectStartTime != 0 && (now - disconnectStartTime >= RECONNECT_TIMEOUT_MS)) {
                Serial.println("[WiFi] Reconnection timeout exceeded. Restarting WiFi connection...");
                WiFi.disconnect();
                WiFi.begin(ssid, password);
                disconnectStartTime = now; // Reset timeout cycle
            }
        }
    }
}

/**
 * @brief Checks if WiFi is currently active and connected.
 * @return True if connected.
 */
bool isWiFiConnected() {
    return isConnected;
}

/**
 * @brief Gets the current WiFi IP address or connection status as a String.
 * @return A string containing the IP address or status description.
 */
String getWiFiIPStr() {
    if (isConnected) {
        return WiFi.localIP().toString();
    } else {
        wl_status_t status = WiFi.status();
        switch (status) {
            case WL_NO_SHIELD:       return "No Shield";
            case WL_IDLE_STATUS:     return "Idle";
            case WL_NO_SSID_AVAIL:   return "SSID Not Found";
            case WL_SCAN_COMPLETED:  return "Scan Complete";
            case WL_CONNECT_FAILED:  return "Conn Failed";
            case WL_CONNECTION_LOST: return "Conn Lost";
            case WL_DISCONNECTED:    return "Offline";
            default:                 return "Connecting...";
        }
    }
}
