#include "brokerController.h"
#include <ctype.h>
#include <math.h>

// Raw numeric JSON tokens retain six decimal places without turning values
// into JSON strings. Padding describes formatting, not sensor accuracy.
static void addDecimal(JsonObject obj, const char* key, float value) {
  if (!isfinite(value)) { obj[key] = nullptr; return; }
  char number[64];
  snprintf(number, sizeof(number), "%.6f", static_cast<double>(value));
  obj[key] = serialized(String(number)); // ArduinoJson owns the copied token.
}

static Controller* activeController = nullptr;

Controller::Controller(const char* wifiSsid, const char* wifiPassword)
  : ssid(wifiSsid), password(wifiPassword), client(espClient) {
  activeController = this;
}

void Controller::begin() {
  pinMode(SSR_PIN, OUTPUT);
  setSsr(true);  // Safe startup state: PZEM supply is ON.
  Serial.printf("[SSR] GPIO %u initialized HIGH; readback=%d\n", SSR_PIN, digitalRead(SSR_PIN));
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  client.setServer(MQTT_SERVER, MQTT_PORT);
  if (!client.setBufferSize(1024)) Serial.println("MQTT buffer allocation failed");
  client.setCallback(mqttCallback);
}

bool Controller::ensureMqtt() {
  if (client.connected() || !wifiConnected()) return client.connected();
  const unsigned long now = millis();
  if (now - lastReconnectAttempt < 5000UL) return false;
  lastReconnectAttempt = now;
  String clientId = String(DEVICE_ID) + "-" + String((uint32_t)esp_random(), HEX);
  Serial.printf("[MQTT] Connecting to %s:%u as %s\n", MQTT_SERVER, MQTT_PORT, clientId.c_str());
  if (client.connect(clientId.c_str())) {
    // The retained Node-RED command is delivered after this subscription.
    if (!client.subscribe(SSR_CONTROL_TOPIC, 1)) {
      Serial.println("MQTT SSR subscription failed");
      client.disconnect(); // Retry connection/subscription on the next retry interval.
      return false;
    } else {
      Serial.print("MQTT connected; subscription request sent for ");
      Serial.println(SSR_CONTROL_TOPIC);
    }
    return true;
  }
  Serial.printf("[MQTT] Reconnect failed, state=%d\n", client.state());
  return false;
}

void Controller::setSsr(bool enabled) {
  ssrState = enabled;
  digitalWrite(SSR_PIN, enabled ? HIGH : LOW);
  Serial.printf("[SSR] requested=%s output=%s readback=%d\n",
                enabled ? "ON" : "OFF",
                enabled ? "HIGH" : "LOW",
                digitalRead(SSR_PIN));
}

void Controller::mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (activeController != nullptr) {
    activeController->handleMqttMessage(topic, payload, length);
  }
}

void Controller::handleMqttMessage(const char* topic, const byte* payload, unsigned int length) {
  Serial.printf("[MQTT RX] topic=%s length=%u payload=", topic, length);
  Serial.write(payload, length);
  Serial.println();

  if (strcmp(topic, SSR_CONTROL_TOPIC) != 0) {
    Serial.println("[MQTT RX] ignored: topic does not match SSR control topic");
    return;
  }
  if (length == 0 || length > 16) {
    Serial.println("[MQTT RX] ignored: invalid payload length");
    return;
  }

  Serial.println("[SSR] command topic matched; parsing payload");

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
    Serial.printf("[SSR] ignored invalid command: %s\n", command);
  }
}

void Controller::loop() {
  ensureMqtt();
  if (client.connected()) client.loop();

  const unsigned long now = millis();
  if (now - lastStatusLog >= 10000UL) {
    lastStatusLog = now;
    Serial.printf("[STATUS] WiFi=%s MQTT=%s MQTT_state=%d SSR=%s GPIO19=%d\n",
                  wifiConnected() ? "connected" : "disconnected",
                  client.connected() ? "connected" : "disconnected",
                  client.state(),
                  ssrState ? "ON" : "OFF",
                  digitalRead(SSR_PIN));
  }
}

void Controller::addReadings(JsonObject obj, const SensorReadings& d) {
  obj["ok"] = d.valid();
  if (!d.valid()) {
    obj["error"] = static_cast<uint8_t>(d.error);
    return;
  }
  addDecimal(obj, "voltage", d.voltage);
  addDecimal(obj, "current", d.current);
  addDecimal(obj, "power", d.power);
  addDecimal(obj, "energy", d.energy);
  addDecimal(obj, "frequency", d.frequency);
  addDecimal(obj, "power_factor", d.powerFactor);
}

bool Controller::publishEnergy(const SensorReadings& readings) {
  if (!client.connected()) return false;
  StaticJsonDocument<1536> doc;
  doc["device_id"] = DEVICE_ID;
  doc["uptime_s"] = (unsigned long)(millis() / 1000UL);
  doc["ssr_on"] = ssrState;

  addReadings(doc["master"].to<JsonObject>(), readings);
  addReadings(doc["sensor"].to<JsonObject>(), readings);
  if (doc.overflowed()) { Serial.println("MQTT JSON capacity exceeded"); return false; }
  char payload[1024];
  const size_t n = serializeJson(doc, payload, sizeof(payload));
  if (n == 0 || n >= sizeof(payload)) return false;
  return client.publish(MQTT_TOPIC, payload);
}
