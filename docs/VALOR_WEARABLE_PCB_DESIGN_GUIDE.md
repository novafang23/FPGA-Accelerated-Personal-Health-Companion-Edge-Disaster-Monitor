# VALOR Wearable — Rev A Wrist Watch

**Status: DESIGN INTENT. Nothing here has been fabricated, assembled or measured.**
No Gerbers exist. No Rev A prototype has been built. Every number is a
specification to be met, not a result.

| Field | Value |
|---|---|
| Project | SIH26181 — VALOR |
| Revision | **Rev A** — carrier board around an ESP32-S3 module |
| Form factor | **Wrist watch**, 44 mm round case, 40 × 38 mm PCB |
| MCU | **ESP32-S3-WROOM-1U-N8R2** (U.FL, no PCB antenna, no keep-out) |
| FPGA | **bare Renesas SLG47910C (U16)**, WLCSP-20, placed on this board |
| Display | **0.66" 64×48 SSD1306 I2C module** (18 × 18 mm) |
| Primary PPG | **MAX30101** (green + red + IR) — see §15 |
| Off-board | PMS5003 and GNSS are **not** on the watch; 4-pin demo connectors only |
| Cell | 200–250 mAh single-cell LiPo, ~25 × 20 × 3 mm |
| Charging | **Pogo contacts** (no USB-C on the watch) |
| Layers | 4, 0.8–1.0 mm, ENIG |
| Regulatory | **Not a medical device.** Research / competition prototype |

> **Why this revision exists.** A wrist watch cannot host the patch-board layout.
> The 15 mm antenna keep-out of a PCB-antenna module plus a 0.96" display needs
> ~45 mm of board length, which makes a 48 mm+ case — not a watch. Two changes fix
> that: an **external-antenna module** (no keep-out at all) and a **0.66" display**
> (18 mm instead of 27 mm). The board becomes 40 × 38 mm and fits a 44 mm case.
>
> ⚠️ **The engineering risk moved, it did not disappear.** Wrist PPG is much weaker
> and far noisier than a fingertip (§7.5). Read that section before committing to a
> wrist design.

---

## 1. Rev A vs Rev B

| | **Rev A (this document)** | Rev B (later) |
|---|---|---|
| Compute | `ESP32-S3-WROOM-1U` module + U.FL antenna | Bare ESP32-S3 + bare FPGA |
| FPGA | **bare SLG47910C (U16)** on this board | same package, tighter layout |
| Board | 40 × 38 mm watch PCB | smaller, single die pair |
| Goal | Prove the **watch form factor and the optics** | Miniaturise |
| Assembly risk | **High** — WLCSP-20 at 0.4 mm pitch, plus double-sided SMD | High |

> **Assembly reality.** This is not a beginner board: 15 surface-mount ICs including a
> 2 × 2 mm WSON, an optical LGA-14, a QFN-24 and a **WLCSP-20 at 0.4 mm pitch**, on
> both sides of a 4-layer 0.8 mm board. Double-sided assembly means two reflow passes,
> double-sided fees, and mostly extended components — roughly ₹15,000–22,000 for five
> assembled boards landed in India.
>
> **Build a Rev 0 first**: a 2-layer practice carrier (₹600–1200) taking the
> `WROOM-1U` on headers with sensors on breakout modules. It teaches KiCad, footprints,
> ground pours and Gerber export with nothing expensive at risk, and validates the
> optics and enclosure before the real spin.

The `-1U` variant is the whole reason a watch is feasible: it has **no PCB antenna**,
so the 15 mm copper-free keep-out disappears and the board can use its full width.
The antenna becomes an FPC that lives in the strap or on the case wall — where a
watch antenna belongs anyway, because a wrist is a bad place to radiate from a
ground plane.

---

## 2. Packaging budget — why 40 × 38 mm

### 2.1 Linear fit is what matters

