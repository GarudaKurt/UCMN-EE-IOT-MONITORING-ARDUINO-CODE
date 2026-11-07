#ifndef OLED_MODULE_H
#define OLED_MODULE_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "pzemModule.h"

#ifndef OLED_SDA_PIN
#define OLED_SDA_PIN 21
#endif
#ifndef OLED_SCL_PIN
#define OLED_SCL_PIN 22
#endif
#ifndef OLED_I2C_ADDRESS
#define OLED_I2C_ADDRESS 0x3C
#endif

class Display {
public:
  Display();
  void initOled();
  void screenDisplay(const SensorReadings& readings, bool wifiConnected, bool mqttConnected);
private:
  U8G2_SSD1309_128X64_NONAME2_F_HW_I2C oled;
  unsigned long lastFrame = 0;
  bool pulse = false;
};

#endif
