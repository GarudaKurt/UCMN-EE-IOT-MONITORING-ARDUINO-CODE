#ifndef BROKER_CONTROLLER_H
#define BROKER_CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "pzemModule.h"

class Controller {
public:
  Controller(const char* ssid, const char* password);
  void begin();
  void loop();
  bool connected() const { return client.connected(); }
  bool wifiConnected() const { return WiFi.status() == WL_CONNECTED; }
  bool publishEnergy(const SensorReadings& readings);

private:
  bool ensureMqtt();
  void addReadings(JsonObject obj, const SensorReadings& readings);
  const char* ssid;
  const char* password;
  // MQTT configuration intentionally retained.
  const char* MQTT_SERVER = "broker.hivemq.com";
  const uint16_t MQTT_PORT = 1883;
  const char* MQTT_TOPIC = "iotmeter";
  const char* DEVICE_ID = "esp32-iot-meter";
  unsigned long lastReconnectAttempt = 0;
  WiFiClient espClient;
  PubSubClient client;
};

#endif
