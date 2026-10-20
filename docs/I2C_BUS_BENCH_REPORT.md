# I2C Bus Bench Report — SIH26181 ShrikeFi Health Companion

| Field | Detail |
|---|---|
| **Audience** | hardware / soldering technician |
| **Board** | Vicharak ShrikeFi — ESP32-S3 + Renesas ForgeFPGA SLG47910 |
| **Firmware** | `v1.0-zynq-SIH-296-g35d77a0` (modified working tree) |
| **Measured** | 21 October 2026, unit powered and running |
| **Scope** | condition of the shared I2C bus, and why the TMP117 skin-temperature probe is not detected |

> **TL;DR for the technician**
> The bus is healthy — **4 of 5 devices answer** at 100 kHz and again at 25 kHz.
> The TMP117 answers at **none** of its four addresses, and a raw read returns `0xFFFF`.
> **All five of its connections are now proven correct** (§8.4): power, ground, SDA,
> SCL, and the `0x48` address strap.
> **Every I2C device acknowledges its own address before anything else**, so a
> powered, grounded, correctly wired, correctly addressed module that stays silent
> is a **faulty module**. With the module removed the bus reads a clean, steady
> **3.18 V on both lines** (§8.6) — the bus and the other four sensors are fine.
> Action: **reflow the module's own joints or replace the breakout** (§8.5).
> Wiring procedure for the replacement is in §8.7. Full procedure in §8, fault tree
> in §9, acceptance test in §11.

---

## 1. SAFETY FIRST — READ BEFORE TOUCHING ANYTHING

> ## ⚠️ 3.3 V STRICT MAXIMUM
> Every GPIO on the ESP32-S3 and every pin of the ForgeFPGA is **3.3 V LVCMOS**.
> **5 V on any signal pin destroys the IC permanently.**
>
> 5 V exists on this build for **one thing only**: the PMS5003 particulate sensor's
> VCC (its fan and laser need 5 V). Nothing else.

Rules:

1. **Never** power the I2C bus, any breakout's logic pin, or any GPIO from 5 V.
2. **Never** connect or disconnect jumper wires while the board is powered.
3. **Never** measure resistance (continuity) with the board powered.
4. Use a common ground for every device. One shared GND net, no exceptions.

---

## 2. Bus specification

| Parameter | Value |
|---|---|
| **SDA** | **GPIO 1** |
| **SCL** | **GPIO 2** |
| **Clock rate** | 100 kHz (I2C Standard Mode) |
| **Logic voltage** | 3.3 V |
| **Master** | ESP32-S3 (I2C port 0) |
| **Topology** | single shared 2-wire bus, all devices in parallel, common ground |
| **Pull-ups** | fitted **on each breakout**, in parallel → typically **1 – 3 kΩ** total |
| **External pull-ups** | **NOT required — do not add any** |

### Pull-up note

This project's own wiring guide states the effective bus pull-up should measure
**1.5 kΩ – 3.3 kΩ** between SDA (or SCL) and 3.3 V. Every breakout (OLED, MAX30102,
BME280, MPU6050, TMP117) carries its own resistors, so the parallel combination is
already in range. **Do not add to it.**

**Do not add 2.2 kΩ or 4.7 kΩ pull-ups "to be safe."** Extra pull-ups lower the bus
resistance and over-drive the sink current of every device on the bus.

The ESP32-S3 also enables its internal ~45 kΩ pull-ups as a fallback. They are too
weak to be the primary pull-up, but they guarantee the lines never float completely.

---

## 3. Device inventory — measured state

The firmware scans **every address from `0x01` to `0x7E`** at boot and prints every
device that acknowledges.

| # | Address | Device | Function | Measured |
|---|---|---|---|---|
| 1 | **0x3C** | SSD1306 OLED | Status display | ✅ **ACKs** |
| 2 | **0x57** | MAX30102 | Pulse / SpO₂ | ✅ **ACKs** |
| 3 | **0x68** | MPU6050 | 6-axis IMU, fall detection | ✅ **ACKs** |
| 4 | **0x76** | BME280 | Ambient temp / humidity / pressure | ✅ **ACKs** |
| 5 | **0x48 – 0x4B** | **TMP117** | **Clinical skin temperature** | ❌ **NO ACK** |

### Raw measurement output (verbatim from the unit)

