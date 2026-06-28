#include "WebManager.h"
#include "sensor.h"
#include "ui.h"
#include "wifi_monitor.h"
#include "config.h"
#include <ArduinoJson.h>

static WebServer server(80);
static WebSocketsServer webSocket(81);

static String buildStatusJson() {
    StaticJsonDocument<256> doc;
    doc["dripCount"] = getDripCount();
    doc["dropsPerMinute"] = getDripRateDPM();
    doc["flowStatus"] = getFlowStatusStr();
    doc["batteryLevel"] = 100;
    doc["firmwareVersion"] = FIRMWARE_VERSION;
    doc["deviceName"] = "Smart IV Monitor";
    doc["uptime"] = millis() / 1000;
    doc["wifiSignal"] = WiFi.RSSI();
    
    String output;
    output.reserve(256);
    serializeJson(doc, output);
    return output;
}

void broadcastWebTelemetry() {
    String json = buildStatusJson();
    webSocket.broadcastTXT(json);
}

static void handleStatus() {
    server.send(200, "application/json", buildStatusJson());
}

static void handleReset() {
    resetDripCounter();
    resetUICache();
    updateUI(true);
    broadcastWebTelemetry();
    server.send(200, "application/json", "{\"success\":true}");
}

static void handleDevice() {
    StaticJsonDocument<256> doc;
    doc["chipModel"] = ESP.getChipModel();
    doc["flashMb"] = ESP.getFlashChipSize() / (1024 * 1024);
    doc["mac"] = WiFi.macAddress();
    doc["firmwareVersion"] = FIRMWARE_VERSION;
    doc["buildDate"] = __DATE__;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["uptime"] = millis() / 1000;
    
    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
}

static void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_CONNECTED) {
        String json = buildStatusJson();
        webSocket.sendTXT(num, json);
    }
}

void initWebManager() {
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/reset", HTTP_POST, handleReset);
    server.on("/api/device", HTTP_GET, handleDevice);
    server.begin();
    
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    
    Serial.println("[WebManager] HTTP REST (port 80) and WebSockets (port 81) services initialized.");
}

static uint32_t lastWebBroadcast = 0;

void updateWebManager() {
    server.handleClient();
    webSocket.loop();
    
    uint32_t now = millis();
    if (now - lastWebBroadcast >= 500) {
        lastWebBroadcast = now;
        broadcastWebTelemetry();
    }
}
