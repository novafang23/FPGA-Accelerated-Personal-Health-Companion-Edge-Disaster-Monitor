# Beginner's Guide to Understanding the Waveforms

*Simple analogies to connect waveforms with the health companion project*

---

## What Is a Waveform?

Think of a waveform like a **heart monitor in a hospital** — it shows voltage over time. Each line (signal) is like a separate monitor showing a different aspect of the system.

**In our project:** The waveform shows what happens inside the FPGA (the hardware accelerator) cycle by cycle, at 50 MHz (every 20 nanoseconds).

---

## The Two Platforms — Same Heart, Different Bodies

### Zynq-7000 (The "Hospital" Setup)
- **FPGA** = Specialized heart monitoring equipment
- **ARM Processor** = Doctor reading the monitors
- **AXI4-Lite Bus** = Medical chart where doctor writes orders & reads results

### ShrikeFi (The "Wearable" Setup)
- **ForgeFPGA** = Tiny heart monitor chip
- **ESP32-S3** = Smartwatch processor with WiFi/Bluetooth
- **4-Bit Link** = Ultra-thin wire (only 4 lines!) passing notes between them

---

## Signal Groups Explained Simply

### 1. Clock & Reset — The Metronome & Power Switch

| Signal | Analogy | What to See |
|--------|---------|-------------|
| `clk` / `s_axi_aclk` | Metronome ticking at 50 MHz (20 ns per tick) | Perfect square wave — everything else dances to this beat |
| `rst_n` / `s_axi_aresetn` | Power-on reset button | Starts low (reset), goes high (system alive) |

> **Beginner tip:** If clock isn't toggling, nothing works. If reset stays low, system never starts.

---

### 2. Moving Average Filter — The "Smoothing" Machine

**Real problem:** Raw PPG signal from finger sensor is noisy (motion, ambient light).

**Hardware solution:** Average last 8 samples to smooth it.

```
Raw:    ▁▃▅▇▅▃▁  ▂▄▆▄▂  ▁▃▅▇▅▃▁  ← Noisy heartbeat
Filter: ────────▅──────▄──────▅──  ← Clean heartbeat
```

**In waveform:**
- `reg_red_raw` / `reg_ir_raw` = Raw noisy samples coming in
- `red_filtered` / `ir_filtered` = Clean smoothed output
- **Magic:** Uses `>> 3` (divide by 8 via wire shift) — **zero multipliers, zero memory**

> **Why it matters:** This runs in **1 clock cycle** (20 ns). Software would take microseconds.

---

### 3. Peak Detector FSM — The "Heartbeat Spotter"

**Analogy:** A nurse watching the smoothed wave, tapping foot on each beat.

