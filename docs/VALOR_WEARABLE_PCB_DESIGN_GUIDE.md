# VALOR Wearable — Rev A Carrier Board (Chest / Sternum Patch)

**Status: DESIGN INTENT. Nothing here has been fabricated, assembled or measured.**
No Gerbers exist. No Rev A prototype has been built. Every number is a
specification to be met, not a result.

| Field | Value |
|---|---|
| Project | SIH26181 — VALOR |
| Revision | **Rev A** — carrier board around the ShrikeFi module |
| Form factor | **Chest / sternum patch**, 45 × 50 mm, worn under or over clothing |
| Outline | **45.0 × 50.0 mm** (see §2 — an earlier 45 × 40 figure was wrong) |
| Layers | 4 (ENIG, 1.2 mm) |
| MCU + FPGA | **Vicharak ShrikeFi module** (ESP32-S3 + SLG47910C), socketed/header |
| Display | 0.96" SSD1306 I2C **module** |
| Regulatory | **Not a medical device.** Research / competition prototype |

> **What changed from the previous revision of this document.** The previous draft
> was an "ultra-compact 38 × 36 mm wearable" that (a) did not close geometrically,
> and (b) then "closed" at 45 × 40 mm using an area-only calculation. Area is not
> the constraint: the module occupies 18 mm of board height and the display needs
> 27 mm, and they cannot overlap because the display would sit over the antenna
> keep-out. The outline is now **45 × 50 mm**. The electrical corrections from the
> earlier audit (BME280 supply pins, MAX30102 dual rails, MPU6050 CPOUT/REGOUT,
> the P-FET gate inverter, unique designators) remain in force.

---

## Table of Contents

