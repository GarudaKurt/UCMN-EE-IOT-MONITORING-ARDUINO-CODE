#line 1 "C:\\Users\\User\\Documents\\Arduino\\UCMN-EE-IOT-METER\\rfidCards.h"
#ifndef RFID_CARDS_H
#define RFID_CARDS_H

// Add approved card UIDs shown in the Serial Monitor,
// e.g. "04A1B2C3", "047A1122334455". Never accept every scanned UID.
// A UID is convenient identification, not strong authentication: some cards
// can be cloned. Use a stronger credential for high-security access.
static const char* const APPROVED_RFID_UIDS[] = { "FA95D94F" };

#endif
