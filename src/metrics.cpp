#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "settings.h"
#include "metrics.h"

static volatile unsigned long idleCounter = 0;
static float usageNow = 0.0f, usage1m = 0.0f, usage5m = 0.0f;
static size_t memoryNowVal = 0, memory1mVal = 0, memory5mVal = 0;
static float usageHistory60[60] = {0.0f}, usageHistory300[300] = {0.0f};
static size_t memHistory60[60] = {0}, memHistory300[300] = {0};
static float rssiNowVal = 0.0f, rssi1mVal = 0.0f, rssi5mVal = 0.0f;
static float rssiHistory60[60] = {0.0f}, rssiHistory300[300] = {0.0f};
static int indexPos = 0;
static bool prevConnected = false;
static int wifiLostNowVal = 0, wifiLost1mVal = 0, wifiLost5mVal = 0;
static int wifiLostHistory60[60] = {0}, wifiLostHistory300[300] = {0};
static unsigned long uptimeSeconds = 0;

void metrics() {
  idleCounter++;
  metricsLoop();
}

static void updateMetrics() {
  unsigned long idleCalls = idleCounter;
  idleCounter = 0;
  float base_idle = 50000.0f;
  usageNow = 1.0f - (idleCalls / base_idle);
  if (usageNow < 0) usageNow = 0;
  if (usageNow > 1) usageNow = 1;

  usageHistory60[indexPos % 60] = usageNow;
  usageHistory300[indexPos % 300] = usageNow;

  float sum1m = 0;
  for (int i = 0; i < 60; i++) {
    sum1m += usageHistory60[i];
  }
  usage1m = sum1m / 60.0f;

  float sum5m = 0;
  for (int i = 0; i < 300; i++) {
    sum5m += usageHistory300[i];
  }
  usage5m = sum5m / 300.0f;

  memoryNowVal = ESP.getFreeHeap();
  memHistory60[indexPos % 60] = memoryNowVal;
  memHistory300[indexPos % 300] = memoryNowVal;

  size_t mem1mSum = 0;
  for (int i = 0; i < 60; i++) {
    mem1mSum += memHistory60[i];
  }
  memory1mVal = mem1mSum / 60;

  size_t mem5mSum = 0;
  for (int i = 0; i < 300; i++) {
    mem5mSum += memHistory300[i];
  }
  memory5mVal = mem5mSum / 300;

  float currentRssi = WiFi.isConnected() ? WiFi.RSSI() : -999.0f;
  rssiNowVal = currentRssi;
  rssiHistory60[indexPos % 60] = currentRssi;
  rssiHistory300[indexPos % 300] = currentRssi;

  float sumRssi1m = 0;
  for (int i = 0; i < 60; i++) {
    sumRssi1m += rssiHistory60[i];
  }
  rssi1mVal = sumRssi1m / 60.0f;

  float sumRssi5m = 0;
  for (int i = 0; i < 300; i++) {
    sumRssi5m += rssiHistory300[i];
  }
  rssi5mVal = sumRssi5m / 300.0f;

  bool connected = (WiFi.status() == WL_CONNECTED);
  int lostEvent = (prevConnected && !connected) ? 1 : 0;
  prevConnected = connected;
  wifiLostNowVal = lostEvent;
  wifiLostHistory60[indexPos % 60] = lostEvent;
  wifiLostHistory300[indexPos % 300] = lostEvent;

  int sumLost1m = 0;
  for (int i = 0; i < 60; i++) {
    sumLost1m += wifiLostHistory60[i];
  }
  wifiLost1mVal = sumLost1m;

  int sumLost5m = 0;
  for (int i = 0; i < 300; i++) {
    sumLost5m += wifiLostHistory300[i];
  }
  wifiLost5mVal = sumLost5m;

  indexPos++;
}

void metricsLoop() {
  static unsigned long lastUpdate = 0;
  unsigned long now = millis();
  if (now - lastUpdate >= 1000) {
    lastUpdate = now;
    updateMetrics();
    uptimeSeconds++;
  }
}

float getCpuUsageNow()      { return usageNow * 100.0f; }
float getCpuUsage1m()       { return usage1m * 100.0f; }
float getCpuUsage5m()       { return usage5m * 100.0f; }

size_t getMemoryUsageNow()  { return memoryNowVal; }
size_t getMemoryUsage1m()   { return memory1mVal; }
size_t getMemoryUsage5m()   { return memory5mVal; }

float getWifiRssiNow()      { return rssiNowVal; }
float getWifiRssi1m()       { return rssi1mVal; }
float getWifiRssi5m()       { return rssi5mVal; }

int getWifiLostNow()        { return wifiLostNowVal; }
int getWifiLost1m()         { return wifiLost1mVal; }
int getWifiLost5m()         { return wifiLost5mVal; }

unsigned long getUptimeSeconds() {
  return uptimeSeconds;
}
