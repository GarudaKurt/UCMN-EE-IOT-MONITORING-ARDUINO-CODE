#include "oledModule.h"
#include <Wire.h>

Display::Display() : oled(U8G2_R0, U8X8_PIN_NONE) {}

void Display::initOled() {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  oled.setI2CAddress(OLED_I2C_ADDRESS << 1);
  oled.begin();
  oled.setContrast(180);
  oled.setFont(u8g2_font_6x10_tf);
  oled.clearBuffer();
  oled.drawStr(0, 12, "UCMN ENERGY METER");
  oled.sendBuffer();
}

void Display::screenDisplay(const SensorReadings& r, bool wifi, bool mqtt) {
  const unsigned long now = millis();
  if (now - lastFrame < 250UL) return;
  lastFrame = now;
  pulse = !pulse;
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(0, 9, "PZEM-004T  |  LIVE");
  oled.drawStr(103, 9, wifi ? "W" : "-");
  oled.drawStr(116, 9, mqtt ? "M" : "-");
  oled.drawHLine(0, 11, 128);
  if (!r.valid()) {
    oled.drawStr(0, 30, "PZEM DISCONNECTED");
    oled.drawStr(0, 45, "Check UART/wiring");
  } else {
    char line[30];
    snprintf(line, sizeof(line), "V %6.1f V   I %5.2f A", r.voltage, r.current);
    oled.drawStr(0, 24, line);
    snprintf(line, sizeof(line), "P %6.1f W   E %5.2f kWh", r.power, r.energy);
    oled.drawStr(0, 37, line);
    snprintf(line, sizeof(line), "F %5.1f Hz  PF %4.2f", r.frequency, r.powerFactor);
    oled.drawStr(0, 50, line);
  }
  if (pulse) oled.drawDisc(124, 60, 2);
  oled.sendBuffer();
}
