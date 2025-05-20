#include "settings.h"

Settings g_settings;
Relay relay;

bool applySettingsFromJson(const JsonDocument &doc) {
  g_settings.wifi.devname = doc["wifi"]["devname"] | g_settings.wifi.devname;
  g_settings.wifi.ssid = doc["wifi"]["ssid"] | g_settings.wifi.ssid;
  g_settings.wifi.password = doc["wifi"]["password"] | g_settings.wifi.password;
  g_settings.wifi.power = doc["wifi"]["power"] | g_settings.wifi.power;
  g_settings.wifi.phy_mode = doc["wifi"]["phy_mode"] | g_settings.wifi.phy_mode;

  g_settings.ntp.ntp_server = doc["ntp"]["ntp_server"] | g_settings.ntp.ntp_server;
  g_settings.ntp.ntp_timezone = doc["ntp"]["ntp_timezone"] | g_settings.ntp.ntp_timezone;

  g_settings.location.lat = doc["location"]["lat"] | g_settings.location.lat;
  g_settings.location.lng = doc["location"]["lng"] | g_settings.location.lng;

  return true;
}

bool loadSettings() {
  loadRelaySettings();

  LittleFS.begin();
  File file = LittleFS.open("/settings.json", "r");
  if (!file) {
  #ifdef DEBUG_SETTINGS
    Serial.println("Error: /settings.json not found. Halting.");
  #endif
    while(true) { delay(1); }
  }
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error) {
  #ifdef DEBUG_SETTINGS
    Serial.println("Error parsing /settings.json. Halting.");
  #endif
    while(true) { delay(1); }
  }
  return applySettingsFromJson(doc);
}

bool saveSettings() {
  File file = LittleFS.open("/settings.json", "w");
  if (!file) {
  }

  JsonDocument doc;

  doc["wifi"]["devname"] = g_settings.wifi.devname;
  doc["wifi"]["ssid"] = g_settings.wifi.ssid;
  doc["wifi"]["password"] = g_settings.wifi.password;
  doc["wifi"]["power"] = g_settings.wifi.power;
  doc["wifi"]["phy_mode"] = g_settings.wifi.phy_mode;

  doc["ntp"]["ntp_server"] = g_settings.ntp.ntp_server;
  doc["ntp"]["ntp_timezone"] = g_settings.ntp.ntp_timezone;

  doc["location"]["lat"] = g_settings.location.lat;
  doc["location"]["lng"] = g_settings.location.lng;

  if (serializeJson(doc, file) == 0) {
    file.close();
    return false;
  }

  file.close();
  return true;
}

bool parseSettings(const String &json) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
  #ifdef DEBUG_SETTINGS
      Serial.print("Failed to parse settings JSON: ");
      Serial.println(error.c_str());
  #endif
    return false;
  }
  return applySettingsFromJson(doc);
}

bool loadRelaySettings() {
  relay.state = false;
  relay.sunPosition = false;
  relay.sunInversion = false;

  if (!LittleFS.begin()) {
    return false;
  }

  if (!LittleFS.exists("/relay.json")) {
    return saveRelaySettings();
  }

  File file = LittleFS.open("/relay.json", "r");
  if (!file) {
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    return false;
  }

  relay.sunPosition = doc["sunPosition"] | false;
  relay.sunInversion = doc["sunInversion"] | false;

  return true;
}

bool saveRelaySettings() {
  if (!LittleFS.begin()) {
    return false;
  }

  File file = LittleFS.open("/relay.json", "w");
  if (!file) {
    return false;
  }

  JsonDocument doc;
  doc["sunPosition"] = relay.sunPosition;
  doc["sunInversion"] = relay.sunInversion;

  bool success = serializeJson(doc, file) > 0;
  file.close();

  return success;
}
