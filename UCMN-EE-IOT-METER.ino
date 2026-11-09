#include <Arduino.h>
#include "pzemModule.h"
#include "oledModule.h"
#include "brokerController.h"

#ifndef WIFI_SSID
#define WIFI_SSID "KurtTyler"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "Pandemaca0"
#endif

Sensors pzemSensors;
Display oledDisplay;
Controller broker(WIFI_SSID, WIFI_PASSWORD);
unsigned long lastSensorSample = 0;
unsigned long lastMqttPublish = 0;
static constexpr unsigned long SENSOR_INTERVAL_MS = 1000UL;
static constexpr unsigned long MQTT_INTERVAL_MS = 2000UL;

void setup() {
  Serial.begin(115200);
  broker.begin();
  pzemSensors.initPzemSensors();
  oledDisplay.initOled();
}

void loop() {
  const unsigned long now = millis();
  broker.loop();
  if (now - lastSensorSample >= SENSOR_INTERVAL_MS) {
    lastSensorSample = now;
    pzemSensors.sendSensorsData();
  }
  if (now - lastMqttPublish >= MQTT_INTERVAL_MS) {
    lastMqttPublish = now;
    broker.publishEnergy(pzemSensors.readings());
  }
  oledDisplay.screenDisplay(pzemSensors.readings(), broker.wifiConnected(), broker.connected());
}
