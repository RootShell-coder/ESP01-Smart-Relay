#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include <TZ.h>
#include "settings.h"
#include "ntp.h"

static unsigned long lastNtpUpdateTime = 0;
const unsigned long ntpUpdateInterval = 6UL * 60UL * 60UL * 1000UL;
const unsigned long ntpRetryInterval = 60UL * 1000UL;
const time_t MIN_VALID_EPOCH_TIME = 1672531200UL;

int32_t getTimezoneOffset() {
  String tz = g_settings.ntp.ntp_timezone;

  if (tz.startsWith("+") || tz.startsWith("-") || isDigit(tz[0])) {
    int hours = tz.toInt();
    return hours * 3600;
  }

  if (tz == "Europe/Moscow") return 10800;  // МСК UTC+3
  if (tz == "Europe/London") return 0;      // GMT UTC+0

  return 0;
}

void initNTP() {
  if (WiFi.status() == WL_CONNECTED && g_settings.ntp.ntp_server.length() > 0) {
    int32_t tzOffset = getTimezoneOffset();

    configTime(tzOffset, 0, g_settings.ntp.ntp_server.c_str());
    if (tzOffset == 10800) {
      setenv("TZ", "MSK-3", 1);
    } else if (tzOffset == 0) {
      setenv("TZ", "GMT0", 1);
    } else {
      char tzPosix[12];
      sprintf(tzPosix, "UTC%d", -tzOffset/3600);
      setenv("TZ", tzPosix, 1);
    }
    tzset();
    lastNtpUpdateTime = millis();
  }
}

void updateNTP() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  if (g_settings.ntp.ntp_server.length() == 0 || g_settings.ntp.ntp_timezone.length() == 0) {
    return;
  }

  unsigned long currentTime = millis();
  bool needsRegularUpdate = (lastNtpUpdateTime != 0 && (currentTime - lastNtpUpdateTime >= ntpUpdateInterval));
  bool needsRetryUpdate = (!isTimeSynced() && (currentTime - lastNtpUpdateTime >= ntpRetryInterval || lastNtpUpdateTime == 0));

  if (needsRegularUpdate || needsRetryUpdate) {
    int32_t tzOffset = getTimezoneOffset();
    configTime(tzOffset, 0, g_settings.ntp.ntp_server.c_str());

    if (tzOffset == 10800) {
      setenv("TZ", "MSK-3", 1);
    } else if (tzOffset == 0) {
      setenv("TZ", "GMT0", 1);
    } else {
      char tzPosix[12];
      sprintf(tzPosix, "UTC%d", -tzOffset/3600);
      setenv("TZ", tzPosix, 1);
    }
    tzset();
    lastNtpUpdateTime = currentTime;
  }
}

bool isTimeSynced() {
  time_t now = time(nullptr);
  return now > MIN_VALID_EPOCH_TIME;
}

String getFormattedDateTime() {
  time_t now;
  struct tm * timeinfo;
  char buffer[30];
  time(&now);
  timeinfo = localtime(&now);
  strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M", timeinfo);
  return String(buffer);
}

time_t getUnixTime() {
    return time(nullptr);
}
