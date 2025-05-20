#include "sun.h"
#include <WiFiClientSecure.h>
#include "relay.h"
#include "ntp.h"
#include <time.h>

SunData g_sunData;

unsigned long lastCheckTime = 0;
const unsigned long CHECK_INTERVAL = 30 * 60 * 1000;

void initSun() {
  g_sunData.valid = false;
  g_sunData.lastUpdate = 0;

  if (g_settings.location.lat != 0 && g_settings.location.lng != 0) {
    updateSunData();
  }
}

bool updateSunData() {
  if (g_settings.location.lat == 0 || g_settings.location.lng == 0) {
    g_sunData.valid = false;
    return false;
  }

  String url = "https://api.sunrise-sunset.org/json?lat=" +
               String(g_settings.location.lat, 6) +
               "&lng=" +
               String(g_settings.location.lng, 6) +
               "&formatted=0";

  WiFiClientSecure client;
  HTTPClient http;

  client.setInsecure();

  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error || doc["status"].as<String>() != "OK") {
    g_sunData.valid = false;
    return false;
  }

  JsonObject results = doc["results"];
  g_sunData.sunrise = results["sunrise"].as<String>();
  g_sunData.sunset = results["sunset"].as<String>();
  g_sunData.solar_noon = results["solar_noon"].as<String>();
  g_sunData.day_length = results["day_length"].as<String>();
  g_sunData.civil_twilight_begin = results["civil_twilight_begin"].as<String>();
  g_sunData.civil_twilight_end = results["civil_twilight_end"].as<String>();
  g_sunData.nautical_twilight_begin = results["nautical_twilight_begin"].as<String>();
  g_sunData.nautical_twilight_end = results["nautical_twilight_end"].as<String>();
  g_sunData.astronomical_twilight_begin = results["astronomical_twilight_begin"].as<String>();
  g_sunData.astronomical_twilight_end = results["astronomical_twilight_end"].as<String>();

  g_sunData.tzid = doc["tzid"].as<String>();
  g_sunData.lastUpdate = millis();
  g_sunData.valid = true;

  return true;
}

time_t isoStringToTimestamp(const String &isoTime) {
  struct tm tm = {};
  char buf[30];

  isoTime.toCharArray(buf, sizeof(buf));

  char *result = strptime(buf, "%Y-%m-%dT%H:%M:%S", &tm);

  if (result == nullptr) {
    result = strptime(buf, "%Y-%m-%d %H:%M:%S", &tm);
    if (result == nullptr) {
      return 0;
    }
  }

  return mktime(&tm);
}

bool isTimeBetween(String startTimeStr, String endTimeStr) {
  time_t now = time(nullptr);

  if (now < 24*60*60) {
    return false;
  }

  time_t startTime = isoStringToTimestamp(startTimeStr);
  time_t endTime = isoStringToTimestamp(endTimeStr);

  if (startTime == 0 || endTime == 0) {
    return false;
  }

  int tzOffset = 0;
  String tzString = g_settings.ntp.ntp_timezone;

  if (tzString.startsWith("+") || tzString.startsWith("-")) {
    int multiplier = tzString.startsWith("+") ? 1 : -1;
    int hours = tzString.substring(1).toInt();
    tzOffset = multiplier * hours * 3600;
  }
  else if (tzString == "Europe/Moscow") {
    tzOffset = 3 * 3600;
  }
  else if (tzString == "Europe/London") {
    tzOffset = 0;
  }

  startTime += tzOffset;
  endTime += tzOffset;

  struct tm startTm, endTm, nowTm;
  gmtime_r(&startTime, &startTm);
  gmtime_r(&endTime, &endTm);
  gmtime_r(&now, &nowTm);

  int startSeconds = startTm.tm_hour * 3600 + startTm.tm_min * 60 + startTm.tm_sec;
  int endSeconds = endTm.tm_hour * 3600 + endTm.tm_min * 60 + endTm.tm_sec;
  int nowSeconds = nowTm.tm_hour * 3600 + nowTm.tm_min * 60 + nowTm.tm_sec;

  if (startSeconds < endSeconds) {
    return (nowSeconds >= startSeconds && nowSeconds <= endSeconds);
  }
  else {
    return (nowSeconds >= startSeconds || nowSeconds <= endSeconds);
  }
}

bool shouldLightBeOn() {
  if (!g_sunData.valid || !relay.sunPosition)
    return false;

  bool isNightTime = isTimeBetween(g_sunData.sunset, g_sunData.sunrise);

  if (relay.sunInversion) {
    return !isNightTime;
  } else {
    return isNightTime;
  }
}

void handleSunData() {
  unsigned long currentTime = millis();

  if (currentTime - lastCheckTime >= CHECK_INTERVAL || lastCheckTime == 0) {
    updateSunData();
    lastCheckTime = currentTime;
  }

  if (relay.sunPosition && g_sunData.valid) {
    bool shouldBeOn = shouldLightBeOn();

    if (shouldBeOn != relay.state) {
      relay.state = shouldBeOn;
    }
  }
}
