#include "brokerController.h"

Controller::Controller(const char* wifiSsid, const char* wifiPassword)
  : ssid(wifiSsid), password(wifiPassword), client(espClient) {}

void Controller::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setBufferSize(512);
}

bool Controller::ensureMqtt() {
  if (client.connected() || !wifiConnected()) return client.connected();
  const unsigned long now = millis();
  if (now - lastReconnectAttempt < 5000UL) return false;
  lastReconnectAttempt = now;
  String clientId = String(DEVICE_ID) + "-" + String((uint32_t)esp_random(), HEX);
  if (client.connect(clientId.c_str())) return true;
  Serial.printf("MQTT reconnect failed, rc=%d\n", client.state());
  return false;
}

void Controller::loop() {
  ensureMqtt();
  if (client.connected()) client.loop();
}

void Controller::addReadings(JsonObject obj, const SensorReadings& d) {
  obj["ok"] = d.valid();
  if (!d.valid()) {
    obj["error"] = static_cast<uint8_t>(d.error);
    return;
  }
  obj["voltage"] = d.voltage;
  obj["current"] = d.current;
  obj["power"] = d.power;
  obj["energy"] = d.energy;
  obj["frequency"] = d.frequency;
  obj["power_factor"] = d.powerFactor;
}

bool Controller::publishEnergy(const SensorReadings& readings) {
  if (!client.connected()) return false;
  StaticJsonDocument<512> doc;
  doc["device_id"] = DEVICE_ID;
  doc["uptime_s"] = (unsigned long)(millis() / 1000UL);

  addReadings(doc["master"].to<JsonObject>(), readings);
  addReadings(doc["sensor"].to<JsonObject>(), readings);
  char payload[512];
  const size_t n = serializeJson(doc, payload, sizeof(payload));
  if (n == 0 || n >= sizeof(payload)) return false;
  return client.publish(MQTT_TOPIC, payload);
}
