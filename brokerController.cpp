#include "brokerController.h"
#include <ctype.h>

static Controller* activeController = nullptr;

Controller::Controller(const char* wifiSsid, const char* wifiPassword)
  : ssid(wifiSsid), password(wifiPassword), client(espClient) {
  activeController = this;
}

void Controller::begin() {
  pinMode(SSR_PIN, OUTPUT);
  setSsr(true);  // Safe startup state: PZEM supply is ON.
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setBufferSize(512);
  client.setCallback(mqttCallback);
}

bool Controller::ensureMqtt() {
  if (client.connected() || !wifiConnected()) return client.connected();
  const unsigned long now = millis();
  if (now - lastReconnectAttempt < 5000UL) return false;
  lastReconnectAttempt = now;
  String clientId = String(DEVICE_ID) + "-" + String((uint32_t)esp_random(), HEX);
  if (client.connect(clientId.c_str())) {
    // The retained Node-RED command is delivered after this subscription.
    if (!client.subscribe(SSR_CONTROL_TOPIC, 1)) {
      Serial.println("MQTT SSR subscription failed");
    }
    return true;
  }
  Serial.printf("MQTT reconnect failed, rc=%d\n", client.state());
  return false;
}

void Controller::setSsr(bool enabled) {
  ssrState = enabled;
  digitalWrite(SSR_PIN, enabled ? HIGH : LOW);
}

void Controller::mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (activeController != nullptr) {
    activeController->handleMqttMessage(topic, payload, length);
  }
}

void Controller::handleMqttMessage(const char* topic, const byte* payload, unsigned int length) {
  if (strcmp(topic, SSR_CONTROL_TOPIC) != 0 || length == 0 || length > 16) return;

  char command[17] = {};
  for (unsigned int i = 0; i < length; ++i) {
    command[i] = static_cast<char>(tolower(payload[i]));
  }

  if (strcmp(command, "true") == 0 || strcmp(command, "on") == 0 || strcmp(command, "1") == 0) {
    setSsr(true);
    Serial.println("SSR command: ON");
  } else if (strcmp(command, "false") == 0 || strcmp(command, "off") == 0 || strcmp(command, "0") == 0) {
    setSsr(false);
    Serial.println("SSR command: OFF");
  } else {
    Serial.printf("Ignored invalid SSR command: %s\n", command);
  }
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
  doc["ssr_on"] = ssrState;

  addReadings(doc["master"].to<JsonObject>(), readings);
  addReadings(doc["sensor"].to<JsonObject>(), readings);
  char payload[512];
  const size_t n = serializeJson(doc, payload, sizeof(payload));
  if (n == 0 || n >= sizeof(payload)) return false;
  return client.publish(MQTT_TOPIC, payload);
}
