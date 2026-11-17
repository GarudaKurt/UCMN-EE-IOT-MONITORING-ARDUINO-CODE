#line 1 "C:\\Users\\User\\Documents\\Arduino\\UCMN-EE-IOT-METER\\UCMN-EE-IOT-METER.ino"
#include <Arduino.h>
#include "pzemModule.h"
#include "oledModule.h"
#include "brokerController.h"
#include "rfidModule.h"

#ifndef WIFI_SSID
#define WIFI_SSID "KurtTyler"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "Pandemaca0"
#endif

Sensors pzemSensors;
Display oledDisplay;
Controller broker(WIFI_SSID, WIFI_PASSWORD);
RfidReader rfid;
unsigned long lastSensorSample = 0;
unsigned long lastMqttPublish = 0;
bool lastSsrOn = true;
static constexpr unsigned long SENSOR_INTERVAL_MS = 1000UL;
static constexpr unsigned long MQTT_INTERVAL_MS = 2000UL;

#line 24 "C:\\Users\\User\\Documents\\Arduino\\UCMN-EE-IOT-METER\\UCMN-EE-IOT-METER.ino"
void setup();
#line 32 "C:\\Users\\User\\Documents\\Arduino\\UCMN-EE-IOT-METER\\UCMN-EE-IOT-METER.ino"
void loop();
#line 24 "C:\\Users\\User\\Documents\\Arduino\\UCMN-EE-IOT-METER\\UCMN-EE-IOT-METER.ino"
void setup() {
  Serial.begin(115200);
  broker.begin();
  pzemSensors.initPzemSensors();
  oledDisplay.initOled();
  rfid.begin();
}

void loop() {
  broker.loop();
  const bool rfidSwitched = rfid.loop(broker);
  const unsigned long now = millis();
  const bool ssrOn = broker.ssrOn();
  if (ssrOn != lastSsrOn) {
    if (ssrOn) {
      lastSensorSample = now - SENSOR_INTERVAL_MS;
#if PZEM_DEBUG_LOGS
      Serial.println("[PZEM] SSR ON: requesting a fresh sample");
#endif
    } else {
      pzemSensors.invalidate(); // Never show or publish the last ON reading.
#if PZEM_DEBUG_LOGS
      Serial.println("[PZEM] SSR OFF: polling paused; cached readings invalidated");
#endif
    }
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
  if (rfidSwitched) oledDisplay.showPowerStatus(ssrOn);
}

