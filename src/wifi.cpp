#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include "settings.h"
#include "wifi.h"
#include "ntp.h"
#include "metrics.h"

const char *AP_SSID = g_settings.wifi.devname.c_str();
const byte DNS_PORT = 53;
const int WEB_PORT = 80;
const int CONNECT_TIMEOUT = 10000;

ESP8266WebServer server(WEB_PORT);
DNSServer dnsServer;
bool isAPMode = false;

unsigned long rebootTime = 0;
bool rebootScheduled = false;

unsigned long lastBlinkTime = 0;
int blinkCounter = 0;

unsigned long wifiBlinkLastTime = 0;
bool wifiBlinkState = false;

void initWiFi() {
  pinMode(BLUE_PIN, OUTPUT);
  digitalWrite(BLUE_PIN, HIGH);

  LittleFS.begin();
  if(g_settings.wifi.ssid.length() > 0 && g_settings.wifi.password.length() > 0) {
    while(!connectToWiFi()) {
      blinkAP();
    }
  } else {
    setupAccessPoint();
    blinkAP();
  }
  initWebServerAP();
}

void blueOn() {
  digitalWrite(BLUE_PIN, LOW);
}

void blueOff() {
  digitalWrite(BLUE_PIN, HIGH);
}

void blinkAP() {
  if (!isAPMode) return;
  unsigned long currentTime = millis();
  unsigned long interval = (blinkCounter == 4) ? 500 : 100;
  if (currentTime - lastBlinkTime >= interval) {
    lastBlinkTime = currentTime;
    if (blinkCounter == 0 || blinkCounter == 2) {
      blueOn();
    } else {
      blueOff();
    }
    blinkCounter = (blinkCounter + 1) % 5;
  }
}

void blinkWifi() {
  unsigned long currentTime = millis();
  if (wifiBlinkState) {
    if (currentTime - wifiBlinkLastTime >= 100) {
      wifiBlinkState = false;
      blueOff();
      wifiBlinkLastTime = currentTime;
    }
  } else {
    if (currentTime - wifiBlinkLastTime >= 400) {
      wifiBlinkState = true;
      blueOn();
      wifiBlinkLastTime = currentTime;
    }
  }
}

void setupAccessPoint() {
  if (g_settings.wifi.phy_mode == "11n") {
    WiFi.setPhyMode(WIFI_PHY_MODE_11N);
  } else if (g_settings.wifi.phy_mode == "11g") {
    WiFi.setPhyMode(WIFI_PHY_MODE_11G);
  } else if (g_settings.wifi.phy_mode == "11b") {
    WiFi.setPhyMode(WIFI_PHY_MODE_11B);
  }
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setOutputPower(20.5f);
  WiFi.hostname(AP_SSID);
  WiFi.softAP(AP_SSID, nullptr, 11);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  isAPMode = true;
}

bool handleFileNotFound(File &file) {
  if(!file) {
    File notFoundFile = LittleFS.open("/404.html", "r");
    if(!notFoundFile) {
      server.send(404, "text/plain", "File Not Found");
      return false;
    }
    server.setContentLength(notFoundFile.size());
    server.send(404, "text/html", "");
    server.sendContent(notFoundFile);
    notFoundFile.close();
    return false;
  }
  return true;
}

