# VALOR Wearable — Rev A Wrist Watch

**Status: DESIGN INTENT. Nothing here has been fabricated, assembled or measured.**
No Gerbers exist. No Rev A prototype has been built. Every number is a
specification to be met, not a result.

| Field | Value |
|---|---|
| Project | SIH26181 — VALOR |
| Revision | **Rev A** — carrier board around an ESP32-S3 module |
| Form factor | **Wrist watch**, 44 mm round case, 40 × 38 mm PCB |
| MCU + FPGA | **ESP32-S3-WROOM-1U** (external antenna) + on-module SLG47910C |
| Display | **0.66" 64×48 SSD1306 I2C module** (18 × 18 mm) |
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
| FPGA | on the module | WLCSP-20 integrated |
| Board | 40 × 38 mm watch PCB | smaller, single die pair |
| Goal | Prove the **watch form factor and the optics** | Miniaturise |
| Assembly risk | Low | High (0.4 mm pitch, sourcing) |

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
| MAX30102 + TMP117 | ~10 × 6 mm | bottom face at **6 o'clock** |
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
| ANT1 | 2.4 GHz FPC antenna + U.FL lead | Case/strap | keep clear of PPG and cell |
| U2 | MAX30102EFD+T | **Bottom** | **VDD 1.8 V, VLED+ 3.3 V** |
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
| J1 | **Pogo pads ×2: 5 V in, GND** | charging cradle contacts |
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
(VLED+), C10 100 nF (MAX30102 VDD), C11 100 nF TMP117, C12 100 nF BME280, C13
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

### 4.2 I2C

3.3 V segment: module, OLED, TMP117, BME280, MPU6050, MAX17048 — R3/R4 pull-ups.
1.8 V segment: MAX30102 only — R6/R7. **PCA9306** bridges: `VREF1` 3.3 V,
`VREF2` 1.8 V, `EN` → 1.8 V via R8.

Addresses: OLED 0x3C, gauge 0x36, TMP117 0x48, MAX30102 0x57, MPU6050 0x68,
BME280 0x76.

### 4.3 MAX30102 — by pin name

| Pin | To |
|---|---|
| `VDD` | **`+1V8`**, C10 |
| `VLED+` | **`+3V3`**, C9 |
| `PGND` | `PGND` → single star point under the sensor |
| `GND` | GND |
| `SCL`/`SDA` | 1.8 V segment |
| `INT` | test pad (firmware polls) |

Never power `VDD` at 3.3 V; never leave `VLED+` open.

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
3. **Cell** bottom face, 12 o'clock half.
4. **MAX30102 + TMP117** bottom face at **6 o'clock**, on the case-back protrusion.
5. **MPU6050** as close to the movement centre as the cell allows.
6. **BME280** at the case wall, isolated from the cell and the charger (a sealed
   watch traps heat — §9.3).
7. **Charger/LDOs** at 12 o'clock, never near TMP117.
8. Decoupling within 1 mm of each supply pin; the MAX30102's within 0.5 mm.

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
- **Wavelength.** MAX30102 is **red/IR only**. Wrist optical HR is normally done
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
5. Separate `PGND` (pulsed LED return) until a single star point under the MAX30102.
6. **Do not route anything under the U.FL pigtail** path or across the antenna FPC.

DRC to zero before ordering — and DRC will not check optics, keep-out, mechanics or
thermal design.

---

## 11. Manufacturing and assembly

- 4 layers, 0.8 mm, ENIG, JLCPCB. Minimum slot 1 mm; no thin features on skin faces.
- Module **not** assembled by JLCPCB — fit it yourself (it is a standard module).
- The QFN/LGA parts (MAX30102, TMP117, BME280, MPU6050) are the hard ones; use
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
- [ ] MAX30102 `VDD` on `+1V8`, `VLED+` on `+3V3`, both connected
- [ ] MAX30102 I2C confirmed 1.8 V-referenced; PCA9306 fitted
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

---

*SIH26181 · Project VALOR · Rev A wrist watch design intent · Not built, not
verified, not a medical device.*