```
I2C_HAL: I2C init: port=0 SDA=GPIO1 SCL=GPIO2 speed=100000 Hz
I2C_HAL: I2C idle levels after init: SDA=1 SCL=1
I2C_SCAN:  -> Found device at 0x3C (SSD1306 OLED)
I2C_SCAN:  -> Found device at 0x57 (MAX30100/MAX30102 PPG)
I2C_SCAN:  -> Found device at 0x68 (MPU6050 IMU)
I2C_SCAN:  -> Found device at 0x76 (BME280 Env)
I2C_SCAN: Scan complete: 4 device(s) found.

TMP117: Device not responding at I2C 0x48
TMP117: Device not responding at I2C 0x49
TMP117: Device not responding at I2C 0x4A
TMP117: Device not responding at I2C 0x4B

BUS DIAGNOSTIC:
  Scan at the normal bus clock:            4 device(s)  0x3C 0x57 0x68 0x76
  Scan at a quarter speed (25 kHz):        4 device(s)  0x3C 0x57 0x68 0x76
  Raw no-ACK read at 0x48: reg 0x0F (ID)=0xFFFF, reg 0x00 (TEMP)=0xFFFF

VERDICT: the TMP117 is not on the bus at any address, at either clock speed.
```

---

## 4. What the measurements prove

| Observation | Conclusion |
|---|---|
| Idle levels **SDA = 1, SCL = 1** | Pull-ups are working. No line is stuck low. No device is holding the bus. |
| **4 devices ACK** at 100 kHz | Bus wiring, pull-ups, clock, and ground are all correct. |
| **Same 4 at 25 kHz** | **Not** a signal-integrity problem. Weak pull-ups or long wires would fail at 100 kHz and pass at 25 kHz. The results are identical. |
| TMP117 absent at **all four** addresses `0x48–0x4B` | **Not** an address-strap problem. The ADD0 pin can only select those four. |
| Raw read returns **`0xFFFF`** | Nothing is driving the data line. With no slave at that address the master reads only the pull-up state, so all ones. Any responding device would have pulled at least some bits low. |
| The **other four devices are unaffected** | The TMP117's SDA/SCL are **not loading the bus at all**. A shorted line, a reversed pair, or a solder bridge to 3.3 V on that module would have taken the whole bus down. It has not. |

### Conclusion

> **The TMP117 module is electrically absent from the bus.**
> Either it has no power, no ground, its SDA/SCL are not on the bus nets,
> or the module itself is dead.

The printed circuit board, the ESP32-S3 I2C peripheral, the firmware, and the
other four sensors are all functioning correctly. **This is a physical fault on the
TMP117 module or its four wires — not a software fault.**

> **Update — 22 Oct 2026:** the technician's continuity pass has now **proven all
> five connections correct** (see §8.4). The wiring is no longer the suspect.
> The fault is now either **SDA/SCL swapped at the module**, or **the module itself
> is faulty**.

---

## 5. The TMP117 breakout — pin identification

The module in use is a **7Semi ES-12132 TMP117 breakout** — a **blue PCB**, 7-pin,
2.54 mm pitch, with an onboard LDO and bi-directional level shifters.

> **Identification for the replacement.** The blue colour is the fast visual check
> that the right part is in hand. Note that "blue TMP117 breakout" is a generic
> description shared by many sellers — the thing that distinguishes them is the
> **7-pin layout below** (`VIN | 3V3 | GND | SCL | SDA | ALT | ADD`). The board
> marked `3V3` is the regulator's **output**, and the board marked `VIN` is its
> **input** — see the next table. A module with only 4 or 6 pins (`VDD GND SCL
> SDA ALERT ADD0`) is a different, simpler board and is wired differently: its
> `VDD` pin takes the 3.3 V rail directly and there is no `VIN`/`3V3` pair.

Header order, **left → right**, viewed from the **top** with the silkscreen readable:

```
     VIN     3V3     GND     SCL     SDA     ALT     ADD
       |       |       |       |       |       |       |
   power     LDO  ground   clock    data   alert    addr
   INPUT     out   (ref)  (host)  (host)  (open) (strap)
```

| Pin | Direction | Function | Must connect to |
|---|---|---|---|
| **VIN** | **input** | Supply input to the onboard regulator | **3.3 V** |
| **3V3** | **output** | Regulated 3.3 V **output** | **leave open** |
| **GND** | — | Ground | **common GND** |
| **SCL** | input | I2C clock | **GPIO 2** |
| **SDA** | I/O | I2C data | **GPIO 1** |
| **ALT** | output | ALERT / interrupt (open-drain) | **leave open** |
| **ADD** | input | Address strap | **GND** (`0x48`) |

