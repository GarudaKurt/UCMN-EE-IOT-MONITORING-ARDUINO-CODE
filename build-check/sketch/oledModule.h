#line 1 "C:\\Users\\User\\Documents\\Arduino\\UCMN-EE-IOT-METER\\oledModule.h"
#ifndef OLED_MODULE_H
#define OLED_MODULE_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "pzemModule.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

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
  void screenDisplay(const SensorReadings& readings, bool wifiConnected, bool mqttConnected, bool ssrOn);
  void showPowerStatus(bool ssrOn);
private:
  U8G2_SSD1309_128X64_NONAME2_F_HW_I2C oled;
  static void taskEntry(void* self);
  void render(const SensorReadings& readings, bool wifi, bool mqtt, bool ssrOn,
              bool showStatus, uint32_t now);
  void cell(int x, int y, int width, float value, const char* unit, int decimals, bool ssrOn);
  portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
  SensorReadings snapshot;
  bool wifiState = false, mqttState = false, ssrState = true;
  bool powerStatusActive = false, powerStatusOn = false;
  uint32_t powerStatusAt = 0;
  uint32_t snapshotAt = 0;
  uint8_t previous[1024] = {};
  TaskHandle_t task = nullptr;
};

#endif
