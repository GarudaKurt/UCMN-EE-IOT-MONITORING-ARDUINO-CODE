#include "pzemModule.h"
#include <math.h>

Sensors::Sensors(int32_t rx, int32_t tx)
  : rxPin(rx), txPin(tx), serial(2), pzem(serial, rx, tx) {}

void Sensors::initPzemSensors() {
  serial.begin(9600, SERIAL_8N1, rxPin, txPin);
}

bool Sensors::sendSensorsData() {
  SensorReadings sample;
  sample.voltage = pzem.voltage();
  sample.current = pzem.current();
  sample.power = pzem.power();
  sample.energy = pzem.energy();
  sample.frequency = pzem.frequency();
  sample.powerFactor = pzem.pf();
  if (isnan(sample.voltage)) sample.error = ErrorType::voltage;
  else if (isnan(sample.current)) sample.error = ErrorType::current;
  else if (isnan(sample.power)) sample.error = ErrorType::power;
  else if (isnan(sample.energy)) sample.error = ErrorType::energy;
  else if (isnan(sample.frequency)) sample.error = ErrorType::frequency;
  else if (isnan(sample.powerFactor)) sample.error = ErrorType::powerFactor;
  else sample.error = ErrorType::none;
  latest = sample;
  return latest.valid();
}