1. [Rev A vs Rev B](#1-rev-a-vs-rev-b)
2. [Packaging budget and the outline](#2-packaging-budget-and-the-outline)
3. [Bill of materials](#3-bill-of-materials)
4. [Schematic by subsystem](#4-schematic-by-subsystem)
5. [Stackup and design rules](#5-stackup-and-design-rules)
6. [Placement](#6-placement)
7. [Optics and the skin interface](#7-optics-and-the-skin-interface)
8. [Power path, charging and battery](#8-power-path-charging-and-battery)
9. [Ingress, mechanics and strap](#9-ingress-mechanics-and-strap)
10. [Routing](#10-routing)
11. [Manufacturing and assembly](#11-manufacturing-and-assembly)
12. [Bring-up](#12-bring-up)
13. [What this document does not establish](#13-what-this-document-does-not-establish)
14. [Pre-order verification checklist](#14-pre-order-verification-checklist)

---

## 1. Rev A vs Rev B

Doing the FPGA in Rev A is the single riskiest choice available: the SLG47910C is
**WLCSP-20 at 0.4 mm pitch**, needs a stencil, reflow and ideally X-ray, and its
JLCPCB stock is unconfirmed. One bad joint kills the board.

| | **Rev A (this document)** | Rev B (later) |
|---|---|---|
| Compute | ShrikeFi module on headers | FPGA as a bare IC on-board |
| Board | Carrier: sensors, power, display, connectors | Single integrated board |
| Goal | Prove the **wearable form factor** and the optics | Miniaturise, one board |
| Risk | Low | High (assembly + sourcing) |
| Keeps determinism claim | Yes — same silicon | Yes |

The module is 25.5 × 18.0 mm and is mounted on **headers or a socket**, so it can be
replaced if damaged. That is a deliberate Rev A trade: a little height for a lot of
recoverability.

**Do not skip Rev A.** Almost every wearable failure is mechanical or optical, not
electrical, and Rev B cannot fix an optical design that was never validated.

---

## 2. Packaging budget and the outline

### 2.1 Why 45 × 50 mm

| Item | Dimension | Consumes |
|---|---|---|
| ShrikeFi module | 25.5 × 18.0 mm | 18 mm of board **length** (top edge) |
| Antenna keep-out | 15 × 18 mm | inside the module's 18 mm strip, copper-free on **all** layers |
| 0.96" OLED module | 27.3 × 27.0 mm | 27 mm of board **length** below the module |
| USB-C | 8.94 × 7.35 mm | edge, top strip |
| Power section | ~12 × 10 mm | beside the module |
| Skin sensors | ~10 × 6 mm | one end of the **bottom** face |
| LiPo cell | ~25 × 20 × 4 mm | bottom face, offset from the sensors |

Length is set by **18 + 27 = 45 mm minimum**, plus 2.5 mm margin top and bottom →
**50 mm**. Width is set by the display (27.3) and module (25.5) plus the power
section → **45 mm**.

So: **45 × 50 mm**. A sternum patch of this size is normal (consumer ECG patches
run 60 × 40 mm).

### 2.2 The battery-versus-sensor conflict

The bottom face is the skin face, so it holds the sensors — but a 4 mm cell needs
the same real estate. They cannot share it. The layout therefore splits the bottom
face:

```
TOP FACE                    BOTTOM FACE (skin)
┌───────────────────┐       ┌───────────────────┐
│  ShrikeFi module  │  ←18mm│   (header tails)   │
│  [antenna at top] │       │                    │
├───────────────────┤       ├───────────────────┤
│                   │       │   LiPo cell        │
│  0.96" OLED       │ 27mm  │   25 × 20 × 4 mm   │
│  display module   │       │                    │
├───────────────────┤       ├───────────────────┤
│ power / connectors│       │ MAX30102 + TMP117  │
└───────────────────┘       └───────────────────┘
                              ↑ skin contact here
```

Consequence: **the sensors sit at one end of the patch, not the centre.** That is
fine for a chest patch (sternum is a good PPG site) and it is why a strap anchor
must be near that end, so the sensor end is the end that is pulled against skin.

### 2.3 Thickness budget

| Layer | mm |
|---|---|
| PCB (4-layer) | 1.2 |
| Module on headers | 3.1 + 2.5 header |
| Cell | 4.0 |
| Display module | 2.6 |
| Enclosure walls (2 × 1.2) + clearance | 2.4 + 0.8 |
| **Total** | **≈ 16.6** |

That is a patch, not a watch. To get under 12 mm, go to Rev B (bare FPGA, no
headers) and a 3 mm cell.

---

## 3. Bill of materials

Designators are unique across the board. Items marked **verify** have no confirmed
LCSC number yet — confirm stock before layout.

### 3.1 Compute and sensors

| Ref | Part | Package | Face | Notes |
|---|---|---|---|---|
| U1 | Vicharak ShrikeFi module | 2 × 20-pin headers | Top | ESP32-S3 + SLG47910C. Socketed |
| U2 | MAX30102EFD+T | LGA-14 (OESIP) | **Bottom** | **Dual rail: VDD 1.8 V, VLED+ 3.3 V** |
| U3 | TMP117AIDRVR | WSON-6 | **Bottom** | ±0.1 °C class, address 0x48 |
| U4 | BME280 | LGA-8 | Top | 0x76, needs a vent |
| U5 | MPU-6050 | QFN-24 | Top | 0x68 |
| U6 | SSD1306 0.96" module | 4-pin I2C | Top | via J3 |

### 3.2 Power

| Ref | Part | Package | Notes |
|---|---|---|---|
| U7 | AP2112K-3.3 | SOT-23-5 | main 3.3 V |
| U8 | AP2112K-1.8 | SOT-23-5 | **PPG core rail** |
| U9 | MCP73831T-2ACI/OT | SOT-23-5 | charger, 212 mA |
| U10 | DW01A | SOT-23-6 | protection controller |
| U11 | FS8205A | TSSOP-8 | protection FETs |
| U12 | PCA9306 | VSSOP-8 | **I2C 3.3 V ↔ 1.8 V** |
| U13 | MAX17048 | TDFN-8 | **fuel gauge, I2C 0x36** (firmware support absent — see §8.3) |
| U14 | TPS22918 or FDC6330L | SOT-23-5 | **power path / load switch** |
| Q1 | 2N7002 | SOT-23 | buzzer driver |
| Q2 | 2N7002 | SOT-23 | FPGA power-gate inverter |
| Q3 | AO3401A (P-FET) | SOT-23 | FPGA high-side switch |

### 3.3 Connectors, indicators, electromechanical

| Ref | Part | Notes |
|---|---|---|
| J1 | USB-C 16-pin (C165948) | power + native USB |
| J2 | JST-SH 2-pin (C145956) | LiPo cell |
| J3 | FPC/header 4-pin | OLED module |
| J4 | JST-SH 4-pin (C145958) | PMS5003 (5 V) — external, not worn |
| J5 | JST-GH 4-pin | GNSS module — external |
| J6 | **Pogo pads (4): 3V3, GND, UART0 TX, UART0 RX** | **recovery / programming** |
| J7 | **Pogo pads (2): EN, BOOT(IO0)** | **force download mode** |
| SW1, SW2 | Tactile 3.0 × 4.0 mm | SOS / reset |
| BZ1 | Active piezo 5 × 5 mm | alarm |
| D1–D3 | LEDs (charge, power, beat) | |
| — | **ESD array (e.g. USBLC6-2SC6)** | on USB D+/D− |

### 3.4 Passives

As the previous revision: R1/R2 5.1 kΩ (CC1/CC2), R3/R4 3.3 kΩ (I2C 3.3 V),
R5 4.7 kΩ (PROG), R6/R7 2.2 kΩ (I2C 1.8 V), R8 200 kΩ (PCA9306 EN), R9 100 Ω
(DW01A), R10–R12 1 kΩ (LEDs), R13/R15 100 Ω and R14/R16/R17 100 kΩ (gate networks),
R18/R19 1 kΩ (SPI series), R20/R21 1 MΩ (**VBAT divider**, optional). Decoupling:
C1 10 µF VBUS, C2 10 µF VBAT, C3 1 µF + C4 10 µF + C5 100 nF (3.3 V), C6 1 µF +
C7 10 µF + C8 100 nF (1.8 V), C9 10 µF (VLED+), C10 100 nF (MAX30102 VDD), C11
100 nF TMP117, C12 100 nF BME280, C13 100 nF MPU6050, **C14 100 nF REGOUT**,
**C15 2.2 nF CPOUT**, C16/C17 100 nF FPGA, C18 100 nF DW01A, C19 100 nF SOS
debounce, C20 1 µF reset, C21 100 nF PCA9306.

---

## 4. Schematic by subsystem

> **Verify pin numbers against the placed footprint before wiring.** The most common
> board-killing mistake is a mirrored footprint; ERC will not catch it.

### 4.1 Power front end

1. **USB-C (J1):** VBUS → `VBUS_5V`; GND + shield → `GND`; CC1/CC2 → 5.1 kΩ → GND
   (without both, a USB-C source supplies 0 V). D+/D− through an **ESD array** to
   `USB_D+`/`USB_D-`.
2. **Power path (U14):** `VBUS_5V` and `BAT+` both feed the system rail through a
   load switch / ideal-diode arrangement so the board **runs from USB with no cell
   fitted** and switches over without a reset. This was absent before and made
   bench work awkward (the board needed a cell to boot).
3. **Charger (U9):** `VIN` ← `VBUS_5V` (C1), `VBAT` → `BAT+` (C2), `PROG` → R5 →
   GND = 212 mA, `STAT` → D1.
4. **Protection (U10/U11):** in series with the cell negative; `VDD` via R9 with
   C18. System `GND` is downstream of the FET pair.
5. **3.3 V (U7)** and **1.8 V (U8)** LDOs, each with its input/output capacitors.

### 4.2 ESP32-S3 (via the module)

All module pins break out to the carrier. Firmware's pin map
(`firmware/shrikefi/shrikefi_pinmap.h`) is the contract:

| GPIO | Net | Destination |
|---|---|---|
| 1 / 2 | `I2C_SDA` / `I2C_SCL` | 3.3 V I2C segment |
| 4 | `BUZZER_OUT` | Q1 gate via R13 |
| 5 | `SOS_CANCEL` | SW1 to GND, C19 |
| 8 | `FPGA_EN` | module pin (on-module FPGA) |
| 9 | `FPGA_PWR` | module pin (on-module FPGA) |
| 10–13 | SPI to FPGA | on-module |
| 14 | `PMS_RX` | J4 |
| 16 / 17 | `GNSS_RX` / `GNSS_TX` | J5 |
| 18 | `PMS_TX` | J4 |
| 19 / 20 | USB D− / D+ | J1 |
| EN | `MCU_RESET` | SW2 + RC |
| IO0 | `BOOT` | **J7 pogo pad** |

⚠️ GPIO 0, 3, 45, 46 are strapping pins — leave them clean.

### 4.3 I2C and level translation

3.3 V segment: module, OLED, TMP117, BME280, MPU6050, **MAX17048** — pull-ups R3/R4.
1.8 V segment: MAX30102 only — R6/R7. **PCA9306 (U12)** bridges them: `VREF1` 3.3 V,
`VREF2` 1.8 V, `EN` → 1.8 V via R8.

Addresses: OLED 0x3C, **gauge 0x36**, TMP117 0x48, MAX30102 0x57, MPU6050 0x68,
BME280 0x76.

### 4.4 MAX30102 — dual rail (by pin name)

| Pin | Connect to |
|---|---|
| `VDD` (core) | **`+1V8`**, C10 |
| `VLED+` (LED supply) | **`+3V3`**, C9 |
| `PGND` | `PGND`, single-point to `GND` under the sensor |
| `GND` | `GND` |
| `SCL`, `SDA` | **1.8 V segment** (via PCA9306) |
| `INT` | test pad, otherwise open (firmware polls) |

Never power `VDD` from 3.3 V, and never leave `VLED+` unconnected.

### 4.5 Other sensors

- **TMP117 (bottom):** `V+` → 3.3 V (C11), `GND`, `SCL`, `SDA`, `ADD0` → GND (0x48).
  `ALERT` open. Thermal pad → isolated copper island (see §7.4).
- **BME280 (top):** pin 1 `GND`, pin 2 `CSB`→3.3 V, pin 3 `SDI`→SDA, pin 4 `SCK`→SCL,
  pin 5 `SDO`→GND (0x76), pin 6 `VDDIO`→3.3 V, pin 7 `GND`, pin 8 `VDD`→3.3 V (C12).
- **MPU6050 (top):** `VDD`/`VLOGIC` → 3.3 V, `GND`, `AD0`→GND, `REGOUT`→**C14 100 nF**,
  `CPOUT`→**C15 2.2 nF**, `CLKIN`→GND, `FSYNC`→GND, `INT` test pad.

### 4.6 External devices

- **PMS5003 (J4):** pin 1 `VBUS_5V`, pin 2 `GND`, pin 3 `SET`→3.3 V or open,
  pin 4 `RX`→GPIO18, pin 5 `TX`→GPIO14, pin 6 `RST` open. **5 V only.**
- **GNSS (J5):** `VCC` 3.3 V + 10 µF/100 nF, `TXD`→GPIO16, `RXD`→GPIO17, RF→U.FL.
- **Buzzer:** `+`→3.3 V, `−`→Q1 drain, gate via R13/R14.
- **SOS:** SW1 to GND with C19.

---

## 5. Stackup and design rules

4 layers, 1.2 mm, ENIG:

| Layer | Use |
|---|---|
| L1 Top | modules, display connector, SPI/I2C, power |
| L2 | solid GND (cut under the antenna) |
| L3 | `+3V3` pour, `+1V8` island, `VBUS_5V` island |
| L4 Bottom | sensors, skin copper, battery connector, returns |

Rules: signal 0.152 mm, power 0.4–0.6 mm, clearance 0.152 mm, via 0.6/0.3 mm.
Fillets 3 mm at corners (it is worn against skin — sharp corners are a defect).

---

## 6. Placement

1. **Module** at the top edge, antenna outward, **15 mm keep-out copper-free on all
   four layers** — including cutting L2 and L3.
2. **Display** below the module (cannot overlap the keep-out).
3. **USB-C** at an edge with the snout overhanging 0.8 mm.
4. **Power section** grouped at the sensor-opposite end; away from TMP117.
5. **Battery connector** near the power section, cell lying over the middle.
6. **Bottom face:** MAX30102 and TMP117 at the **sensor end**; both near the strap
   anchor at that end so tension presses them to skin.
7. **MPU6050** as close to the mechanical centre as the sensors allow.
8. **BME280** at an edge with an enclosure vent above it.
9. Decoupling within 1 mm of each supply pin.

---

## 7. Optics and the skin interface

This is where the wearable succeeds or fails. On the bench, a **cold finger gave a
perfusion index of 0.83 %** and the FPGA found **zero crests**; a warm finger at
1.32 % worked. Contact quality, not firmware, decided whether there was a signal.

### 7.1 Window

- Material: **IR-transparent** polycarbonate, acrylic or glass. Ordinary opaque
  plastic kills the 660/880 nm light path.
- Thickness ≤ 1.5 mm, flat, polished, no texture.
- **One window over both devices** (LED and photodiode) — separate holes let
  ambient light straight into the detector.

### 7.2 Light seal

- A **black, closed-cell foam gasket** (≈1 mm) around the window perimeter,
  compressing against skin.
- Opaque black potting or a black plastic light pipe around the sensor.
- Do not rely on the enclosure alone — reflected light from the inside of a
  translucent shell is enough to swamp the AC signal.

### 7.3 Pressure and repeatability

- The sensor should protrude **0.2–0.5 mm** beyond the surrounding bottom face so
  skin meets the window before the housing rocks on it.
- Add a **hard stop** (a shoulder of 0.3 mm) so strap tension gives a repeatable
  pressure. Pressing harder **occludes** the capillary bed and destroys the pulse —
  more pressure is not better.
- Keep the sensor end rigid; a flexing board modulates contact pressure at the
  heart-rate harmonic.

### 7.4 TMP117 thermal design

- Copper island on L4 connected **only** to the TMP117 pad, isolated from the GND
  pour by ≥0.3 mm, with a matching cut-out in L1 copper beneath.
- Keep the LDOs and charger at the opposite end: they are the dominant heat sources.
- Expect 12 s to settle after contact. Measured skin values ranged **33.1–33.7 °C**
  across sessions at the same sites, i.e. over 0.5 °C of placement variance — so the
  calibration must be taken per session (see the firmware `/cal` or `cal <ref>`
  console command).

---

## 8. Power path, charging and battery

### 8.1 Load sharing (new)

U14 lets the board run from USB with **no cell fitted** and switches to battery
without resetting the MCU. Previously the system depended on a charged cell to
boot, which made bring-up and recovery harder than it needed to be.

### 8.2 Charge current

MCP73831 with `PROG` = 4.7 kΩ → **212 mA**. Fine for a 400–600 mAh cell (~0.4C).
Do not exceed 0.5C for a cell worn against skin. The cell **must** have the DW01A +
FS8205A protection pair fitted — this is a safety element, not an option.

### 8.3 Battery gauge — hardware present, firmware absent

U13 (MAX17048, I2C **0x36**) and/or the R20/R21 divider give a battery reading at
the hardware level.

**The firmware does not read either today.** The status JSON has no battery field,
and no ADC channel is assigned. Fit the hardware if you want the option, but do not
claim battery monitoring until the firmware publishes it — that would be exactly the
kind of untested claim this project keeps having to retract.

### 8.4 Thermal

Charger and LDO losses sit next to a sensor that measures skin temperature. Keep
them at the far end and, if the cell is adjacent, add a small vent. Charging warms
the patch; do not calibrate skin temperature while charging.

---

## 9. Ingress, mechanics and strap

### 9.1 Ingress target

**IP54** (splash and dust) is realistic for a patch. It requires:

- gasket around the enclosure seam and the window
- USB-C either behind a flap or a **waterproof USB-C variant**
- conformal coat (silicone) over the PCB, **except** the optical window, the TMP117
  skin pad and the pogo pads
- the cell and the PCM sealed from the board cavity

Sweat is conductive and mildly corrosive; it will bridge an uncoated gap. Assume it
will get wet.

### 9.2 Skin-contact materials

ENIG is **nickel with a thin gold flash** — nickel is a common contact allergen, and
this board puts a lithium cell and 3.3 V electronics against skin. Do not describe
the finish as bio-compatible. Options:

- conformal-coat everything except an aperture, or
- use a separate **gold-plated or stainless contact piece** for the TMP117 pad, or
- keep the sensor behind the window and rely on the window's thermal path.

### 9.3 Strap and anchors

- Two **22 mm × 2 mm** slot cut-outs, one at each end, for a standard strap.
- Anchor points must be **outside** the battery region and in line with the sensor
  end, so tension pulls the sensor into skin.
- Add strain relief where the strap passes the PCB edge; a 1.2 mm board will crack
  at a slot under repeated tension. Consider a 2 mm border of solid board around
  each slot.

### 9.4 Recovery pads

J6/J7 expose `3V3`, `GND`, `UART0 TX/RX`, `EN`, `IO0` on pogo pads. Cost is
essentially zero; without them, a board with a dead USB connector or a bricked boot
loader is scrap.

---

## 10. Routing

1. SPI to the FPGA is **on-module** in Rev A — nothing to route.
2. **I2C**: SDA/SCL as a pair, away from the LED return and from the switching node.
3. **1.8 V sensor traces** short, translator close to the sensor.
4. **UART** to J4/J5.
5. **Power** at 0.4–0.6 mm; keep the cell-to-LDO path short.
6. **GND**: pours on L1/L4 stitched to L2 with ≥20 vias, denser near the optical
   sensor and the power section.
7. Separate `PGND` (pulsed LED return) from signal return until a single star point
   under the MAX30102.

Run DRC to zero before ordering, and again after any change. DRC does not check
optics, thermal design, keep-out or mechanics.

---

## 11. Manufacturing and assembly

- **JLCPCB**: 4 layers, 1.2 mm, ENIG, SMT assembly. Panel-friendly outline; no
  slot narrower than 1 mm.
- **Module is not assembled** — it is a socketed/header part you fit yourself. Order
  it separately from Vicharak.
- **Stencil** if you hand-place the QFN/LGA parts; otherwise use JLCPCB's assembly
  for U2–U5, which are the hard ones.
- **Antenna**: verify with a Wi-Fi scan that RSSI does not collapse when the module
  is seated; a keep-out violation shows up as reduced range, not as a build error.

---

## 12. Bring-up

1. **Continuity before power:** `+3V3`↔`GND` and `+1V8`↔`GND` both > 10 kΩ. A beep
   means a short — find it first.
2. **Current-limited first power:** 5.0 V at 100 mA into VBUS, **no cell fitted**
   (possible now that the power path exists). Expect 15–35 mA.
3. Check `+3V3` = 3.30 ± 0.05 V and `+1V8` = 1.80 ± 0.05 V.
4. Fit the module, flash:
   ```powershell
   . C:\Espressif\frameworks\esp-idf-v5.5.5\export.ps1
   cd firmware/shrikefi
   idf.py -p COM4 flash monitor
   ```
5. Expect the authentic boot sequence: 5 I2C devices, `configuration COMPLETE!
   (46408 bytes loaded)`, handshake `0x55` → `0x00` (**`0x00` is success**),
   SoftAP `VALOR-0081`, and a 4-digit control PIN on the OLED.
6. **Optical check with warm hands:** perfusion index should read **> 1 %**. If it
   sits at ~0.8 % and `IBI` stays 0, the light seal or the contact pressure is wrong
   — that is a mechanical fix, not a firmware one.
7. **Calibrate skin temperature** only once the reading has been flat for ~40 s:
   `cal <oral_ref_C>` over the console, then confirm with `calstat`.

---

## 13. What this document does not establish

- Nothing has been fabricated. No Gerbers exist for this revision.
- No power, thermal or EMC measurement exists.
- Sensor accuracy on this board is unmeasured; all bench numbers come from the
  breadboard rig.
- The FPGA is **not** integrated in Rev A by design (§1).
- The battery gauge is fitted but **not read by firmware** (§8.3).
- Not a medical device; no clinical claim is made or supported.

---

## 14. Pre-order verification checklist

Each line needs a datasheet or a stock page, not a memory:

- [ ] BME280 pin 1 = GND, pin 6 = VDDIO, pin 8 = VDD in the placed footprint
- [ ] MAX30102 `VDD` on `+1V8`, `VLED+` on `+3V3`, both connected
- [ ] MAX30102 I2C confirmed 1.8 V-referenced; PCA9306 fitted
- [ ] TMP117 `GND` connected; `ADD0` on the correct pin
- [ ] MPU6050 `GND` and `CPOUT` connected; 2.2 nF on CPOUT, 100 nF on REGOUT
- [ ] ShrikeFi module header footprint / pinout matches the module silkscreen
- [ ] Module antenna keep-out clear on **all four** layers
- [ ] Cell dimensions fit the layout, protection FETs in series with cell negative
- [ ] Power path verified: board boots from USB with **no cell**
- [ ] Optical window: IR-transparent material specified, single window over both devices
- [ ] Light seal gasket specified with a compression figure
- [ ] Sensor protrusion and hard stop defined (0.2–0.5 mm)
- [ ] TMP117 thermal island isolated; LDOs at the opposite end
- [ ] Recovery pogo pads present (3V3, GND, UART0, EN, IO0)
- [ ] Reference designators unique across the whole BOM
- [ ] ERC zero errors, DRC zero errors
- [ ] No 3.3 V net within 0.3 mm of the skin-contact copper

---

*SIH26181 · Project VALOR · Rev A design intent · Not built, not verified, not a
medical device.*
