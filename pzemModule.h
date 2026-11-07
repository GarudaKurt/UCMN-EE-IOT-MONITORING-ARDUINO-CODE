#ifndef PZEM_MODULE_H
#define PZEM_MODULE_H

#include <Arduino.h>
#include <PZEM004Tv30.h>

#ifndef PZEM_RX_PIN
#define PZEM_RX_PIN 16
#endif
#ifndef PZEM_TX_PIN
#define PZEM_TX_PIN 17
#endif

enum class ErrorType : uint8_t { none, voltage, current, power, energy,
                                 frequency, powerFactor, disconnected };

struct SensorReadings {
  float voltage = NAN;
  float current = NAN;
  float power = NAN;
  float energy = NAN;
  float frequency = NAN;
  float powerFactor = NAN;
  ErrorType error = ErrorType::disconnected;
  bool valid() const { return error == ErrorType::none; }
};

class Sensors {
public:
  Sensors(int32_t rx = PZEM_RX_PIN, int32_t tx = PZEM_TX_PIN);
  void initPzemSensors();
  bool sendSensorsData();
  const SensorReadings& readings() const { return latest; }

private:
  int32_t rxPin;
  int32_t txPin;
  HardwareSerial serial;
  PZEM004Tv30 pzem;
  SensorReadings latest;
};

#endif
