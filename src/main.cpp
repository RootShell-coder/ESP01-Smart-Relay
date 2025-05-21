#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include "settings.h"
#include "wifi.h"
#include "relay.h"
#include "ntp.h"
#include "metrics.h"
#include "sun.h"

// Полностью запретить микроконтроллеру входить в режим прошивки без аппаратных мер невозможно.
// Даже при самом раннем подъёме GPIO0 в '1' загрузчик может выдать кратковременный импульс.
// Намеренное "отключение" режима прошивки предполагает аппаратную подтяжку GPIO0 к 3.3В (10 кОм)
// или перенос реле на иной пин, не участвующий в загрузочном процессе.
// Специальный конструктор, выполняющийся раньше initVariant.
// Абсолютно гарантировать отсутствие короткого импульса нельзя,
// но это максимально ранняя точка старта, и GPIO0 будет быстро поднят в '1'.
// Также после этого пин GPIO0 не сможет использоваться для входа в режим прошивки.
//
// Тихое включение: Что бы реле не включалось на 100 ms  при включении, нужно доработать модуль реле
// и в прошивке relay.h переопределить GPIO0 (#define RELAY_PIN 0) на GPIO3 (#define RELAY_PIN 3)

__attribute__((constructor)) void forceHighGPIO0() {
  // Устанавливаем пин на выход
  GPC0 = (GPC0 & ~(0b111 << (RELAY_PIN * 3))) | (0b010 << (RELAY_PIN * 3));
  GPOS = (1 << RELAY_PIN);
  PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);
  GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1 << RELAY_PIN);
  GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(RELAY_PIN)),
      GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(RELAY_PIN))) & (~GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE)));
}
void initVariant() {
  GPC0 = (GPC0 & ~(0b111 << (RELAY_PIN * 3))) | (0b010 << (RELAY_PIN * 3));
  GPOS = (1 << RELAY_PIN);
}

void setup() {
  ESP.wdtDisable();
  ESP.wdtEnable(WDTO_8S);
  delay(500);
  loadSettings();
  initWiFi();
  initNTP();
  relayInit();
  initSun();
}

void loop() {
  metrics();
  ESP.wdtFeed();
  checkWiFiConnection();
  relaySwitch();
  updateNTP();
  handleSunData();
}