**4 States (like nurse's mental states):**
| State | Nurse Analogy | Waveform Signal |
|-------|---------------|-----------------|
| `ARMED` (00) | "Waiting for beat to rise" | `current_state = 0` |
| `RISING` (01) | "Going up... going up..." | `current_state = 1` |
| `PEAK` (10) | "TOP! Beat detected!" | `current_state = 2` + `beat_pulse` fires |
| `REFRACTORY` (11) | "Ignore for 250ms (can't have another beat this fast)" | `current_state = 3` |

**Key counters:**
- `interval_cnt` = Time since last beat (counts 20 ns ticks)
- `refractory_cnt` = Countdown timer (250 ms = 12.5M cycles)

> **In waveform:** Watch `current_state` cycle 0→1→2→3→0→1→2→3... each heartbeat!

---

### 4. Output: The "Doctor's Notes"

| Signal | Meaning | Units |
|--------|---------|-------|
| `hw_beat_pulse` | One-cycle pulse when beat found | Digital 0/1 |
| `irq_beat` | Interrupt to processor: "New beat!" | Digital 0/1 |
| `hw_ibi_cycles` | **Inter-Beat Interval** in clock cycles | Cycles × 20 ns |
| `reg_ibi_latched` (ShrikeFi) | IBI value latched for 4-bit link transfer | Cycles |

**Calculate heart rate:**
```
Heart Rate (BPM) = 60 / (ibi_cycles × 20ns)
                 = 60 / (ibi_cycles × 20 × 10⁻⁹)
                 = 3,000,000,000 / ibi_cycles
```
Example: `ibi_cycles = 50,000,000` → 60 BPM

---

### 5. Communication: How FPGA Talks to Processor

#### Zynq: AXI4-Lite (The Medical Chart)
```
Doctor (ARM)                    FPGA (Monitor)
   │                               │
   ├─ Write 0x0C (threshold) ────▶ │  "Set detection sensitivity"
   │                               │
   ├─ Read 0x04 (red_filtered) ◀───┤  "Give me clean red signal"
   ├─ Read 0x08 (ibi_cycles) ◀─────┤  "How long since last beat?"
   │                               │
```
**Waveform:** See `awaddr`/`wdata` (writes), `araddr`/`rdata` (reads) — handshakes like `valid`/`ready`

#### ShrikeFi: 4-Bit Link (The Tiny Wire)
Only **4 wires** + strobe + direction!

```
FPGA                    ESP32-S3
  │                       │
  ├─ link_dout[3:0] ────▶ │  Nibble 0: red_filtered[7:4]
  │  (strobe pulses)      │  Nibble 1: red_filtered[3:0]
  │                       │  Nibble 2: ir_filtered[7:4]
  │                       │  Nibble 3: ir_filtered[3:0]
  │                       │  Nibble 4-7: ibi_cycles[31:0] (32 bits = 8 nibbles)
  │                       │
  ◀─ link_din[3:0] ──────┤  Commands from ESP32 (threshold, config)
```
**Waveform:** Watch `link_dout[3:0]` shift nibbles, `link_strobe` pulse per nibble, `link_dir` shows direction

---

## How to Read the Waveform Like a Story

### Zynq Story (Open `presentation.gtkw`)
1. **Reset releases** → System wakes up
2. **ARM writes threshold** (0x0C) via AXI write channel
3. **Raw PPG samples arrive** → `reg_red_raw` changes
4. **Filter smooths** → `red_filtered` follows with 1-cycle lag
5. **Peak detector FSM cycles** → `current_state` 0→1→2→3→0...
6. **Beat found!** → `hw_beat_pulse` + `irq_beat` fire
7. **IBI captured** → `hw_ibi_cycles` updates
8. **ARM reads IBI** (0x08) via AXI read channel
9. **Scoreboard** → `tests_passed` increments

### ShrikeFi Story (Open `presentation.gtkw`)
1. **Reset releases** → Both sides wake up
2. **Link synchronizes** → `strobe_sync` aligns
3. **Raw PPG → Filter → Peak detect** (same as Zynq!)
4. **Beat found** → `peak_beat_detected` fires
5. **IBI latched** → `reg_ibi_latched` holds value
6. **4-bit link transmits** → `link_dout` shifts 12 nibbles (red + ir + ibi)
7. **ESP32 receives** → `link_din` gets nibbles
8. **Interrupt sent** → `irq_beat` to ESP32
9. **Scoreboard** → `tests_passed` increments

---

## Beginner Exercises

### Exercise 1: Count Heartbeats
1. Open Zynq waveform
2. Find `hw_beat_pulse` (green group)
3. Count rising edges in 2-second simulation
4. Calculate: `beats / 2s × 60 = BPM`

### Exercise 2: Measure Filter Latency
1. Find `reg_red_raw` and `red_filtered` (purple group)
2. Place cursor on raw sample change
3. Place cursor on filtered output change
4. Difference = **1 clock cycle (20 ns)**

### Exercise 3: Trace FSM States
1. Find `current_state` (red group)
2. Watch sequence: 0 → 1 → 2 → 3 → 0 → 1 → 2 → 3...
3. Each full cycle = **one heartbeat detected**

### Exercise 4: Decode 4-Bit Link (ShrikeFi)
1. Find `link_dout[3:0]` and `link_strobe` (magenta group)
2. Count strobe pulses per beat transfer
3. Should be ~12 nibbles (3 bytes red + 3 bytes ir + 4 bytes ibi + overhead)

---

## Connecting to the Big Picture

| Waveform Signal | Project Feature | Why It Matters |
|-----------------|-----------------|----------------|
| `red_filtered` | Clean PPG for SpO₂ | Noisy signal → wrong oxygen reading |
| `hw_ibi_cycles` | HRV (Heart Rate Variability) | 20 ns resolution → detects stress 15-30 min early |
| `irq_beat` | Real-time alert | Processor wakes only on beat, saves power |
| `link_dout` (ShrikeFi) | Ultra-low-power comms | 4 wires vs 100+ for AXI → wearable size |

---

## Files Reference

| File | Purpose |
|------|---------|
| `ppg_system.vcd` | Zynq simulation dump |
| `hardware/shrikefi/shrikefi_sim.vcd` | ShrikeFi simulation dump |
| `hardware/zynq/presentation.gtkw` | Zynq presentation config |
| `hardware/shrikefi/presentation.gtkw` | ShrikeFi presentation config |
| `docs/WAVEFORM_PRESENTATION_GUIDE.md` | Signal tables for presentations |
| `hardware/common/moving_average_8tap.v` | Filter RTL (readable!) |
| `hardware/common/ppg_peak_detector.v` | Peak detector RTL (readable!) |

---

## Next Steps

1. **Open both waveforms** side by side in GTKWave
2. **Compare filter outputs** — same RTL, different platforms
3. **Compare peak detector** — same FSM, different interfaces
4. **Read the RTL** — it's shorter than you think!
5. **Modify threshold** in testbench → see detection change

> **Remember:** Every signal in the waveform maps to a line of Verilog or C code. The waveform is just the code "running in slow motion" so humans can understand it.