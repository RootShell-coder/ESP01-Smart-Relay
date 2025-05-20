#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include "settings.h"
#include "wifi.h"
#include "relay.h"
#include "ntp.h"
#include "metrics.h"
#include "sun.h"

void setup() {
  ESP.wdtDisable();
  ESP.wdtEnable(WDTO_8S);
  delay(500);
  loadSettings();
  initWiFi();
  initNTP();
  relayInit();
  initSun();
}

void loop() {
  metrics();
  ESP.wdtFeed();
  checkWiFiConnection();
  relaySwitch();
  updateNTP();
  handleSunData();
}
