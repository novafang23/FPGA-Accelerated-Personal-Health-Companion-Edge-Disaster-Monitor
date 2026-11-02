# VALOR / ShrikeFi — Wearable PCB Design Guide (Rev A)

**Status: DESIGN INTENT — nothing in this document has been fabricated, assembled or measured.**
No Gerbers exist for this board. No Rev A prototype has been built. Treat every number here
as a specification to be met, not a result.

| Field | Value |
|---|---|
| Project | SIH26181 — VALOR (Vital and Atmospheric Logic for Offline Rescue) |
| Target hardware | ESP32-S3 (Wi-Fi/BLE MCU) + Renesas ForgeFPGA SLG47910C |
| EDA | EasyEDA Standard / Pro |
| Fabrication | JLCPCB, 4-layer, ENIG, SMT assembly |
| Board outline | **45 mm × 40 mm** (see §2 — the 38 × 36 mm figure in the first draft does not close) |
| Intended wearer | Wrist or sternum strap; bottom face against skin, top face outward |
| Regulatory status | **Not a medical device.** Research and competition prototype only. |

> **What changed from the first draft.** The initial version of this guide contained
> electrical errors that would have destroyed the board on first power-up — the BME280
> supply pins were reversed (3.3 V onto GND), the MAX30102 was powered from 3.3 V when its
> core rail is 1.8 V, and three ICs had no ground connection at all. Those are corrected
> below, with the reasoning shown so the correction can be checked rather than trusted.
> §12 lists the items that must still be verified before an order is placed.

---

## Table of Contents

