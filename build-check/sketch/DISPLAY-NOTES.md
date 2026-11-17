#line 1 "C:\\Users\\User\\Documents\\Arduino\\UCMN-EE-IOT-METER\\DISPLAY-NOTES.md"
# OLED monitoring update

SSD1309 128x64, U8g2 hardware I2C, SDA 21 / SCL 22, address 0x3C.
The renderer checks the library-reported dimensions before starting.

Layout: header occupies y=0..10, separator y=13, and six 64x16 cells
occupy y=16..63. Order is voltage/power, frequency/current, energy/PF.
The two outlined header indicators represent Wi-Fi (left) and MQTT (right).
Filled means connected in the latest snapshot. A snapshot older than 3.5 s
is marked unavailable. The display uses no animated icons. The text-only
header moves one pixel every 80 ms. Each reading uses a 5x8 font within
its 64x16 cell. Numeric text is width-tested
and rounded for display only; extreme values fall back to scientific notation
or OVF.

Only the OLED task owns U8g2 after initialization. The main loop copies a
snapshot under a short critical section; no I2C or rendering happens under
that lock. Changed 8x8 tile runs are transferred instead of whole frames.
The task yields between frames and keeps the header scrolling during network stalls.

An approved RFID tap that successfully changes the SSR displays `POWER ON`
or `POWER OFF` for three seconds. The sensor view then returns. The status
screen has no moving elements, and sampling and MQTT continue while it is shown.

MQTT numeric tokens have six fractional digits. This does not increase PZEM
accuracy, and binary float representation can appear in the final digits.
Node-RED will parse them as numbers and may remove trailing zeros when it
serializes them again. Topics, sensor units, device ID, SSR handling, and
the 2000 ms publish interval are unchanged.

Existing PubSubClient connect calls and PZEM reads remain synchronous.
The display task isolates animation from their latency; this does not make
network reconnects or sensor polling asynchronous or guarantee hard realtime
publish timing during outages.

Hardware acceptance: check header wrap, all six cells at maximum expected
values, invalid sensor placeholders, both connection indicators, Wi-Fi
outage animation, 2-second MQTT reception, and true/false SSR commands.
GPIO readback reports the command pin, not physical relay/load feedback.

When the SSR is OFF, polling pauses, the cached sample is invalidated, and
the OLED shows `NaN` for all six values. MQTT continues on its two-second
schedule with `ssr_on=false` and `ok=false` (no stale numeric readings). When
the SSR turns ON, the first PZEM sample is requested immediately; readings
return only after a valid sample. `NaN` on the OLED is a display marker, not
a claim that the physical meter measured a non-number.
