#ifndef RFID_MODULE_H
#define RFID_MODULE_H

#include <Arduino.h>
#include <MFRC522.h>
#include "brokerController.h"

#define RFID_SCK_PIN 18 // WHITE
#define RFID_MOSI_PIN 23 // ORANGE
#define RFID_MISO_PIN 32 // YELLOW
#define RFID_SS_PIN 25 // BROWN
#define RFID_RST_PIN 26 // PRUOLE

class RfidReader {
public:
  RfidReader() : reader(RFID_SS_PIN, RFID_RST_PIN) {}
  void begin();
  // True only when an approved tap successfully changes the SSR state.
  bool loop(Controller& broker);

private:
  bool approved(const char* uid) const;
  MFRC522 reader;
  bool ready = false;
  unsigned long lastPoll = 0;
  unsigned long lastTap = 0;
  char lastUid[21] = {};
};

#endif