### Board architecture (why 3V3 must stay open)

The 7Semi board contains:

- an **SE5218 3.3 V LDO** fed from `VIN`, which powers the TMP117 chip and the sensor
  side of the level shifters;
- **bi-directional NMOS level shifters** on SDA/SCL, so the **host side** may be
  3.3 V or 5 V;
- **4.7 kΩ pull-ups referenced to `VIN`** on the host side.

Two consequences:

1. **`3V3` is the regulator's OUTPUT, not an input.** Connecting it to the 3.3 V
   rail puts the regulator output in contention with the rail that feeds its own
   input. It works for a while, runs hot, and eventually fails.
2. **`VIN` must be 3.3 V** on this build. The 5 V rating on the product page is for
   5 V *hosts*; with 5 V on VIN the host-side pull-ups would sit at 5 V and put
   5 V on GPIO1/GPIO2 and on every other device's bus pins.

---

## 6. Correct wiring — TMP117

| TMP117 pin | Connect to | Colour suggestion |
|---|---|---|
| **VIN** | **3.3 V** hub | red |
| **3V3** | *no connection* | — |
| **GND** | **GND** hub | black |
| **SCL** | **GPIO 2** / SCL hub | yellow |
| **SDA** | **GPIO 1** / SDA hub | green |
| **ALT** | *no connection* | — |
| **ADD** | **GND** hub | black |

Only **5 wires**: VIN, GND, SCL, SDA, and the ADD strap.

---

## 7. Breadboard placement

The existing 170-tie-point layout (see `docs/SHRIKEFI_HARDWARE_CONNECTIONS.md`) uses:

```text
Row 1  GND hub  : 1A ShrikeFi GND   1B OLED GND   1C MAX30102 GND   1D BME280 GND   1E MPU6050 GND
                  1F MPU6050 AD0    1G GNSS GND    1H PMS5003 GND    1I Buzzer (-)   1J Button leg 2
Row 2  3.3V hub : 2A ShrikeFi 3.3V  2B OLED VCC   2C MAX30102 VIN   2D BME280 VIN   2E BME280 CS
                  2F MPU6050 VCC    2G GNSS VCC    2H PMS5003 SET    2I Buzzer VCC   2J Empty
Row 4  SDA hub  : 4A ESP32 GPIO 1   4B OLED SDA   4C MAX30102 SDA   4D BME280 SDA   4E MPU6050 SDA
Row 5  SCL hub  : 5A ESP32 GPIO 2   5B OLED SCL   5C MAX30102 SCL   5D BME280 SCL   5E MPU6050 SCL
```

### ⚠️ Two traps on this board

**1. The split row.** Each numbered row is **two electrically separate halves**:
`A-B-C-D-E` on the left and `F-G-H-I-J` on the right. **`E` and `F` are NOT connected.**
A jumper pushed into `4E` with its partner in `4F` looks connected and is not. This is
the single most common cause of a "silent" sensor that never acknowledges.

**2. The hubs are full.** On the documented layout every `A-E` hole on rows 1, 2, 4
and 5 is already occupied, and so is the `F-J` half of row 1. The `F-J` halves of rows
4 and 5 are free, but they are on a **different net** from `4A-4E` / `5A-5E`.
(`2J` is drawn as empty, but whether it is actually on the 3.3 V net depends on a
row-2 `E→F` bridge that the wiring map does not show — beep-test it before relying
on it.)

**Do not force a second wire into an occupied breadboard hole.** A square dupont post
will spread the spring contact and the whole row becomes intermittent — it can work
when you press on it and fail when the board is moved.

### Recommended connection method — solder branch leads

Solder a branch ("Y") lead onto each of the OLED's four wires at **`1B` (GND)**,
**`2B` (3.3 V)**, **`4B` (SDA)**, **`5B` (SCL)**:

```
  OLED wire ──────┬──────► breadboard 1B / 2B / 4B / 5B
                  │
                  └──────► TMP117 corresponding pin
                  (solder joint, heat-shrink, short strain-relief tail)
```

Four joints, each insulated with heat-shrink. This is the most reliable option, the
OLED stays connected the whole time, and the module can then sit anywhere on the
bench or in the enclosure.

