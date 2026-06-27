#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Service UUID
#define SERVICE_UUID                    "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

// Characteristic UUIDs
#define CHAR_UUID_DEVICE_INFO           "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_UUID_FIRMWARE_VER          "13370001-8fcc-459e-8fcc-c5c9c331914b"
#define CHAR_UUID_DEVICE_NAME           "13370002-8fcc-459e-8fcc-c5c9c331914b"
#define CHAR_UUID_DRIP_COUNT            "13370003-8fcc-459e-8fcc-c5c9c331914b"
#define CHAR_UUID_DROPS_PER_MIN         "13370004-8fcc-459e-8fcc-c5c9c331914b"
#define CHAR_UUID_FLOW_STATUS           "13370005-8fcc-459e-8fcc-c5c9c331914b"
#define CHAR_UUID_WIFI_STATUS           "13370006-8fcc-459e-8fcc-c5c9c331914b"
#define CHAR_UUID_IP_ADDRESS            "13370007-8fcc-459e-8fcc-c5c9c331914b"
#define CHAR_UUID_RESET_COUNTER         "13370008-8fcc-459e-8fcc-c5c9c331914b"
#define CHAR_UUID_WIFI_SSID             "13370009-8fcc-459e-8fcc-c5c9c331914b"
#define CHAR_UUID_WIFI_PASS             "1337000a-8fcc-459e-8fcc-c5c9c331914b"
#define CHAR_UUID_RECONNECT_WIFI        "1337000b-8fcc-459e-8fcc-c5c9c331914b"
#define CHAR_UUID_BATTERY_LEVEL         "1337000c-8fcc-459e-8fcc-c5c9c331914b"

// Public Interface Functions
void initBLE();
void updateBLE(); // Periodic check called in the main loop to send notifications on change
bool isBLEClientConnected();

#endif // BLE_MANAGER_H
