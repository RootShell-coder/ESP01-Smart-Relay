#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#ifndef DEBUG_SETTINGS
#define DEBUG_SETTINGS 1
#endif

struct NTPSettings {
  String ntp_server;
  String ntp_timezone;
};

struct WiFiSettings {
  String devname;
  String ssid;
  String password;
  int power;
  String phy_mode;
};

struct LocationSettings {
  float lat;
  float lng;
};

struct Settings{
  NTPSettings ntp;
  WiFiSettings wifi;
  LocationSettings location;
};

struct Relay {
  bool state;
  bool sunPosition;
  bool sunInversion;
};

extern Settings g_settings;
extern Relay relay;

bool loadSettings();
bool saveSettings();
bool parseSettings(const String &json);
bool applySettingsFromJson(const JsonDocument &doc);
bool loadRelaySettings();
bool saveRelaySettings();
void handleToggle();
