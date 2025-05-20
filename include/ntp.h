#pragma once
#include <Arduino.h>

void initNTP();
void updateNTP();
String getFormattedDateTime();
bool isTimeSynced();
time_t getUnixTime();
