#include "pzemModule.h"
#include <math.h>

Sensors::Sensors(int32_t rx, int32_t tx)
  : rxPin(rx), txPin(tx), serial(2), pzem(serial, rx, tx) {}

void Sensors::initPzemSensors() {
  serial.begin(9600, SERIAL_8N1, rxPin, txPin);
#if PZEM_DEBUG_LOGS
  Serial.printf("[PZEM] UART2 initialized at 9600 baud, RX=%ld TX=%ld\n",
                static_cast<long>(rxPin), static_cast<long>(txPin));
#endif
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
#if PZEM_DEBUG_LOGS
  Serial.printf("[PZEM SAMPLE] t=%lu ok=%d error=%u V=%.6f A=%.6f W=%.6f kWh=%.6f Hz=%.6f PF=%.6f\n",
                static_cast<unsigned long>(millis()), latest.valid(),
                static_cast<unsigned>(latest.error),
                latest.voltage, latest.current, latest.power, latest.energy,
                latest.frequency, latest.powerFactor);
#endif
  return latest.valid();
}
