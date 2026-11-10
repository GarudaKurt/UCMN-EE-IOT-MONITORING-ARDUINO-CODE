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
bool lastSsrOn = true;
static constexpr unsigned long SENSOR_INTERVAL_MS = 1000UL;
static constexpr unsigned long MQTT_INTERVAL_MS = 2000UL;

void setup() {
  Serial.begin(115200);
  broker.begin();
  pzemSensors.initPzemSensors();
  oledDisplay.initOled();
}

void loop() {
  broker.loop();
  const unsigned long now = millis();
  const bool ssrOn = broker.ssrOn();
  if (ssrOn != lastSsrOn) {
    if (ssrOn) lastSensorSample = now - SENSOR_INTERVAL_MS;
    else pzemSensors.invalidate(); // Never show or publish the last ON reading.
    lastSsrOn = ssrOn;
  }
  if (ssrOn && now - lastSensorSample >= SENSOR_INTERVAL_MS) {
    lastSensorSample = now;
    pzemSensors.sendSensorsData();
  }
  if (now - lastMqttPublish >= MQTT_INTERVAL_MS) {
    lastMqttPublish = now;
    broker.publishEnergy(pzemSensors.readings());
  }
  oledDisplay.screenDisplay(pzemSensors.readings(), broker.wifiConnected(), broker.connected(), ssrOn);
}
