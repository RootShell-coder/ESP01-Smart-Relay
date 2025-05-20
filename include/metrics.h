#pragma once
#include <Arduino.h>

void metricsLoop();
void metrics();

float getCpuUsageNow();
float getCpuUsage1m();
float getCpuUsage5m();

size_t getMemoryUsageNow();
size_t getMemoryUsage1m();
size_t getMemoryUsage5m();

float getWifiRssiNow();
float getWifiRssi1m();
float getWifiRssi5m();

int getWifiLostNow();
int getWifiLost1m();
int getWifiLost5m();

unsigned long getUptimeSeconds();