1. [Architecture and wearable partitioning](#1-architecture-and-wearable-partitioning)
2. [Area and volume budget](#2-area-and-volume-budget)
3. [EasyEDA primer](#3-easyeda-primer)
4. [Bill of materials](#4-bill-of-materials)
5. [Schematic, subsystem by subsystem](#5-schematic-subsystem-by-subsystem)
6. [Board setup, stackup and design rules](#6-board-setup-stackup-and-design-rules)
7. [Placement](#7-placement)
8. [Routing](#8-routing)
9. [Manufacturing reality check](#9-manufacturing-reality-check)
10. [Assembly and bench bring-up](#10-assembly-and-bench-bring-up)
11. [What this document does not establish](#11-what-this-document-does-not-establish)
12. [Pre-order verification checklist](#12-pre-order-verification-checklist)

---

## 1. Architecture and wearable partitioning

The compute split is unchanged from the bench prototype and is the reason the wearable is
viable at all:

- **ForgeFPGA (1120 5-input LUTs, 50 MHz internal oscillator):** 8-tap running-sum
  moving average, 4-state systolic peak detector with refractory blanking, and a crest
  timestamp register. It removes operating-system scheduling jitter from beat detection.
  It does **not** deliver 20 ns IBI resolution — see the note below.
- **ESP32-S3 Core 0:** streams one 8-bit optical sample per 10 ms over SPI2, applies the
  signal-quality and motion gates, and timestamps returned beats.
- **ESP32-S3 Core 1:** polls the environmental sensors, runs the triage model and rules,
  drives the OLED, and serves the offline dashboard over the SoftAP.

> **Resolution, stated correctly.** The FPGA clock period is 20 ns and the IBI counter's
> least-significant bit is therefore 20 ns. That is *not* the measurement resolution. The
> optical front end delivers one sample every 10 ms, so a crest cannot be located more
> precisely than **10 ms**. Quote 10 ms. The FPGA's contribution is determinism — the same
> 10 ms quantisation on every beat, with no jitter added on top.

### Partitioning by face

| Face | Components | Why |
|---|---|---|
| **Bottom (skin)** | MAX30102 optical sensor, TMP117 temperature sensor, decoupling, skin-contact copper | Must contact skin with no air gap; minimises ambient light ingress; thermal contact |
| **Top (outward)** | ESP32-S3 module, ForgeFPGA, BME280, MPU6050, USB-C, power management, OLED, button, buzzer | Unobstructed RF, display and controls face the wearer or rescuer, environmental sensor breathes room air |

> **Voltage rule.** ESP32-S3 and ForgeFPGA I/O are 3.3 V only. Never apply 5 V to a GPIO,
> SPI or I2C line. 5 V exists only on USB VBUS and the particulate sensor's supply pin.

---

## 2. Area and volume budget

The first draft specified 38 mm × 36 mm. That outline does not accommodate the chosen
parts, and it is cheaper to discover that here than after a fabrication order.

| Item | Footprint | Area |
|---|---|---|
| ESP32-S3-WROOM-1 module | 25.5 × 18.0 mm | 459 mm² |
| Antenna keep-out (no copper, any layer) | ≈ 15 × 18 mm | 270 mm² |
| 0.96" OLED module | ≈ 27.3 × 27.0 mm | 737 mm² |
| USB-C receptacle | 8.94 × 7.35 mm | 66 mm² |
| Power management (3 ICs + passives) | ≈ 12 × 10 mm | 120 mm² |
| Sensors + support | ≈ 15 × 12 mm | 180 mm² |

Top-face budget for a 45 × 40 mm board is 1800 mm². Module, keep-out, USB and power
consume 915 mm², leaving 885 mm² — enough for the display (737 mm²) with ~148 mm² spare
for the button, buzzer and passives.

On a 38 × 36 mm board (1368 mm²) the same items leave 453 mm² against a 737 mm² display.
**It does not fit.** Three ways out:

1. **45 × 40 mm board (recommended).** Still a small wearable. Everything fits on two faces.
2. **38 × 36 mm with the display on a flying 4-pin FPC lead**, mounted in the enclosure
   rather than on the board. Keeps the board tiny; adds an assembly step.
3. **38 × 36 mm with a 0.49" or 0.66" panel.** Readable enough for two lines of triage data,
   not for a dashboard.

This guide assumes option 1.

**Thickness budget** — the first draft claimed an 11 mm chassis. That does not close either:

| Layer | Thickness |
|---|---|
| PCB | 1.2 mm |
| LiPo cell | 5.0 mm |
| OLED module | 2.6 mm |
| Enclosure walls (2 × 1.5 mm) | 3.0 mm |
| Clearance | 0.7 mm |
| **Total** | **12.5 mm** |

To hit 11 mm, use a 4.0 mm cell and 1.2 mm walls: 1.2 + 4.0 + 2.6 + 2.4 + 0.8 = 11.0 mm.
Pick one target and design to it — do not quote both.

---

## 3. EasyEDA primer

Two stages: **schematic** (logical connections, verified by ERC) then **PCB layout**
(physical copper, verified by DRC). Symbols and footprints are pulled from the LCSC
library by exact part number, which is the whole reason to use this toolchain.

- `Shift + F` opens the library; select the JLCPCB/LCSC tab and paste the C-number.
- Never search by keyword ("resistor", "ESP32"). Keyword results are user-contributed and
  frequently have wrong pin numbering.
- `W` wire/track, `V` via, `N` net label, `R` rotate, `X` no-connect flag, `E` copper area.

> **Footprint trap.** When you place an IC, compare the footprint's pad-1 marker against the
> datasheet's pin-1 corner *before* wiring. A mirrored footprint is the most common
> board-killing mistake, and the schematic will still pass ERC.

---

## 4. Bill of materials

Reference designators are unique across the whole board. The first draft reused `C1` for
two different capacitors and left eleven resistors unlabelled, which would have broken the
assembly BOM.

### 4.1 Silicon

| Ref | Part | Package | LCSC | Face | Notes |
|---|---|---|---|---|---|
| U1 | ESP32-S3-WROOM-1-N8R2 | SMD-38 | C2913200 | Top | 3.3 V, 8 MB flash, 2 MB PSRAM |
| U2 | Renesas **SLG47910C** | **WLCSP-20** | verify | Top | 3.3 V, 1120 LUT5s. Package is chip-scale, 0.4 mm pitch — see §9 |
| U3 | MAX30102EFD+T | LGA-14 (OESIP) | C89146 | **Bottom** | **Two rails: 1.8 V core + 3.3 V LED** |
| U4 | TMP117AIDRVR | WSON-6 | C2682622 | **Bottom** | ±0.1 °C, address 0x48 |
| U5 | BME280 | LGA-8 | C92489 | Top | Address 0x76, needs vent |
| U6 | MPU-6050 | QFN-24 | C24112 | Top | Address 0x68 |
| U7 | AP2112K-3.3 | SOT-23-5 | C52994 | Top | Main 3.3 V rail |
| U8 | AP2112K-1.8 | SOT-23-5 | verify | Top | **1.8 V rail for the PPG sensor only** |
| U9 | MCP73831T-2ACI/OT | SOT-23-5 | C14881 | Top | LiPo charger, 212 mA |
| U10 | DW01A | SOT-23-6 | C347326 | Top | Protection controller |
| U11 | FS8205A | TSSOP-8 | C32254 | Top | Protection FET pair |
| U12 | PCA9306 | VSSOP-8 | verify | Top | **I2C level translator, 3.3 V ↔ 1.8 V** |
| Q1 | 2N7002 | SOT-23 | C20917 | Top | Buzzer low-side switch |
| Q2 | 2N7002 | SOT-23 | C20917 | Top | Inverts the FPGA power-gate drive |
| Q3 | AO3401A (P-FET) | SOT-23 | verify | Top | High-side switch for FPGA supply |

### 4.2 Connectors, indicators, electromechanical

| Ref | Part | LCSC | Notes |
|---|---|---|---|
| J1 | USB-C receptacle, 16-pin | C165948 | Power in + native USB |
| J2 | JST-SH 1.0 mm, 2-pin | C145956 | Single-cell LiPo |
| J3 | FPC 0.5 mm, **4-pin** | verify | I2C OLED **module** — see §9 note on bare panels |
| J4 | JST-SH 1.0 mm, 4-pin | C145958 | Particulate sensor |
| J5 | JST-GH 1.25 mm, 4-pin | verify | GNSS module |
| SW1 | Tactile 3.0 × 4.0 mm | C318884 | SOS cancel |
| SW2 | Tactile 3.0 × 4.0 mm | C318884 | Reset |
| BZ1 | Active piezo, 5.0 × 5.0 mm | C963955 | Alarm |
| D1 | Red LED 0603 | verify | Charge status |
| D2 | Green LED 0603 | verify | 3.3 V present |
| D3 | Blue LED 0603 | verify | Beat indicator |

### 4.3 Passives

| Ref | Value | Package | Purpose |
|---|---|---|---|
| R1, R2 | 5.1 kΩ | 0402 | USB-C CC1/CC2 pull-downs (mandatory) |
| R3, R4 | 3.3 kΩ | 0402 | I2C pull-ups, 3.3 V segment |
| R5 | 4.7 kΩ | 0402 | Charger PROG → 212 mA |
| R6, R7 | 2.2 kΩ | 0402 | I2C pull-ups, 1.8 V segment |
| R8 | 200 kΩ | 0402 | PCA9306 enable pull-up |
| R9 | 100 Ω | 0402 | DW01A supply filter |
| R10, R11, R12 | 1 kΩ | 0402 | D1, D2, D3 series |
| R13 | 100 Ω | 0402 | Q1 gate |
| R14 | 100 kΩ | 0402 | Q1 gate pull-down |
| R15 | 100 Ω | 0402 | Q2 gate |
| R16 | 100 kΩ | 0402 | Q2 gate pull-down |
| R17 | 100 kΩ | 0402 | Q3 gate pull-up to +3V3 |
| R18, R19 | 1 kΩ | 0402 | Series on SPI SCK/MOSI (back-power protection) |
| C1 | 10 µF | 0603 | VBUS bulk |
| C2 | 10 µF | 0603 | Charger VBAT |
| C3 | 1 µF | 0402 | 3.3 V LDO input |
| C4, C5 | 10 µF + 100 nF | 0603 / 0402 | 3.3 V LDO output |
| C6 | 1 µF | 0402 | 1.8 V LDO input |
| C7, C8 | 10 µF + 100 nF | 0603 / 0402 | 1.8 V LDO output |
| C9 | 10 µF | 0603 | MAX30102 **VLED+** bulk |
| C10 | 100 nF | 0402 | MAX30102 VDD |
| C11 | 100 nF | 0402 | TMP117 |
| C12 | 100 nF | 0402 | BME280 |
| C13 | 100 nF | 0402 | MPU6050 VDD |
| C14 | **100 nF** | 0402 | MPU6050 **REGOUT** |
| C15 | **2.2 nF** | 0402 | MPU6050 **CPOUT** |
| C16, C17 | 100 nF | 0402 | ForgeFPGA supply pins (one per VDD pin) |
| C18 | 100 nF | 0402 | DW01A |
| C19 | 100 nF | 0402 | SOS debounce |
| C20 | 1 µF | 0402 | Reset RC |
| C21 | 100 nF | 0402 | PCA9306 |

---

## 5. Schematic, subsystem by subsystem

New project `VALOR_Wearable_PCB`. Create the net classes early: `PWR` (wide), `I2C`,
`SPI`, `SIG`.

> **Verify pin numbers against the footprint before wiring.** The numbers given below are
> from the manufacturer pin tables. Where a part is marked *by name*, read the datasheet
> pin table rather than copying a breakout module — modules routinely reorder and rename
> pins because they contain extra regulators.

### 5.1 Power path

1. **USB-C (J1).** `VBUS` pins together → net `VBUS_5V`. GND and shield → `GND`.
   `CC1` → R1 → GND, `CC2` → R2 → GND. Without both 5.1 kΩ pull-downs a USB-C charger
   supplies 0 V. `D+`/`D-` → `USB_D+`/`USB_D-`.
2. **Charger (U9).** `VIN` ← `VBUS_5V`, C1 to GND. `VBAT` → `BAT+`, C2 to GND.
   `PROG` → R5 → GND (212 mA). `STAT` (open-drain) → D1 → R10 → `VBUS_5V`.
3. **Protection (U10/U11).** `VDD` → `BAT+` via R9, with C18 to `BAT-`. `OD`/`OC` drive
   the two FET gates. The FET pair sits **in series with the cell negative**, so system
   `GND` is downstream of protection while `BAT+` is direct.
4. **3.3 V rail (U7).** `VIN`/`EN` ← `BAT+` (protected), C3 to GND. `VOUT` → `+3V3`, C4 and
   C5 to GND. D2 via R11.
5. **1.8 V rail (U8).** `VIN`/`EN` ← `+3V3`, C6. `VOUT` → `+1V8`, C7 and C8. **This rail
   exists only because the PPG sensor's core is 1.8 V.**

> **No power-path circuit.** The MCP73831 charges the cell but does not pass USB power to
> the system. With no battery fitted the board will not run from USB. Acceptable for a
> prototype; if you want USB-only operation, add a load-sharing switch or an ideal-diode.

### 5.2 MCU (U1)

| GPIO | Net | Destination |
|---|---|---|
| 1 | `I2C_SDA` | 3.3 V I2C segment |
| 2 | `I2C_SCL` | 3.3 V I2C segment |
| 4 | `BUZZER_OUT` | Q1 gate via R13 |
| 5 | `SOS_CANCEL` | SW1 to GND, C19, external 10 kΩ pull-up |
| 8 | `FPGA_EN` | ForgeFPGA enable/reset |
| 9 | `FPGA_PWR_GATE` | Q2 gate |
| 10 | `SPI_SS_N` | ForgeFPGA `PIN_17` via 1 kΩ |
| 11 | `SPI_MOSI` | ForgeFPGA `PIN_18` via R18 |
| 12 | `SPI_SCK` | ForgeFPGA `PIN_16` via R19 |
| 13 | `SPI_MISO` | ForgeFPGA `PIN_19` |
| 14 | `PMS_RX` | Particulate sensor TX |
| 18 | `PMS_TX` | Particulate sensor RX |
| 16 | `GNSS_RX` | GNSS TX |
| 17 | `GNSS_TX` | GNSS RX |
| 19, 20 | `USB_D-`, `USB_D+` | Native USB |
| EN | `MCU_RESET` | SW2, 10 kΩ up, C20 down |

This table matches `firmware/shrikefi/shrikefi_pinmap.h` exactly. If you change a pin, change
the header too or the firmware will drive the wrong net.

> **Strapping pins.** GPIO 0, 3, 45 and 46 are sampled at reset. Leave them unconnected;
> do not attach sensors, pull-ups or test loads.

### 5.3 ForgeFPGA (U2)

- `VDD` pins → `+3V3_FPGA`, one 100 nF per pin (C16, C17).
- `OSC_EN` → `VDD`; the part runs from its internal 50 MHz oscillator.
- `PIN_16` → `SPI_SCK`, `PIN_17` → `SPI_SS_N`, `PIN_18` → `SPI_MOSI`, `PIN_19` → `SPI_MISO`.
- `PIN_7` → R12 → D3 → GND (beat indicator).
- `EN` → `FPGA_EN`.

**Switched supply.** The firmware enters the FPGA boot-mode sequence by controlling power as
well as enable, so the supply must be switchable:

- Q3 (P-FET) source → `+3V3`, drain → `+3V3_FPGA`, gate pulled to `+3V3` by R17.
- Q2 (N-FET) drain → Q3 gate, source → GND, gate ← `FPGA_PWR_GATE` via R15, with R16 to GND.
- GPIO 9 high → Q2 on → Q3 gate low → FPGA powered.

The first draft drove a P-FET gate directly from a 3.3 V GPIO, which cannot work: the gate
can never be pulled below the source, so the device never turns on. The inverter fixes it.

> **Back-powering.** An unpowered FPGA with 3.3 V still present on its SPI inputs will be
> powered through its ESD diodes. R18/R19 limit that current, and firmware should drive SCK
> and MOSI low before removing power.

### 5.4 I2C bus and level translation

The bus is split because two devices need different rails:

- **3.3 V segment:** ESP32-S3, OLED, TMP117, BME280, MPU6050. Pull-ups R3/R4 (3.3 kΩ) to `+3V3`.
- **1.8 V segment:** MAX30102 only. Pull-ups R6/R7 (2.2 kΩ) to `+1V8`.
- **PCA9306 (U12)** bridges them: `VREF1` → `+3V3`, `VREF2` → `+1V8`, `EN` → `+1V8` via R8,
  `SDA1`/`SCL1` on the 3.3 V side, `SDA2`/`SCL2` on the 1.8 V side, C21 across `VREF2`.

Addresses: OLED 0x3C, TMP117 0x48, MAX30102 0x57, MPU6050 0x68, BME280 0x76.

### 5.5 Sensors

**MAX30102 (U3), bottom face — by name, not by breakout pin order.**

| Pin | Connect to |
|---|---|
| `VDD` (core) | **`+1V8`**, with C10 |
| `VLED+` (LED supply) | **`+3V3`**, with C9 |
| `PGND` | `PGND`, single-point to `GND` at the star |
| `GND` | `GND` |
| `SCL`, `SDA` | 1.8 V segment |
| `INT` | Test pad, otherwise unconnected (firmware polls) |

The first draft powered `VDD` from 3.3 V and never connected `VLED+`. Either mistake alone
stops the sensor working; the first also exceeds the core rail's absolute maximum.

**TMP117 (U4), bottom face.** `V+` → `+3V3` with C11. `GND` → `GND`. `SCL`, `SDA` → 3.3 V
segment. `ADD0` → `GND` for 0x48. `ALERT` unconnected. The first draft omitted `GND`
entirely and put `ADD0` on the wrong pin.

**BME280 (U5), top face.** Verified pinout:

| Pin | Name | Connect to |
|---|---|---|
| 1 | `GND` | `GND` |
| 2 | `CSB` | `+3V3` (selects I2C) |
| 3 | `SDI` | `I2C_SDA` |
| 4 | `SCK` | `I2C_SCL` |
| 5 | `SDO` | `GND` (address 0x76) |
| 6 | `VDDIO` | `+3V3` |
| 7 | `GND` | `GND` |
| 8 | `VDD` | `+3V3` with C12 |

The first draft had pin 1 as `VDD` and pin 8 as `VDDIO`, which applies 3.3 V directly to
ground. Pin 1 is ground on this part.

**MPU6050 (U6), top face.** `VDD` → `+3V3` with C13. `VLOGIC` → `+3V3`. `GND` → `GND`.
`AD0` → `GND` (0x68). `REGOUT` → **C14 (100 nF)** to GND. `CPOUT` → **C15 (2.2 nF)** to GND.
`CLKIN` → `GND` (internal oscillator). `FSYNC` → `GND`. `INT` → test pad.

The first draft swapped the two capacitor values and omitted `CPOUT` and `GND`.

### 5.6 Peripherals

- **Particulate sensor (J4):** pin 1 `VBUS_5V`, pin 2 `GND`, pin 3 → `PMS_RX`, pin 4 → `PMS_TX`.
  This device is **5 V only** — never power it from 3.3 V.
- **GNSS (J5):** `VCC` → `+3V3` with 10 µF + 100 nF, `TXD` → `GNSS_RX`, `RXD` → `GNSS_TX`,
  RF pad → U.FL or ceramic antenna with a matching network.
- **Buzzer:** `+` → `+3V3`, `-` → Q1 drain, Q1 source → GND, gate ← `BUZZER_OUT` via R13,
  R14 to GND.
- **SOS button:** SW1 between `SOS_CANCEL` and GND, C19 across it, 10 kΩ pull-up to `+3V3`.

### 5.7 ERC

`Design → Check ERC`. Every intentional no-connect needs an `X` flag. Do not proceed until
ERC reports zero errors — but understand that ERC proves connectivity, **not** correctness.
The BME280 reversal above would pass ERC.

---

## 6. Board setup, stackup and design rules

1. `Design → Convert Schematic to PCB`.
2. **Outline:** 45.0 × 40.0 mm on `BoardOutLine`, 6.0 mm corner fillets, four M2 holes
   (2.2 mm drill, 4.0 mm pad) 3.5 mm in from each corner.
3. **Stackup — 4 layers (required):**

| Layer | Use |
|---|---|
| L1 Top | Components, SPI, I2C, power |
| L2 | Solid GND plane (cut under the antenna — see §7) |
| L3 | Power plane: `+3V3` pours, `+1V8` island, `VBUS_5V` island |
| L4 Bottom | Sensors, skin copper, return traces |

4. **Design rules:** signal 0.152 mm (6 mil), power 0.4–0.6 mm, clearance 0.152 mm, via
   0.6/0.3 mm. JLCPCB's standard 4-layer process meets these comfortably.

---

## 7. Placement

1. **ESP32-S3 module** at the top edge, antenna end facing out. **Keep-out: no copper,
   trace, via or pour on any layer within 15 mm of the antenna.** This means the L2 ground
   plane and L3 power pours must be *cut away* in that region — the stackup description
   above is a simplification, not an absolute.
2. **OLED** top face, centred below the module's keep-out. Aperture and light-blocking
   gasket belong to the enclosure, not the PCB.
3. **ForgeFPGA** within 10–15 mm of the module, SPI traces short and direct.
4. **USB-C** at an edge, snout overhanging 0.8 mm.
5. **Power section** grouped away from the skin sensors, near the battery connector.
6. **MPU6050** at the geometric centre so impact forces are symmetric.
7. **BME280** at the board edge. It must sample ambient air, so the *enclosure* needs a
   vent aperture above it — a PCB hole only helps if the sensor is boxed in. Optionally mill
   a 1.0 × 12.0 mm slot between it and the power section; the slot is a paid fabrication
   option and it weakens the board, so on a 45 × 40 outline simply placing the sensor away
   from the regulators is usually enough.
8. **Bottom face:** MAX30102 centred on the optical window, TMP117 4 mm to one side,
   decoupling within 1 mm.
9. **TMP117 thermal island.** Put a copper island on L4 under the sensor connected only to
   its thermal pad, isolated from the GND pour by ≥0.3 mm clearance, and mill a matching
   cut-out in the L1 copper beneath it. The goal is to couple the sensor to skin, not to the
   board. The firmware allows 12 s to settle for this reason.

---

## 8. Routing

Order of work:

1. **SPI** (1 MHz): four short traces, top layer, no stubs.
2. **I2C**: SDA/SCL as a pair, away from SPI and away from the buck/charger switching node.
3. **1.8 V sensor traces** on the bottom, short, with the translator close to the sensor.
4. **UART** lines to the connectors.
5. **Power**: `VBUS_5V`, `BAT+`, `+3V3`, `+1V8` at 0.4–0.6 mm.
6. **GND**: pours on L1/L4 stitched to L2 with ≥20 vias, denser near the optical sensor and
   the antenna edge of the power section.

Keep the MAX30102's LED return (`PGND`) separate from the signal return until a single star
point under the sensor; LED currents are pulsed and will otherwise appear as noise on the
I2C and PPG signal.

Run DRC until it reports zero errors, then re-run it after every change. DRC does not check
placement, thermal design or RF keep-out — those are your job.

---

## 9. Manufacturing reality check

The earlier draft described this as beginner-friendly one-click assembly. Three problems
with that, in order of severity:

1. **The ForgeFPGA is a chip-scale part.** `docs/MIGRATION.md` records the target as
   SLG47910C in **WLCSP-20** — 0.4 mm pitch, no leads. This is not hand-solderable and
   needs a stencil, reflow and ideally X-ray. The earlier BOM calling it QFN-24 was wrong.
2. **JLCPCB may not stock it.** Renesas FPGA parts are not commodity LCSC stock. Confirm
   availability and assembly support *before* designing around it. A sane Rev A keeps the
   FPGA on the existing ShrikeFi module and puts the new board around the MCU and sensors;
   integrate it in Rev B once you know the part can be placed.
3. **Panel and controller confusion.** The draft specified a 14-pin FPC for a 0.96" OLED.
   A 0.96" OLED *module* is a 4-pin I2C device and needs no controller work. A bare 14-pin
   panel needs an SSD1306 die, a charge pump and its capacitors — a separate design. Use the
   module (J3 = 4-pin) unless you deliberately take on the panel.

**Skin contact.** ENIG is nickel with a thin gold flash. Nickel is a common contact
allergen, and this board puts 3.3 V electronics and a lithium cell against skin. Do not
describe the finish as bio-compatible. Either cover the contact area with a conformal coat
except the sensor apertures, or use a separate gold-plated contact piece. Sweat plus bias
voltage is also a leakage path — keep the skin-contact copper isolated from every net except
the temperature sensor's pad.

**Safety.** The protection FETs are mandatory, not optional. Never charge unattended. Never
power the board from USB while the cell is damaged.

**Order settings:** 4 layers, 1.2 mm, ENIG, impedance control not required, SMT assembly on,
BOM and CPL exported from EasyEDA.

---

## 10. Assembly and bench bring-up

1. **Continuity before power.** Measure `+3V3` to `GND` and `+1V8` to `GND`. Expect > 10 kΩ.
   A beep means a short — find it before applying power. Pay particular attention to U5: a
   reversed supply pin pair is the exact failure mode this guide was corrected for.
2. **Current-limited first power.** 5.0 V at 100 mA into VBUS. Undamaged unprogrammed board:
   15–35 mA. Check `+3V3` = 3.30 V ±0.05 V and `+1V8` = 1.80 V ±0.05 V. D2 lit.
3. **Flash:**

```powershell
. C:\Espressif\frameworks\esp-idf-v5.5.5\export.ps1
cd firmware/shrikefi
idf.py -p COM4 flash monitor
```

4. **Expected console sequence** — this is the authentic sequence from the bench rig:

```text
I (502) SHRIKEFI_LINK: Programming Renesas ForgeFPGA SLG47910 via SPI2...
I (621) SHRIKEFI_LINK: ForgeFPGA SLG47910 configuration COMPLETE! (46408 bytes loaded)
I (631) SHRIKEFI_LINK: ForgeFPGA runtime byte probe: sent 0x55, received 0x00
I (994) I2C_SCAN: Scanning I2C bus (SDA=GPIO1, SCL=GPIO2)...
I (1004) I2C_SCAN:  -> Found device at 0x3C (SSD1306 OLED 128x64)
I (1014) I2C_SCAN:  -> Found device at 0x48 (TMP117 Clinical Skin Temp)
I (1024) I2C_SCAN:  -> Found device at 0x57 (MAX30102 PPG Optical Sensor)
I (1034) I2C_SCAN:  -> Found device at 0x68 (MPU6050 6-Axis Motion IMU)
I (1044) I2C_SCAN:  -> Found device at 0x76 (BME280 Env Temp/Humidity/Pressure)
I (1054) I2C_SCAN: Scan complete: 5 device(s) found! All peripherals responding.
I (1102) SOFTAP: Wi-Fi Access Point active: SSID='VALOR-0081', IP=192.168.4.1
```

> `received 0x00` is the **expected** handshake result, not a failure. See
> `docs/SHRIKEFI_LINK_PROTOCOL.md`. The proof the FPGA is alive is that it drives MISO at
> all rather than leaving it floating.

5. **Live check.** Cover the optical window with a fingertip: D3 should flash with each
   beat. Heart rate settles in about 15 s. **HRV is not valid at 15 s** — RMSSD needs on the
   order of three minutes of clean signal, and the bench measurements took that long. Do not
   demo HRV from a 15-second capture.

---

## 11. What this document does not establish

- No board has been fabricated. These are specifications, not results.
- No power measurement, thermal measurement or EMC pre-compliance exists.
- Sensor accuracy on this board is unmeasured. The PPG and temperature figures elsewhere in
  the repo come from the bench rig, not from a wearable.
- The ForgeFPGA package, stock and assembly path are unconfirmed (§9).
- Nothing here is a medical claim, and the board is not a medical device.

---

## 12. Pre-order verification checklist

Do not place an order until each line is answered with a datasheet or a stock page, not a
memory:

- [ ] BME280 pin 1 confirmed **GND** and pin 8 **VDD** in the LCSC footprint
- [ ] MAX30102 `VDD` on `+1V8` and `VLED+` on `+3V3`, both connected
- [ ] MAX30102 I2C pins confirmed 1.8 V-only, and the translator present
- [ ] TMP117 `GND` connected, `ADD0` on the correct pin
- [ ] MPU6050 `GND` and `CPOUT` connected, 2.2 nF on CPOUT and 100 nF on REGOUT
- [ ] ForgeFPGA package, pin numbering and LCSC availability confirmed
- [ ] P-FET gate drive includes an inverter and a pull-up
- [ ] Every IC's decoupling present and within 1 mm of its supply pin
- [ ] Reference designators unique across the whole BOM
- [ ] ERC zero errors, DRC zero errors
- [ ] Antenna keep-out clear on all four layers
- [ ] No 3.3 V net within 0.3 mm of the skin-contact copper

---

*SIH26181 · Project VALOR · Design intent document, Rev A · Not built, not verified, not a medical device.*
