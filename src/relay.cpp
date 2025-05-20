#include <Arduino.h>
#include "settings.h"
#include "relay.h"

void relayInit() {
  digitalWrite(RELAY_PIN, HIGH);
  pinMode(RELAY_PIN, OUTPUT);
}

void relayOn() {
  digitalWrite(RELAY_PIN, LOW);
}

void relayOff() {
  digitalWrite(RELAY_PIN, HIGH);
}

void relaySwitch() {
  if (relay.state) {
    relayOn();
  } else {
    relayOff();
  }
}
