#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include "settings.h"

struct SunData {
  String sunrise;
  String sunset;
  String solar_noon;
  String day_length;
  String civil_twilight_begin;
  String civil_twilight_end;
  String nautical_twilight_begin;
  String nautical_twilight_end;
  String astronomical_twilight_begin;
  String astronomical_twilight_end;
  String tzid;
  unsigned long lastUpdate;
  bool valid;
};

extern SunData g_sunData;

void initSun();
bool updateSunData();
bool shouldLightBeOn();
void handleSunData();
bool isTimeBetween(String startTimeStr, String endTimeStr);
