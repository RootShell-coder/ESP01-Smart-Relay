#pragma once

#include <Arduino.h>

#define BLUE_PIN 1
#define RELAY_PIN 0

void initWiFi();
bool connectToWiFi();
void checkWiFiConnection();
void handleWebServer();
void initWebServerAP();
void handleToggle();
void blueOn();
void blueOff();
void blinkAP();
void blinkWifi();
void setupAccessPoint();
void handleDNS();
void checkReboot();