void initWebServerAP() {
  server.on("/", HTTP_GET, []() {
    File f = LittleFS.open("/index.html", "r");
    if (!handleFileNotFound(f)) return;
    server.streamFile(f, "text/html");
    f.close();
  });
  server.on("/index.html", HTTP_GET, []() {
    File f = LittleFS.open("/index.html", "r");
    if (!handleFileNotFound(f)) return;
    server.streamFile(f, "text/html");
    f.close();
  });
  server.on("/style.css", HTTP_GET, []() {
    File f = LittleFS.open("/style.css", "r");
    if (!handleFileNotFound(f)) return;
    server.streamFile(f, "text/css");
    f.close();
  });
  server.on("/settings.js", HTTP_GET, []() {
    File f = LittleFS.open("/settings.js", "r");
    if (!handleFileNotFound(f)) return;
    server.streamFile(f, "application/javascript");
    f.close();
  });
  server.on("/api/getsettings", HTTP_GET, []() {
    File f = LittleFS.open("/settings.json", "r");
    if (!handleFileNotFound(f)) return;
    server.streamFile(f, "application/json");
    f.close();
  });
  server.on("/api/datetime", HTTP_GET, []() {
    JsonDocument doc;
    doc["datetime"] = getFormattedDateTime();
    doc["synced"] = isTimeSynced();
    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
  });
  server.on("/favicon.ico", HTTP_GET, []() {
    File f = LittleFS.open("/favicon.ico", "r");
    server.streamFile(f, "image/x-icon");
    f.close();
  });

  server.on("/api/setsettings", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      String body = server.arg("plain");
      if (!parseSettings(body)) {
        server.send(400, "text/plain", "Failed to parse settings");
        return;
      }
      if (!saveSettings()) {
        server.send(500, "text/plain", "Failed to save settings");
        return;
      }
      server.send(200, "text/plain", "Settings updated. Rebooting...");

      rebootScheduled = true;
      rebootTime = millis() + 1000;
    } else {
      server.send(400, "text/plain", "Invalid request");
    }
  });

  server.on("/metrics", HTTP_GET, []() {
    String metricsOutput;
    metricsOutput += "# HELP cpu_usage_percentage CPU usage percentage\n";
    metricsOutput += "# TYPE cpu_usage_percentage gauge\n";
    metricsOutput += "cpu_usage_now " + String(getCpuUsageNow(), 2) + "\n";
    metricsOutput += "cpu_usage_1m "  + String(getCpuUsage1m(), 2)  + "\n";
    metricsOutput += "cpu_usage_5m "  + String(getCpuUsage5m(), 2)  + "\n\n";

    metricsOutput += "# HELP memory_free_bytes Free heap memory\n";
    metricsOutput += "# TYPE memory_free_bytes gauge\n";
    metricsOutput += "memory_free_now " + String(getMemoryUsageNow()) + "\n";
    metricsOutput += "memory_free_1m "  + String(getMemoryUsage1m())  + "\n";
    metricsOutput += "memory_free_5m "  + String(getMemoryUsage5m())  + "\n\n";

    metricsOutput += "# HELP wifi_rssi_dBm WiFi signal strength in dBm\n";
    metricsOutput += "# TYPE wifi_rssi_dBm gauge\n";
    metricsOutput += "wifi_rssi_now " + String(getWifiRssiNow(), 2) + "\n";
    metricsOutput += "wifi_rssi_1m "  + String(getWifiRssi1m(), 2)  + "\n";
    metricsOutput += "wifi_rssi_5m "  + String(getWifiRssi5m(), 2)  + "\n\n";

    metricsOutput += "# HELP wifi_lost_connection_count Number of WiFi disconnects\n";
    metricsOutput += "# TYPE wifi_lost_connection_count gauge\n";
    metricsOutput += "wifi_lost_now " + String(getWifiLostNow()) + "\n";
    metricsOutput += "wifi_lost_1m " + String(getWifiLost1m()) + "\n";
    metricsOutput += "wifi_lost_5m " + String(getWifiLost5m()) + "\n\n";

    metricsOutput += "# HELP system_uptime_seconds Uptime in seconds\n";
    metricsOutput += "# TYPE system_uptime_seconds gauge\n";
    metricsOutput += "system_uptime_seconds " + String(getUptimeSeconds()) + "\n";

    server.send(200, "text/plain", metricsOutput);
  });

  server.on("/api/gettoggle", HTTP_GET, []() {
    JsonDocument doc;
    doc["state"] = relay.state ? "on" : "off";
    doc["sunPosition"] = relay.sunPosition ? "on" : "off";
    doc["sunInversion"] = relay.sunInversion ? "on" : "off";
    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
  });

  server.on("/api/relaystate", HTTP_GET, []() {
    JsonDocument doc;
    doc["state"] = relay.state ? "on" : "off";
    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
  });

  server.on("/api/toggle", HTTP_POST, handleToggle);

  server.onNotFound([]() {
    if (isAPMode) {
      server.sendHeader("Location", "/index.html", true);
      server.send(302, "text/plain", "");
    } else {
      server.send(404, "text/plain", "Not found");
    }
  });
  server.begin();
}

void handleToggle() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);

    if (!error) {
      if (!doc["state"].isNull()) {
        String stateValue = doc["state"].as<String>();
        relay.state = (stateValue == "on") ? true : false;
      }

      if (!doc["sunPosition"].isNull()) {
        String sunValue = doc["sunPosition"].as<String>();
        relay.sunPosition = (sunValue == "on") ? true : false;
      }

      if (!doc["sunInversion"].isNull()) {
        String invValue = doc["sunInversion"].as<String>();
        relay.sunInversion = (invValue == "on") ? true : false;
      }

      saveRelaySettings();

      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Invalid JSON");
    }
  } else {
    server.send(400, "text/plain", "No data received");
  }
}

bool connectToWiFi() {
  blueOn();

  if (g_settings.wifi.phy_mode == "11n") {
    WiFi.setPhyMode(WIFI_PHY_MODE_11N);
  } else if (g_settings.wifi.phy_mode == "11g") {
    WiFi.setPhyMode(WIFI_PHY_MODE_11G);
  } else if (g_settings.wifi.phy_mode == "11b") {
    WiFi.setPhyMode(WIFI_PHY_MODE_11B);
  }
  if(g_settings.wifi.power > 0) {
    WiFi.setOutputPower(g_settings.wifi.power);
  }
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(AP_SSID);
  WiFi.begin(g_settings.wifi.ssid.c_str(), g_settings.wifi.password.c_str());
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < CONNECT_TIMEOUT) {
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    isAPMode = false;
    return true;
  } else {
    blueOff();
    return false;
  }
}

void checkReboot() {
  if (rebootScheduled && millis() >= rebootTime) {
    ESP.restart();
  }
}

void checkWiFiConnection() {
  checkReboot();

  if (isAPMode) {
    handleDNS();
    handleWebServer();
    blinkAP();
  } else if (WiFi.status() == WL_CONNECTED) {
    handleWebServer();
    blueOn();
  } else {
    blinkWifi();

    static unsigned long lastReconnectAttempt = 0;
    if (millis() - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = millis();
      connectToWiFi();
    }
  }
}

void handleDNS() {
  if (isAPMode) {
    dnsServer.processNextRequest();
  }
}

void handleWebServer() {
  server.handleClient();
}