| Item | Size | Consumes |
|---|---|---|
| `WROOM-1U` module | 25.5 × 18.0 mm | **18 mm of length** (12 o'clock end) |
| 0.66" OLED module | ~18 × 18 mm | **18 mm of length** (centre) |
| Cell | 25 × 20 × 3 mm | bottom face, offset toward 12 o'clock |
| MAX30101 + TMP117 | ~10 × 6 mm | bottom face at **6 o'clock** |
| Power section | ~10 × 8 mm | beside the module |

18 + 18 = **36 mm** of length, plus 1 mm margins → **38 mm**. Width is set by the
module at 25.5 mm, but 40 mm is chosen so the 6 o'clock sensor end has room for the
case-back protrusion and the strap lugs.

**No antenna keep-out is required** because the module has no antenna. Do keep the
U.FL pigtail and the antenna FPC **away from** the PPG traces and the cell.

### 2.2 The case

44 mm diameter, 12.5 mm thick:

| Layer | mm |
|---|---|
| Case back + sensor protrusion | 1.5 |
| PCB (0.8 mm) | 0.8 |
| Cell (3.0 mm) | 3.0 |
| Display module | 2.4 |
| Crystal / cover glass | 0.8 |
| Gasket + clearances | 4.0 |
| **Total** | **12.5** |

44 mm is a large watch but a normal one (many sport watches are 44–46 mm). Going to
40 mm means dropping to a 0.49" display and a smaller cell.

### 2.3 Bottom-face conflict, resolved

The back face must hold the cell **and** the skin sensors. They cannot share space,
so the watch is split front-to-back, not left-to-right:

```
CASE BACK (skin side)                MOVEMENT
┌──────────────────────────┐
│  12 o'clock: cell        │        PCB top: module (12), display (centre)
│  25 × 20 × 3             │        PCB bottom: cell (12), sensors (6)
├──────────────────────────┤
│   6 o'clock: optical     │        ← convex case back presses these into skin
│   window + TMP117        │
└──────────────────────────┘
```

Consequence: **the sensor end must be the strap-tight end.** Put the buckle side at
12 o'clock and the tail at 6 o'clock so the strap tension pulls the optical window
onto the wrist rather than lifting it.

---

## 3. Bill of materials

### 3.1 Compute and sensors

| Ref | Part | Face | Notes |
|---|---|---|---|
| U1 | **ESP32-S3-WROOM-1U-N8R2** | Top | 8 MB flash, 2 MB PSRAM, **U.FL antenna** |
| **U16** | **Renesas SLG47910C** | Top | **WLCSP-20, 0.4 mm pitch** — 1120 LUT5s, internal 50 MHz osc. The FPGA is NOT inside the ESP32 module |
| ANT1 | 2.4 GHz FPC antenna + U.FL lead | Case/strap | keep clear of PPG and cell |
| U2 | **MAX30101EFD+T** | **Bottom** | **VDD 1.8 V, VLED+ VBAT**. Green for wrist HR, red/IR for SpO2. Same LGA-14 land pattern as the MAX30102 it replaces |
| U15 | DRV2605 + LRA (optional) | Bottom | haptic alert — silent SOS feedback |
| U3 | TMP117AIDRVR | **Bottom** | 0x48, thermal pad to skin island |
| U4 | BME280 | Top | 0x76, needs a vent — a watch traps air, see §9.3 |
| U5 | MPU-6050 | Top | 0x68 |
| U6 | **0.66" SSD1306 module** (64×48) | Top | 4-pin I2C, ~18 × 18 mm |

### 3.2 Power

| Ref | Part | Notes |
|---|---|---|
| U7 | AP2112K-3.3 | main rail |
| U8 | AP2112K-1.8 | **PPG core rail** |
| U9 | MCP73831T-2ACI/OT | charge from pogo contacts |
| U10/U11 | DW01A + FS8205A | cell protection (mandatory, worn against skin) |
| U12 | PCA9306 | **I2C 3.3 V ↔ 1.8 V** |
| U13 | MAX17048 | fuel gauge, I2C **0x36** (firmware does not read it — §8.4) |
| U14 | Load switch / ideal diode | runs from charge input with no cell |
| Q1/Q2/Q3 | 2N7002 ×2, AO3401A | buzzer, FPGA power gate |

### 3.3 Connectors and electromechanical

| Ref | Part | Notes |
|---|---|---|
| J1 | **Pogo pads ×2: 5 V in, GND** | charging contacts in the **3 and 9 o'clock margins, outside the cell outline** |
| J2 | **Pogo pads ×4: 3V3, GND, UART0 TX/RX** | flashing / recovery |
| J3 | **Pogo pads ×2: EN, IO0** | force download mode |
| J4 | JST-SH 2-pin | cell |
| J5 | FPC 4-pin | OLED |
| J6 | JST-SH 4-pin | PMS5003 (5 V) — external, not worn |
| J7 | JST-GH 4-pin | GNSS — external |
| SW1, SW2 | Side tactile 3 × 4 mm | SOS / reset, on the case side |
| BZ1 | Piezo 5 × 5 mm | alarm; needs a sound port |
| D1–D3 | LEDs | charge / power / beat |

**No USB-C on the watch.** It costs 66 mm², 3 mm of thickness and an ingress path.
Flashing and recovery go over the pogo UART pads, which `esptool` supports exactly
like a USB-UART bridge. Use the existing bench board for convenient USB work.

### 3.4 Passives

Decoupling and gate networks as before: R1/R2 5.1 kΩ (only needed where USB-C is
present — omit on the watch), R3/R4 3.3 kΩ (I2C 3.3 V), R5 4.7 kΩ (PROG), R6/R7
2.2 kΩ (I2C 1.8 V), R8 200 kΩ, R9 100 Ω, R10–R12 1 kΩ, R13/R15 100 Ω,
R14/R16/R17 100 kΩ, R20/R21 1 MΩ (VBAT divider, optional). C1 10 µF, C3 1 µF +
C4 10 µF + C5 100 nF (3.3 V), C6 1 µF + C7 10 µF + C8 100 nF (1.8 V), C9 10 µF
(VLED+), C10 100 nF (MAX30101 VDD), C11 100 nF TMP117, C12 100 nF BME280, C13
100 nF MPU6050, **C14 100 nF REGOUT**, **C15 2.2 nF CPOUT**, C16/C17 100 nF FPGA,
C18 100 nF DW01A, C19 100 nF SOS, C20 1 µF reset, C21 100 nF PCA9306.

---

## 4. Schematic

> Verify pin numbers against the placed footprint. A mirrored footprint passes ERC
> and fails on the bench.

### 4.1 Module

`WROOM-1U` breaks out to the carrier. Pin map is the contract in
`firmware/shrikefi/shrikefi_pinmap.h`: I2C on GPIO1/2, buzzer GPIO4, SOS GPIO5,
FPGA on-module (GPIO8–13), PMS UART1 GPIO14/18, GNSS UART2 GPIO16/17, USB
GPIO19/20 (**unused on the watch — leave as test pads**). Strapping pins 0/3/45/46
stay clean except IO0 which goes to the J3 pogo pad.

### 4.1b ForgeFPGA (U16) — the part the module does not contain

An `ESP32-S3-WROOM-1U` is an Espressif Wi-Fi/BLE module and contains **no FPGA**.
The ForgeFPGA is a separate die that must be placed on this board.

| U16 pin | Net | Notes |
|---|---|---|
| `VDD` (both pins) | `+3V3_FPGA` | C16, C17 (100 nF each) |
| `OSC_EN` | `VDD` | internal 50 MHz oscillator |
| `PIN_16` | `SPI_SCK` ← GPIO12 | 1 kΩ series |
| `PIN_17` | `SPI_SS_N` ← GPIO10 | |
| `PIN_18` | `SPI_MOSI` ← GPIO11 | 1 kΩ series |
| `PIN_19` | `SPI_MISO` → GPIO13 | |
| `PIN_7` | beat LED | 1 kΩ to D3 |
| `EN` | `FPGA_EN` ← GPIO8 | boot-mode latch |
| supply | via Q3 (AO3401A) | high-side switch driven by Q2 from `FPGA_PWR` (GPIO9) |

Firmware loads the 46,408-byte bitstream over **this same SPI bus** at boot, then
probes `0x55` expecting `0x00` (success, not failure).

**If the WLCSP cannot be placed** (stock, stencil, X-ray), fall back to a socketed
**Vicharak ShrikeFi** daughterboard, which already carries both dies. Larger case,
no 0.4 mm-pitch assembly. Decide **before** layout — it changes the whole board.

### 4.2 I2C

3.3 V segment: module, OLED, TMP117, BME280, MPU6050, MAX17048 — R3/R4 pull-ups.
1.8 V segment: MAX30101 only — R6/R7. **PCA9306** bridges: `VREF1` 3.3 V,
`VREF2` 1.8 V, `EN` → 1.8 V via R8.

Addresses: OLED 0x3C, gauge 0x36, TMP117 0x48, MAX30101 0x57, MPU6050 0x68,
BME280 0x76.

### 4.3 MAX30101 — by pin name

| Pin | To |
|---|---|
| `VDD` | **`+1V8`**, C10 |
| `VLED+` | **`VBAT` (3.7–4.2 V)**, C9 10 µF |
| `PGND` | `PGND` → single star point under the sensor |
| `GND` | GND |
| `SCL`/`SDA` | 1.8 V segment |
| `INT` | test pad (firmware polls) |

Never power `VDD` at 3.3 V; never leave `VLED+` open.

**`VLED+` runs from `VBAT`, not `+3V3`.** The green LED's forward voltage is roughly
**3.3–3.7 V**, and the internal current sink needs ~0.3 V of compliance headroom on
top. A 3.3 V rail therefore starves the green driver exactly when you raise current
for wrist use. `VLED+` is rated to 5.25 V for precisely this reason, so the protected
battery rail (3.7–4.2 V) is the correct source. Confirm green V_F and the `VLED+`
minimum on the datasheet before layout.

### 4.4 Other sensors

- **TMP117:** `V+` 3.3 V (C11), `GND`, `SCL`, `SDA`, `ADD0`→GND. Thermal pad →
  isolated skin island (§7.6).
- **BME280:** 1 `GND`, 2 `CSB`→3.3 V, 3 `SDI`→SDA, 4 `SCK`→SCL, 5 `SDO`→GND,
  6 `VDDIO`→3.3 V, 7 `GND`, 8 `VDD`→3.3 V (C12).
- **MPU6050:** `VDD`/`VLOGIC` 3.3 V, `GND`, `AD0`→GND, `REGOUT`→**C14 100 nF**,
  `CPOUT`→**C15 2.2 nF**, `CLKIN`→GND, `FSYNC`→GND, `INT` test pad.

### 4.5 Charging and protection

Pogo 5 V → MCP73831 `VIN` (C1); `VBAT`→`BAT+` (C2); `PROG` = R5 → 212 mA; `STAT`→D1.
DW01A + FS8205A in series with the cell negative; system `GND` downstream of the
FETs. Load switch so the board runs from the cradle with no cell fitted.

---

## 5. Stackup and rules

4 layers, **0.8 mm** (its 1.2 mm on a watch is wasted thickness), ENIG.
L1 signals/top components, L2 solid GND, L3 power (`+3V3`, `+1V8`, `VBUS`), L4
sensors/skin copper/returns. Signal 0.15 mm, power 0.4 mm, clearance 0.15 mm, via
0.5/0.25 mm (0.6/0.3 does not fit a watch pitch budget). No sharp corners on any
skin-facing edge.

---

## 6. Placement

1. **Module** at 12 o'clock, U.FL connector at the case edge, pigtail routed away
   from the optical window and the cell.
2. **Display** centre, on top; its aperture is in the cover glass.
3. **Cell** bottom face, 12 o'clock half. The 3 and 9 o'clock margins (~7.5 mm each)
   are reserved for pogo pads and must not be covered by it.
4. **MAX30101 + TMP117** bottom face at **6 o'clock**, on the case-back protrusion.
5. **MPU6050** as close to the movement centre as the cell allows.
6. **BME280** at the case wall, isolated from the cell and the charger (a sealed
   watch traps heat — §9.3).
7. **Charger/LDOs** at 12 o'clock, never near TMP117.
8. Decoupling within 1 mm of each supply pin; the MAX30101's within 0.5 mm.

---

## 7. Optics, skin interface, and the wrist problem

### 7.1 Window

Single **IR-transparent** window (polycarbonate, acrylic or glass, ≤ 1.5 mm, flat,
polished) covering **both** the LED and the photodiode. Two separate holes leak
ambient light straight into the detector.

### 7.2 Light seal

Black closed-cell foam gasket (~1 mm) around the window, compressing against skin,
plus a black light pipe or black potting around the sensor. A translucent case will
pipe ambient light to the photodiode even if the window is perfect.

### 7.3 Pressure and hard stop

- Sensor window protrudes **0.3–0.5 mm** beyond the case-back surface.
- Convex case back (~1.5 mm dome) so the watch centres itself on the wrist and the
  window sees consistent pressure.
- **Hard stop** so strap tension cannot over-press. Pressing hard occludes the
  capillary bed and destroys the pulse — this is the most common self-inflicted PPG
  failure.

### 7.4 Window cleanliness and hair

A wrist sensor sits where sweat, hair and skin oil collect. Specify a **flush,
wipeable** window with no recess that traps debris, and no adhesive edges near it.

### 7.5 ⚠️ The wrist is a much worse PPG site — plan for it

This is the honest risk of the watch path, and it is bigger than anything electrical
in this document.

- **Signal amplitude.** On the bench, a fingertip gave **0.83 % perfusion index cold
  and 1.32 % warm**, and at 0.83 % the FPGA crest detector found **zero beats**.
  Wrist perfusion is typically *lower* than a fingertip, so expect PI in the
  **0.2–1 %** band at rest. The detector may be marginal.
- **Motion artifact.** Wrist PPG is dominated by movement; the fingertip rig never
  saw this. Firmware already has a motion/SQI gate — it will now be load-bearing
  rather than a nicety, and HR will be **intermittent while moving**. That is normal
  for wrist optical HR and should be stated, not hidden.
- **Wavelength.** MAX30101 is **red/IR only**. Wrist optical HR is normally done
  with **green**, which has much better SNR in superficial wrist vasculature. Red/IR
  is workable (it is what many bands use for SpO2) but expect a weaker pulse.

**Mitigations, in order of value:**

1. Optically tight window + light seal + repeatable pressure (§7.1–7.3) — the
   single biggest lever.
2. Sternum-compatible strap tension; 6 o'clock sensor; convex back.
3. Tap the existing adaptive LED current (`max30102_adjust_led_current`) upward.
4. Rely on the SQI + motion gate; publish HR only when quality is good, and publish
   nothing otherwise. The firmware already refuses to invent values — keep it.
5. If PI stays below ~0.3 %, accept that this is a **SpO2 / trend** device rather
   than a beat-to-beat HRV device, and say so.

HRV on a wrist, at 10 ms IBI quantisation *plus* motion artifact, is not something to
promise. The bench HRV numbers were taken on a still fingertip.

### 7.6 TMP117 thermal design

Isolated copper island on L4 connected only to the TMP117 pad, ≥0.3 mm clearance from
the GND pour, with an L1 cut-out beneath. The charger and LDOs sit at the opposite
end. Measured skin values ranged **33.1–33.7 °C** across sessions at the same site —
over 0.5 °C of placement variance — so calibrate per session (`cal <ref>` on the
console, or `/cal?ref=` over the AP).

---

## 8. Power

### 8.1 Cell and runtime

200–250 mAh at 3.0 mm. Charge current 212 mA ≈ 0.9C for a 250 mAh cell — **too
aggressive for a cell worn on skin**. Reduce `PROG` to 10 kΩ → **100 mA** (0.4C) on
the watch variant.

### 8.2 Charging

Two pogo contacts (5 V, GND) into the MCP73831, with a load switch so the watch runs
while charging. Add reverse-polarity protection on the contacts — a cradle can be
reversed by a user.

### 8.3 Runtime estimate

Not measured. Rough: ~90 mA average with Wi-Fi AP active → **~2.5 h** on 250 mAh. If
runtime matters, the Wi-Fi AP is the thing to duty-cycle; that is a firmware issue,
not a PCB one.

### 8.4 Fuel gauge

U13 (MAX17048, 0x36) is fitted. **Firmware does not read it** and `status.json` has
no battery field. Do not claim battery monitoring until the firmware publishes it.

---

## 9. Mechanics and ingress

### 9.1 Case and strap

44 mm case, 20 mm strap lugs, spring bars. Lugs are part of the case, not the PCB —
do not hang a strap load off a 0.8 mm board.

### 9.2 Ingress target

**IP67** for a watch that is worn daily. Requires: gasket at the case back, o-ring or
gasket at the crystal, sealed pogo contacts (gold, recessed, with a drain path),
conformal coat over everything except the optical window, the TMP117 skin island and
the pogo pads. Sweat is conductive and mildly corrosive — assume ingress.

### 9.3 A sealed watch traps heat and air

Two consequences specific to a watch enclosure:

- **BME280** must breathe through a vent to outside air, not the case cavity. Fit it
  near a dedicated vent with a membrane, and accept that case temperature offsets it.
- **Temperature drift** in a sealed case is larger than on an open bench. The TMP117
  offset calibration absorbs a constant bias but not a drift.

### 9.4 Skin-contact materials

ENIG is **nickel with a thin gold flash**; nickel is a common contact allergen, and
this puts a lithium cell against skin. Do not call the finish bio-compatible. Either
conformal-coat everything except an aperture, or use a separate gold-plated or
stainless contact piece.

### 9.5 Recovery pads

The J2/J3 pogo pads are the **only** way to flash or unbrick the watch once assembled.
Cost is essentially zero; without them a failed OTA or a bad boot loader means
opening a sealed case or scrapping the board.

---

## 10. Routing

1. **PPG traces short and away from** the cell, the U.FL pigtail and the charger.
2. I2C SDA/SCL paired; keep the 1.8 V segment short with the translator near the
   sensor.
3. Pogo charge input and battery leads at 0.4 mm.
4. GND pours L1/L4 stitched to L2, denser under the sensor and the power section.
5. Separate `PGND` (pulsed LED return) until a single star point under the MAX30101.
6. **Do not route anything under the U.FL pigtail** path or across the antenna FPC.

DRC to zero before ordering — and DRC will not check optics, keep-out, mechanics or
thermal design.

---

## 11. Manufacturing and assembly

- 4 layers, 0.8 mm, ENIG, JLCPCB. Minimum slot 1 mm; no thin features on skin faces.
- Module **not** assembled by JLCPCB — fit it yourself (it is a standard module).
- The QFN/LGA parts (MAX30101, TMP117, BME280, MPU6050) are the hard ones; use
  stencil + reflow, or JLCPCB assembly for those.
- Antenna FPC and U.FL fitted after assembly; verify Wi-Fi RSSI **with the case on**,
  since the case and wrist change the antenna environment.

---

## 12. Bring-up

1. Continuity: `+3V3`↔`GND`, `+1V8`↔`GND` both > 10 kΩ.
2. 5.0 V at 100 mA into the **charge pogo pads**, no cell fitted (power path
   permits this). Expect 15–35 mA, `+3V3` 3.30 ± 0.05 V, `+1V8` 1.80 ± 0.05 V.
3. Fit the module, flash over the **pogo UART** pads:
   ```powershell
   . C:\Espressif\frameworks\esp-idf-v5.5.5\export.ps1
   cd firmware/shrikefi
   idf.py -p COM4 flash monitor
   ```
4. Expect 5 I2C devices, `configuration COMPLETE! (46408 bytes loaded)`, handshake
   `0x55` → `0x00` (**`0x00` is success**), SoftAP `VALOR-0081`, 4-digit PIN on the
   display.
5. **Optical check on the wrist, warm and still.** Watch the perfusion index. Above
   ~1 % is good; 0.3–1 % is workable with the SQI gate; below 0.3 % means the window,
   seal or pressure is wrong — fix mechanics, not firmware.
6. Calibrate skin temperature only after the reading has been flat for ~40 s, then
   confirm with `calstat`.

---

## 13. What this document does not establish

- Nothing has been fabricated; no Gerbers exist for this revision.
- No power, thermal, EMC or ingress testing exists.
- **Wrist PPG performance is entirely unmeasured.** All optical numbers come from a
  fingertip on the bench rig.
- Battery runtime is an estimate, not a measurement.
- The fuel gauge is fitted but not read by firmware.
- Not a medical device; no clinical claim is made or supported.

---

## 14. Pre-order verification checklist

- [ ] `WROOM-1U` footprint and pin numbering match the module silkscreen
- [ ] U.FL pigtail routed clear of the optical window, cell and PPG traces
- [ ] BME280 pin 1 = GND, pin 6 = VDDIO, pin 8 = VDD in the placed footprint
- [ ] MAX30101 `VDD` on `+1V8`, `VLED+` on **VBAT**, both connected
- [ ] Green V_F and `VLED+` minimum confirmed against the datasheet
- [ ] MAX30101 I2C confirmed 1.8 V-referenced; PCA9306 fitted
- [ ] TMP117 `GND` connected, `ADD0` correct, thermal island isolated
- [ ] MPU6050 `GND` + `CPOUT`; 2.2 nF on CPOUT, 100 nF on REGOUT
- [ ] Cell ≤ 26 × 21 × 3.2 mm fits the cavity with the protection FETs
- [ ] Board boots from the charge contacts with **no cell**
- [ ] Charge current reduced to ~100 mA (`PROG` = 10 kΩ)
- [ ] Optical window: IR-transparent, **single** aperture, flush and wipeable
- [ ] Light seal gasket specified with a compression figure
- [ ] Sensor protrusion 0.3–0.5 mm with a hard stop
- [ ] Pogo pads present for charging **and** UART/EN/IO0 recovery
- [ ] Case-back convex dome and 6 o'clock sensor position defined
- [ ] Reference designators unique across the whole BOM
- [ ] ERC zero errors, DRC zero errors
- [ ] No 3.3 V net within 0.3 mm of skin-contact copper
- [ ] Green LED channel enabled on the MAX30101 and read back on hardware
- [ ] Case modelled to §16 dimensions: window at 6 o’clock, convex back, 20 mm lugs
- [ ] Light-seal groove and gasket drawn, not left to assembly time
- [ ] BME280 vent to outside air with a membrane, not into the case cavity

---

## 15. Sensor set decision — what to buy, replace, or drop

You are free to change parts, so this is the recommended set for a **wrist**
wearable. The rule applied: change only what the wrist forces, keep everything the
firmware already drives.

| Part | Decision | Why | Firmware impact |
|---|---|---|---|
| **MAX30101 → MAX30101** | **Replace** | The MAX30102 is **red/IR only**. Wrist optical HR is done with **green**, which sees superficial wrist vessels with far better SNR. The MAX30101 adds a green channel in the **same LGA-14 land pattern**, and shares part ID 0x15 | **Small.** Enable LED2 (green) in multi-LED mode and read the green slot. Register map is otherwise the same family |
| MPU6050 | **Keep for Rev A** | EOL and only I2C, but the driver, fall FSM and host tests all exist. Swapping to LSM6DSO/BMI270 is a genuine driver port | none now; a Rev B task |
| TMP117 | Keep | ±0.1 °C class, 2 × 2 mm, ideal contact thermometer | none |
| BME280 | Keep | Needed for cyclone trend and the PM humidity correction | none |
| MAX17048 gauge | Keep | Battery state on a wearable is table stakes | **read it** — currently fitted but unused |
| **PMS5003** | **Drop from the watch** | 5 V laser module, ~100 mA, needs airflow. It cannot live in a sealed 44 mm case | Keep the 4-pin connector for bench/demo use only |
| **GNSS** | **Drop from the watch** | Antenna + 5 V-ish module + sky view; a wrist is the worst place | Bench connector only |
| Display | Keep SSD1306 0.66" | The driver, CSP hashes and dashboard all work. A round GC9A01 looks better but is a new driver + new UI | none |
| Haptic (DRV2605) | **Add (optional)** | A wearable alarm must be felt, not heard. Cheap, small, and it makes the SOS demo land | new driver, ~100 lines |

**Product framing that follows from this:** the watch is the *physiology* device
(HR, SpO2, HRV, skin/ambient temperature, motion, fall, triage, display). The
*hazard* sensing (PM2.5, pressure trend, GNSS) belongs to a strap-clip companion or
stays on the bench rig. Trying to fit a laser particle counter into a watch case is
what makes wearable projects fail.

### 15.1 Do not buy these

- Any PPG part without **green** for a wrist design.
- A 0.96" OLED for a 44 mm case (27 mm of a 38 mm board).
- A PCB-antenna ESP32 module (the 15 mm keep-out does not fit a watch).
- USB-C on the watch board (66 mm², 3 mm, and an ingress path).

---

## 16. 3D enclosure specification

Model this from the numbers below; every dimension is a decision, not a suggestion.
Prototype in **PA12 (SLS) or resin (SLA)**; tolerances assume ±0.1 mm.

### 16.1 Case

| Feature | Dimension |
|---|---|
| Outside diameter | **44.0 mm** |
| Overall thickness | **12.5 mm** (back 1.5 + PCB 0.8 + cell 3.0 + display 2.4 + crystal 0.8 + clearances) |
| Wall thickness | 1.2 mm (min 1.0 at the lug roots) |
| Draft | 1–2° on all vertical faces |
| Fillets | R1 minimum, R2 on outer edges |
| Parts | **2**: bezel (crystal + display) and case back (sensor window) |
| Joint | gasket groove 1.0 × 0.8 mm, 4 × M1.6 screws into heat-set inserts, **or** snap-fit + 2 screws |
| Colour | matte black (a translucent shell pipes ambient light into the photodiode) |

### 16.2 PCB mounting

| Feature | Dimension |
|---|---|
| PCB | 40.0 × 38.0 mm, 0.8 mm thick, 3 mm corner fillets |
| Mounting holes | 4 × Ø1.6 mm, 2.5 mm in from corners |
| Standoffs | 4 × Ø3.0 mm, height 2.0 mm from the back inner face |
| Retention | M1.6 × 4 mm screws into brass heat-set inserts |
| Clearance | ≥0.4 mm PCB to case wall; ≥1.0 mm to the crystal |

### 16.3 Display aperture (top)

| Feature | Dimension |
|---|---|
| Active area (0.66" 64×48) | ~13.5 × 10.5 mm |
| Aperture | active + 0.6 mm per side |
| Bezel lip | 0.5 mm overlap onto the glass |
| Crystal | 0.8 mm, flush, bonded with a gasket ring |
| FPC exit | 4-pin, routed away from the optical end |

### 16.4 Optical window (6 o'clock, case back)

| Feature | Dimension |
|---|---|
| Window centre | 9.0 mm from the 6 o'clock edge, on the centreline |
| Aperture | **8.0 × 5.0 mm single opening** over LED **and** photodiode |
| Window body | IR-transparent PC/acrylic, 1.0–1.5 mm, flat and polished |
| Light seal groove | 1.0 wide × 0.8 deep, black closed-cell foam gasket |
| Sensor protrusion | window sits **0.3–0.5 mm proud** of the case-back surface |
| Hard stop | 0.3 mm shoulder so strap tension cannot over-press |
| Case back | **convex dome, ~1.5 mm**, so the watch centres on the wrist |

### 16.5 TMP117 skin pad

| Feature | Dimension |
|---|---|
| Position | adjacent to the optical window, same 6 o'clock zone |
| Aperture | Ø2.5 mm through the case back |
| Contact | gold-plated or stainless disc, flush with the window protrusion |
| Isolation | ≥1.0 mm from any 3.3 V copper; thermal island on L4 |

### 16.6 Pogo contacts and debug

| Feature | Dimension |
|---|---|
| Charge pads | 2 × Ø3.0 mm gold, **recessed 0.3 mm**, in the **3 and 9 o'clock margin strips** — never under the 25 mm cell |
| Polarity | marked on the case; reverse-protection diode on the PCB |
| Debug pads | 6 pads under a **removable rubber plug** on the case back |
| Drain | chamfered recess so sweat runs off, not into the seal |

### 16.7 Vents and ports

| Feature | Dimension |
|---|---|
| BME280 vent | Ø1.0 mm through the **case wall**, ePTFE membrane, opposite the skin side |
| Buzzer port | Ø2.0 mm, side wall, with a mesh |
| Buttons | 2 side buttons (SOS, reset), silicone boots, 0.3 mm pre-travel |

### 16.8 Strap

| Feature | Dimension |
|---|---|
| Lug width | **20 mm** |
| Lugs | part of the case, 2.5 mm thick, integral fillet to the body |
| Spring bars | Ø1.5 mm |
| Sensor end | **6 o'clock** — buckle at 12 so tension presses the window to skin |
| Load path | never through the PCB |

### 16.9 Volume check

Internal cavity ≈ π × 19² × 10 ≈ 11.3 cm³. Board 40 × 38 × 0.8 ≈ 1.2 cm³, cell
≈ 1.5 cm³, display ≈ 0.8 cm³, module 1.5 cm³ → **~5 cm³ used, ~6 cm³ free**. There is
room; the constraint is thickness and the 6 o'clock sensor stack, not volume.

---

*SIH26181 · Project VALOR · Rev A wrist watch design intent · Not built, not
verified, not a medical device.*
