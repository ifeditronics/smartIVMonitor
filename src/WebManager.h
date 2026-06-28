#ifndef WEB_MANAGER_H
#define WEB_MANAGER_H

#include <Arduino.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

void initWebManager();
void updateWebManager();
void broadcastWebTelemetry();

#endif // WEB_MANAGER_H
