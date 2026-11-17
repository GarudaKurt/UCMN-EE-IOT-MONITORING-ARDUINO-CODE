#line 1 "C:\\Users\\User\\Documents\\Arduino\\UCMN-EE-IOT-METER\\rfidModule.cpp"
#include "rfidModule.h"
#include "rfidCards.h"
#include <SPI.h>
#include <string.h>

void RfidReader::begin() {
  SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SS_PIN);
  reader.PCD_Init();
  const byte version = reader.PCD_ReadRegister(MFRC522::VersionReg);
  ready = version != 0x00 && version != 0xFF;
  Serial.printf("[RFID] RC522 version=0x%02X %s (SCK=%d MOSI=%d MISO=%d SS=%d RST=%d)\n",
                version, ready ? "ready" : "not responding",
                RFID_SCK_PIN, RFID_MOSI_PIN, RFID_MISO_PIN, RFID_SS_PIN, RFID_RST_PIN);
}

bool RfidReader::approved(const char* uid) const {
  for (const char* allowed : APPROVED_RFID_UIDS) {
    if (allowed != nullptr && strcmp(uid, allowed) == 0) return true;
  }
  return false;
}

void RfidReader::loop(Controller& broker) {
  if (!ready) return;
  const unsigned long now = millis();
  if (now - lastPoll < 100UL) return;
  lastPoll = now;
  if (!reader.PICC_IsNewCardPresent() || !reader.PICC_ReadCardSerial()) return;

  char uid[21] = {};
  const byte size = reader.uid.size;
  if (size <= 10) {
    static const char hex[] = "0123456789ABCDEF";
    for (byte i = 0; i < size; ++i) {
      uid[i * 2] = hex[reader.uid.uidByte[i] >> 4];
      uid[i * 2 + 1] = hex[reader.uid.uidByte[i] & 0x0F];
    }
  }
  reader.PICC_HaltA();
  reader.PCD_StopCrypto1();
  if (size == 0 || size > 10) {
    Serial.printf("[RFID TAP] t=%lu ignored: unsupported UID length=%u\n", now, size);
    return;
  }
  Serial.printf("[RFID TAP] t=%lu UID=%s SSR_before=%s\n",
                now, uid, broker.ssrOn() ? "ON" : "OFF");
  if (strcmp(uid, lastUid) == 0 && now - lastTap < 1500UL) {
    Serial.println("[RFID TAP] ignored: repeat within 1.5 seconds");
    return;
  }
  strncpy(lastUid, uid, sizeof(lastUid) - 1);
  lastTap = now;

  if (!approved(uid)) {
    Serial.println("[RFID TAP] denied: UID not in rfidCards.h; SSR unchanged");
    return;
  }
  if (!broker.toggleFromRfid()) {
    Serial.println("[RFID TAP] approved, but MQTT request failed; SSR unchanged");
  } else {
    Serial.printf("[RFID TAP] approved: SSR_now=%s\n", broker.ssrOn() ? "ON" : "OFF");
  }
}
