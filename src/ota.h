#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Public Interface Functions
void initOTA();
void updateOTA(); // Non-blocking, calls ArduinoOTA.handle()
bool isOTAReady();

#endif // OTA_H