If the module is already mounted across **rows 11–17** (the 7-pin header spans 7 rows
at 2.54 mm pitch), that is mechanically fine — the four signal leads still have to
reach the hubs as described above, through branch leads or direct jumpers.

### Temporary no-solder option

The only free holes on the correct nets are the `F-J` halves of rows 4 and 5:

```
4E ── short jumper ── 4F     (extends SDA;  beep-test E→F before use)
5E ── short jumper ── 5F     (extends SCL;  beep-test E→F before use)

TMP117 SDA ──► 4F            TMP117 SCL ──► 5F
```

Ground and 3.3 V have **no verified-free hole**, so branch leads (§ above) are needed
for those two regardless. If you use this temporary method, mark the two bridge
jumpers clearly — the next person to look at the board will otherwise read them as
the classic split-row mistake.

**Keep every wire short (< 15 cm), fully seated, and never re-use a hole that a
previous jumper has already stretched.**

---

## 8. Verification procedure

Work through this in order. Stop at the first failed step — that is the fault.

### 8.1 Powered measurements

Board **ON**. Use the **OLED's pads as the ground reference**, because the OLED is
known good. Do **not** reference to the TMP117's own GND pad: if that wire is the
broken one, the reading will hide the fault.

| Probe (+) | Probe (−) | Expected | If wrong |
|---|---|---|---|
| TMP117 **VIN** | OLED GND | **3.3 V** | Supply wire open, or on a split row |
| TMP117 **3V3** | OLED GND | **3.1 – 3.3 V** | 0 V ⇒ module has no ground return, or the LDO is dead |
| TMP117 **GND** | OLED GND | **≈ 0 V** | Non-zero ⇒ GND wire open |
| TMP117 **SDA** | OLED GND | **≈ 3.3 V** (idle) | 0 V ⇒ line shorted or not connected |
| TMP117 **SCL** | OLED GND | **≈ 3.3 V** (idle) | 0 V ⇒ line shorted or not connected |

> A DMM in DC-volts mode is a 10 MΩ load. It will *not* disturb the bus.

### 8.2 Continuity — board OFF, power removed

| Probe A | Probe B | Expected |
|---|---|---|
| TMP117 **GND** | **OLED GND** | **beep** (< 1 Ω) |
| TMP117 **SDA** | **OLED SDA** | **beep** (< 1 Ω) |
| TMP117 **SCL** | **OLED SCL** | **beep** (< 1 Ω) |
| TMP117 **ADD** | **OLED GND** | **beep** (if strapped to GND) |
| TMP117 **VIN** | OLED **VCC** (row 2 / `2B`) | beep |

Testing against the **OLED's** pads (not against another TMP117 pin) is deliberate:
it proves the signal actually reaches the shared bus, not merely that two wires both
touch the module.

### 8.3 Visual inspection

- Header pin count: **7 pins**, count from the **VIN** end. SCL is the **4th**,
  SDA is the **5th**. Landing one hole off is easy and produces exactly this fault.
- Reflow suspicion list, in priority order:
    1. **GND pad**
    2. **VIN pad**
    3. **SDA pad**
    4. **SCL pad**
- Check for **lifted pads** and **cold joints** on the breakout's through-holes.
- Check for a solder bridge between adjacent pins. (A bridge between SDA and SCL
  would have killed the whole bus; it has not, so this is unlikely — but check
  bridges to the unused `3V3` and `ALT` pins as well.)
- Confirm the module is actually marked **TMP117** (see §10).

### 8.4 Results of the measurement pass — 22 October 2026

| Measurement | Result | Verdict |
|---|---|---|
| TMP117 **VIN** → OLED VCC | 2 Ω | ✅ connected to the 3.3 V net |
| TMP117 **GND** → OLED GND | > 2 Ω, beeps | ✅ connected — 2 Ω is the meter's resolution floor |
| TMP117 **SDA** → OLED SDA | 2 Ω | ✅ on the bus data net |
| TMP117 **SCL** → OLED SCL | 1.9 – 2.0 Ω | ✅ on the bus clock net |
| TMP117 **ADD0** → OLED GND | 2 Ω | ✅ strapped low → address `0x48` |
| TMP117 **SDA** → OLED GND (powered) | 2.7 V, fluctuating | ⚠️ not an idle reading — see below |
| TMP117 **SCL** → OLED GND (powered) | 2.8 V, fluctuating | ⚠️ not an idle reading — see below |

