# RC522 RFID control

The RC522 module uses 3.3 V power, not 5 V. Connect GND to ESP32 GND.
Its pin labeled SDA is SPI chip-select, not OLED I2C SDA.

| RC522 | ESP32 |
| --- | --- |
| SDA / SS | GPIO 25 |
| SCK | GPIO 18 |
| MOSI | GPIO 23 |
| MISO | GPIO 32 |
| RST | GPIO 26 |
| 3.3V | 3V3 |
| GND | GND |

Existing PZEM UART uses GPIO 16/17, OLED I2C 21/22, and SSR output 19.
GPIO 32 is the remapped SPI MISO input, avoiding the SSR on GPIO 19.
Confirm these GPIO numbers exist on your exact ESP32 board before wiring.

With Serial Monitor at 115200, tap each card once and note `[RFID TAP] ... UID=...`.
The approved card UID is `FA95D94F` (`FA:95:D9:4F`). Add further quoted UIDs
in `rfidCards.h` if needed, then upload again. Unknown cards are
rejected. A repeated read within 1.5 seconds is ignored. Card UIDs can be
cloned and are not strong authentication for a security-sensitive load.

After an approved tap successfully changes the SSR, the OLED shows `POWER ON`
or `POWER OFF` for three seconds, then returns to the six PZEM readings.
Denied taps and failed MQTT requests leave the readings on screen.

Import `node-red/rfid-ssr-sync.json` into the existing **SSR Poll** tab and
deploy. It refers to the existing broker config node `a7` and requires the
existing Node-RED environment variable `SUPABASE_KEY` with permission to
UPDATE the `public.pzem_status` row for `rental-house`. It writes the new
`is_on` value and publishes the confirmed state to `iotmeter/ssr/set`.
Do not expose the Supabase key or put it in ESP32 firmware.

An approved tap requires MQTT connectivity. The ESP32 publishes the intended
state to `iotmeter/ssr/request` and switches the SSR immediately after the
broker accepts the publish. If Node-RED/Supabase rejects the change, the
existing three-second database poll will restore the authoritative table
state. If no MQTT connection exists, the SSR does not change. The existing
PZEM and OLED handling of SSR transitions remains unchanged.

The public HiveMQ broker has no device authentication. Other clients could
forge MQTT commands, including requests to update this database row. Use a
private authenticated broker before relying on RFID control for security or
critical loads; a card UID alone is also cloneable.
