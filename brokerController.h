#ifndef BROKER_CONTROLLER_H
#define BROKER_CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "pzemModule.h"

#ifndef SSR_PIN
#define SSR_PIN 19
#endif

class Controller {
public:
  Controller(const char* ssid, const char* password);
  void begin();
  void loop();
  bool connected() { return client.connected(); }
  bool wifiConnected() const { return WiFi.status() == WL_CONNECTED; }
  bool publishEnergy(const SensorReadings& readings);
  bool ssrOn() const { return ssrState; }

private:
  static void mqttCallback(char* topic, byte* payload, unsigned int length);
  void handleMqttMessage(const char* topic, const byte* payload, unsigned int length);
  void setSsr(bool enabled);
  bool ensureMqtt();
  void addReadings(JsonObject obj, const SensorReadings& readings);
  const char* ssid;
  const char* password;
  // MQTT configuration intentionally retained.
  const char* MQTT_SERVER = "broker.hivemq.com";
  const uint16_t MQTT_PORT = 1883;
  const char* MQTT_TOPIC = "iotmeter";
  const char* DEVICE_ID = "esp32-iot-meter";
  const char* SSR_CONTROL_TOPIC = "iotmeter/ssr/set";
  unsigned long lastReconnectAttempt = 0;
  unsigned long lastStatusLog = 0;
  bool ssrState = true;
  WiFiClient espClient;
  PubSubClient client;
};

#endif