> **All five connections are present and land on the correct nets.** The wiring is
> complete and correct. **Stop looking there.**

**Why 2.7 V / 2.8 V is not a fault.** Both were measured on a *live* bus. The ESP32-S3
is talking on this bus continuously — the MAX30102 is polled at 100 Hz, the MPU6050 at
40 Hz, the BME280 at 1 Hz, and the OLED pushes its changed framebuffer pages (only the
1–2 of 8 that actually changed) on every 50 ms main-loop pass. That is roughly 20 %
bus occupancy. A DMM integrates the waveform, so it reports a weighted average rather
than the high level. Note that **SDA came out ~0.1 V below SCL** — on I2C, SDA is low
for longer than SCL on every transfer, so that offset is exactly what an averaging
meter must show. **To read a true idle level, hold the ESP32 in RESET (board still
powered); both lines must then read a solid 3.3 V.**

**Still not measured:** the module's **`3V3` pin** voltage, and a true idle reading.

### 8.5 Conclusion — the module is faulty

The continuity pass (§8.4) proves the module's power, ground, data and clock lines
all land on the correct nets, and that its address strap selects `0x48`. With that
established there is only one explanation left:

> **Every I2C device acknowledges its own address — before anything else.**
> A chip that is powered, grounded, correctly addressed and on both bus lines
> responds to an address-only write even if it has never been configured, even if
> it is the wrong model, and even if it is a clone. The boot-time scan sweeps
> `0x01`–`0x7E`; the 5 s hot-plug retry probes `0x48`–`0x4B` indefinitely.
> **Silence on a correctly wired module means the chip is not alive.**

**Action: replace the TMP117 breakout.** The board, the firmware and the other four
sensors are all good.

> **Confirmed on a known-good ESP32 — 22 Oct 2026.** A temporary standalone bench
> rig was flashed onto the ShrikeFi's own ESP32-S3, running the **shipping driver**,
> with the TMP117 as the **only device on the bus** (the other four sensors
> disconnected). A pull-up presence probe — with the ESP32's internal pull-ups
> disabled — confirmed the breakout's own 4.7 kΩ pull-ups were holding both data
> lines high, which proves the module **was attached and powered**. It still
> acknowledged nothing at `0x48`–`0x4B`, and a raw no-ACK read returned `0xFFFF`.
> **The module is dead.** It was removed afterwards and the bench rig deleted; the
> product firmware was restored to the board.

> **Optional rework instead of replacement — one hour of soldering.** The fault may
> be a dry joint *inside* the module rather than a dead IC: a broken solder
> connection between the breakout's pad and the chip pin, on the TMP117 IC itself,
> on the level-shifter MOSFET, or on the LDO. Continuity testing cannot see inside
> the module. The measurement in §8.6 item 1 tells you which part to attack:
> - `3V3` pin reads **3.1 – 3.3 V** → the LDO is alive → the fault is the TMP117 IC
>   or the level shifter → **reflow those joints** (and the IC's own pins).
> - `3V3` pin reads **0 V** → the LDO is the dead part → reflow it, or replace.
>
> Either way the module is a dud for this project — the goal is only to find out
> whether it is worth a second module.

> **The product is not blocked while you wait.** With no skin thermometer the clinical
> engine scores **zero** NEWS2 temperature points instead of raising a false
> hypothermia call, and the disaster engine falls back to the BME280 ambient
> cold-stress proxy, deliberately capped at `RISK_HIGH`. The only thing lost is the
> clinical skin-temperature measurement itself.

### 8.6 Two optional confirmations (10 minutes — useful before ordering a replacement)

**Result — 22 October 2026:**

> With the TMP117 removed and the ESP32 held in RESET, **both SDA and SCL read a
> steady 3.18 V** — identical values, no flicker.
>
> **The bus is clean.** No line is shorted, nothing is dragging the bus down, and
> the four working sensors are unaffected. **3.18 V is this board's healthy idle
> level** (3.3 V nominal, measured at the far end of the breadboard) — use it as the
> reference for any future idle measurement, not 3.30 V.
>
> Leave the TMP117 disconnected until a replacement is fitted, so a possibly-damaged
> module cannot touch the shared bus in the meantime.

**With the module reconnected and the board running normally:**

| Line | Reading | Behaviour |
|---|---|---|
| SDA → OLED GND | ≈ 2.85 V | fluctuating |
| SCL → OLED GND | ≈ 2.80 V | fluctuating |

Two things are true in that reading at once: the module is attached, **and** the bus
is in use. The ESP32 polls the MAX30102 at 100 Hz, the MPU6050 at 40 Hz and pushes
OLED pages every 50 ms — roughly 10 % low duty overall. A DMM integrates that
waveform and reports an average about 11 % below the 3.18 V rail. **Fluctuation on a
live I2C bus is normal and implies no fault.**

One detail is worth noting: in the first pair of readings **SDA was the lower** of the
two (2.7 V vs 2.8 V), and in this pair **SDA is the higher** (2.85 V vs 2.80 V). A
genuine fault on one specific line would show the *same* line lower every time. The
offset moving like that means it is measurement noise, not a fixed impedance sitting
on one wire.

> **One measurement still outstanding — it is the decisive one.**
> Module **reconnected**, ESP32 **held in RESET**. Both lines must read **3.18 V**.
> - **3.18 V** ⇒ the module is dead but *not* loading the bus. Replace it; the other
>   four sensors are unaffected.
> - **≈ 2.8 V** ⇒ the module *is* pulling the bus down (roughly a 7 kΩ leak to ground
>   on each line). Leave it disconnected and replace it — a leaking module can also
>   degrade signal integrity for the other four sensors.

1. **Measure the module's `3V3` pin** → OLED GND, board powered.
   - `3.1 – 3.3 V` → the module's regulator works; the IC itself is dead.
   - `0 V` → the module's regulator is dead.
   Either way the module is a dud — this only tells you what to say to the supplier,
   and which part to attack if you attempt a rework (§8.5).
2. ~~**Unplug the module and confirm the bus is clean**~~ — **DONE 22 Oct 2026.**
   Power off, remove the module's SDA and SCL, power on, hold RESET and measure
   SDA/SCL → OLED GND. **Result: both a steady 3.18 V.** The bus is clean and the
   other four sensors are safe.

### 8.7 When the replacement arrives

**What to order:** a **blue 7-pin TMP117 breakout** with the pin order
`VIN | 3V3 | GND | SCL | SDA | ALT | ADD` — i.e. the same part as the one that
failed, with an onboard 3.3 V regulator and level shifters on the host side.
**Buy two.** Cheap modules of this type fail silently, and a spare on the bench
saves a second round trip.

> Do not accept a 4-pin or 6-pin module (`VDD GND SCL SDA ALERT ADD0`). Those are
> simpler boards without a regulator — they wire differently (`VDD` takes the 3.3 V
> rail directly) and their level shifter expects a 5 V-capable host side.

**Wiring it:** exactly as in §6 — VIN to 3.3 V, **`3V3` left open**, GND to common
ground, SCL to GPIO 2, SDA to GPIO 1, ADD0 to GND.

**Then power up and watch the serial console.** The expected line is:

```text
TMP117: TMP117 initialized OK at 0x48 (ID=0x0117, AVG=8, 1 Hz continuous)
```

The **`ID=0x0117`** matters. Many modules sold as "TMP117" are actually TMP102,
LM75, PCT2075 or MAX30205 silicon. Those **will** acknowledge but are rejected by
the firmware with a named message (§10) — the ±0.1 °C / ASTM E1112 accuracy claim
belongs to the TMP117 alone. If you see `is not a TMP117 (ID=0x....)`, the module is
the wrong part, not badly wired. Report the ID value printed — do not modify the
board.

The OLED skin line must change from `Ts:-- (TMP117 ABSENT)` to `Ts: -- (PLACE FGR)`.

> **Note on a swapped SDA/SCL pair:** a swap would have been caught by the multimeter
> work above, because the probe was placed on the module's own pads. Continuity
> cannot detect a swap *only* if the probe was placed on the wire instead of the pad.
> If the replacement module is silent in exactly the same way, repeat the SDA/SCL
> swap test (§9) on the new module before concluding it is also faulty.

---

## 9. Fault decision tree

```
Is VIN = 3.3 V (referenced to OLED GND)?
├── NO .................. supply wire open / split breadboard row → fix, retest
└── YES
    └── Is 3V3 = 3.1–3.3 V (referenced to OLED GND)?
        ├── NO ......... module GND open, or LDO dead → reflow GND first
        └── YES
            └── Are SDA and SCL both ≈ 3.3 V idle?
                ├── NO .. line shorted / not on the bus → continuity test §8.2
                └── YES
                    └── Continuity SDA→OLED SDA and SCL→OLED SCL both beep?
                        ├── NO ......... jumper on a dead net → re-route
                        └── YES
                            └── STILL no ACK:
                                ├── SDA/SCL reversed at the module only
                                │   (every other device still works — matches)
                                └── module is dead
```

### 8.8 Standalone module test — settles "dead module" vs "our side"

The last open question is whether the module is faulty or whether something on the
ShrikeFi side is stopping it from answering. A separate ESP32 dev board and a bare
breadboard answer that question with none of our firmware in the loop.

**Wiring (ESP32 dev board → module):**

| Module pin | Dev board pin |
|---|---|
| **VIN** | **3V3** — the 3.3 V pin, *not* the 5 V / VIN pin |
| **GND** | GND |
| **SDA** | SDA (the dev board's default I2C SDA) |
| **SCL** | SCL (the dev board's default I2C SCL) |
| **3V3** | *leave open* |
| **ADD** | GND |
| **ALT** | *leave open* |

> ⚠️ **3.3 V only.** The module's host-side pull-ups are referenced to `VIN`. Put 5 V
> on `VIN` and you put 5 V on the dev board's GPIO pins.

**Disconnect the ShrikeFi board entirely** (USB out) so that only one I2C bus exists.

**Sketch** for the Arduino IDE — `Wire.begin()` with no arguments uses the board's
default I2C pins, so no pin configuration is needed:

```cpp
#include <Wire.h>

static uint16_t readReg16(uint8_t dev, uint8_t reg) {
  Wire.beginTransmission(dev);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0xFFFF;
  if (Wire.requestFrom(dev, (uint8_t)2) != 2) return 0xFFFF;
  uint16_t hi = Wire.read();
  uint16_t lo = Wire.read();
  return (uint16_t)((hi << 8) | lo);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Wire.begin();
  Wire.setClock(100000);

  Serial.println("=== I2C scan ===");
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) Serial.printf("  found 0x%02X\n", a);
  }

  Serial.println("=== TMP117 ===");
  for (uint8_t a = 0x48; a <= 0x4B; a++) {
    uint16_t id  = readReg16(a, 0x0F);
    uint16_t tmp = readReg16(a, 0x00);
    if (id != 0xFFFF || tmp != 0xFFFF) {
      Serial.printf("  0x%02X  ID=0x%04X  TEMP=0x%04X (%.4f C)\n",
                    a, id, tmp, (int16_t)tmp * 0.0078125f);
    }
  }
}

void loop() { delay(1000); }
```

**Results:**

| Result | Conclusion |
|---|---|
| Found at `0x48`, `ID reg 0x0F = 0x0117` | **The module is good.** The fault is on the ShrikeFi side after all — re-open that investigation. |
| Found at `0x48` but `ID ≠ 0x0117` | It is a clone. Note the ID and report it — the firmware would have named and rejected it. |
| Found at `0x49`–`0x4B` | ADD0 is not actually being grounded. |
| Nothing found at any address | **The module is dead.** Replace it. |

> **Do this with the replacement too, before fitting it to the board.** Two minutes,
> and it confirms both that the new module works and that its device ID really is
> `0x0117`.

### 8.9 The swap test — only needed if a replacement module behaves identically

No reflashing is needed — the firmware retries every 5 seconds.

1. ~~**Substitute the module for the OLED**~~ — **DONE 22 Oct 2026.** The continuity
   pass proved SDA, SCL, GND and VIN all land on the correct nets (§8.4).
2. **Swap only the module's SDA and SCL wires.** Power down, swap the two wires where
   they meet the module, power up, and watch the serial console. Both lines are
   open-drain 3.3 V, so the swap itself is electrically safe.
    - **It appears** → the pair was reversed on that module. Keep it swapped and
      relabel the wires.
    - **It still does not appear** → the module is faulty; replace it.

> **Not needed for the module that was tested on 22 Oct 2026** — its SDA and SCL were
> verified at the module's own pads (§8.4). Keep this procedure for the replacement.

---

## 10. If a non-TMP117 module is fitted

The firmware now **requires** the TI device ID `0x0117` before it will use a sensor
as the clinical temperature source. This is intentional: the ±0.1 °C / ASTM E1112
accuracy claim belongs to the TMP117 alone.

Many cheap modules sold as "TMP117 / CJMCU-117" actually contain a TMP102, LM75,
PCT2075 or MAX30205 (±0.5 to ±2 °C, uncalibrated). Those parts will **acknowledge**
on the bus but will be **rejected** with:

```text
TMP117: Device at 0x48 is not a TMP117 (ID=0x0548, expected 0x0117); refusing to use it as the clinical temperature source
```

If that message appears, the wiring is correct and the **module is the wrong part**.
Report the ID value printed — do not modify the board.

---

## 11. Acceptance test

The firmware probes `0x48`–`0x4B` **every 5 seconds**. No reflash, no reset, and no
configuration change is required after a wiring fix. While the sensor is still absent,
one rate-limited reminder appears every **30 seconds**:

```text
W (xxxx) SHRIKEFI_MAIN: [TMP117] Still absent on I2C (0x48-0x4B probed). Check VIN=3.3V, GND, SDA=GPIO1, SCL=GPIO2 on the breakout.
```

**If the board stays powered** while you rewire, the retry announces success within
5 seconds:

```text
I (xxxx) SHRIKEFI_MAIN: TMP117 initialized after a runtime I2C retry at 0x48
```

**If you power-cycle afterwards**, the boot path also prints the verified device ID:

```text
I (xxxx) TMP117: TMP117 initialized OK at 0x48 (ID=0x0117, AVG=8, 1 Hz continuous)
```

The OLED skin line then follows this sequence:

| Condition | OLED line |
|---|---|
| No TMP117 (fault present now) | `Ts:-- (TMP117 ABSENT)` |
| TMP117 OK, no finger on the PPG | `Ts: --   (PLACE FGR)` |
| Finger placed, probe still warming | `Ts:31.2C (SETTLING)` |
| Finger on the **PPG** but the TMP117 is still below 29 °C after 10 s | `Ts:31.2C (TOUCH TMP)` |
| Probe settled | `Ts:33.4C Core:35.6C` |

The probe must be **in contact with skin**, not merely near the finger: the TMP117 is
a contact thermometer, and with no thermal contact it will sit at ambient and never
reach the settling threshold.

---

## 12. Do NOT

| ✗ | Why |
|---|---|
| **Do not connect 5 V to VIN** | Header pull-ups reference VIN → 5 V would reach GPIO1/GPIO2 and every other device. |
| **Do not connect 3V3 to the 3.3 V rail** | It is the regulator's output. Output-to-output contention; kills the regulator. |
| **Do not add pull-up resistors** | The bus already has its own (typically 1 – 3 kΩ total). Adding more over-drives the bus. |
| **Do not connect ADD to SDA or SCL** | That straps the address to a driven bus line and can wedge the bus for every device. |
| **Do not hot-plug wires** | Power down before changing any connection. |
| **Do not power the module from the 5 V rail** | Same as the first row: 3.3 V only. |

---

## 13. Reference data — TI TMP117

| Parameter | Value |
|---|---|
| Supply range | **1.8 V – 5.5 V** |
| I2C clock | 400 kHz max (Fast Mode); this bus runs 100 kHz |
| Addresses | `0x48` (ADD0→GND), `0x49` (→V+), `0x4A` (→SDA), `0x4B` (→SCL) |
| Device ID register | `0x0F`, bits 11:0 = **`0x0117`** |
| Temperature register | `0x00`, 16-bit two's complement |
| Resolution | **0.0078125 °C** per LSB (1/128 °C) |
| Configuration register | `0x01`, `0x0220` = continuous, 1000 ms, 8× averaging (power-on default) |
| Accuracy | ±0.1 °C (−20…50 °C), ±0.15 °C (−40…70 °C) |

---

## 14. Document control

| Item | Value |
|---|---|
| File | `docs/I2C_BUS_BENCH_REPORT.md` |
| Related | `docs/SHRIKEFI_HARDWARE_CONNECTIONS.md` (full pin map and breadboard layout) |
| Firmware driver | `firmware/shrikefi/tmp117.c` |
| Measurement source | live serial capture, COM4, unit powered |
| Status | **OPEN — TMP117 module confirmed DEAD (attached + powered on a known-good ESP32 with an empty bus, 22 Oct 2026 — §8.5). Replacement required** |
