# SIH26181 Master Theory Notes & Judge Defense Companion

**Project Title:** AI-Powered Personal Health Companion & Edge Disaster Monitor  
**Challenge:** Qualcomm Hardware Challenge — Smart India Hackathon 2026 (Problem SIH26181)  
**Target Platforms:** Xilinx Zynq-7000 (Reference Baseline) ➔ Renesas ForgeFPGA + ESP32-S3 ShrikeFi (Ultra-Low-Cost Wearable) ➔ Qualcomm Snapdragon Wear / QCS6490 (Commercial Road-map)

---

# 1. THE ULTIMATE PITCH GUIDE (Executive Summary)

If you have 3 minutes to pitch this to a panel of judges, hit these exact points in this order:

### 1. What are we solving? (The Problem)
In 2024 alone, extreme heatwaves in India claimed over 700 lives in just three months, while 380 million outdoor workers (75% of the national workforce) faced severe risk of heatstroke, acute kidney injury, and lost wages. Current early-warning systems only monitor the *environment* (e.g., weather apps or city air monitors), leaving vulnerable outdoor workers guessing how much physiological danger their own bodies are actually in.

### 2. How does it relate to the SIH Problem Statement? (Qualcomm Hardware Challenge)
Problem **SIH26181** calls for innovative hardware architectures leveraging Qualcomm platforms for societal impact. We built a hardware-accelerated, edge-AI wearable that acts as a personal disaster shield. It proves custom silicon offloading while solving a massive humanitarian crisis: protecting vulnerable populations during heatwaves, toxic smog, and flood immersion **100% offline with zero cloud dependency**.

*(HARDWARE WOW-FACTOR)*: Most hackathon health wearables run everything in software on a standard microcontroller. We went much deeper: we designed custom digital hardware circuits in synthesizable Verilog that filter noisy optical signals and extract heartbeat intervals physically at the silicon gate level, taking 0 DSP slices and 0 Block RAM.

### 3. Who is doing this in the world? (The Competitor Gap)
* **Fitness Wearables (Apple Watch / Garmin / Fitbit):** Track heart rate in air-conditioned gyms, but have zero awareness if you are standing in 46°C heat or toxic PM2.5 = 400 smog.
* **Environmental Monitors (PurpleAir / Weather Apps):** Measure ambient air quality, but don't know if *your* specific lungs or cardiovascular system are collapsing because of it. Furthermore, they depend on cellular networks that fail during disasters.

### 4. How are we different? (The "Tick in the Ass" Defense)
If asked why this isn't just another Arduino/Raspberry Pi project, use these 4 core differentiators:

1. **Custom Silicon vs. Software Loops:** Normal smartwatches read sensors using a C `while()` loop, suffering 5–20 ms of OS scheduling jitter that ruins Heart Rate Variability (HRV). We wrote **custom Verilog RTL** to move signal filtering and systolic peak detection directly into physical FPGA hardware. The crest is located to a **20-nanosecond clock edge** in synchronous logic, with no operating system in the loop — so the *detection* has no OS jitter at all. (Be precise about the next step: the interval between those hardware-detected beats is then timed by the ESP32's **1 µs** hardware timer, because the FPGA does not transmit a cycle count over the link. Detection: 20 ns, jitter-free. Interval timing: 1 µs, still 10,000× finer than the RTOS tick.)
2. **Edge AI (TinyML) vs. Lazy Cloud AI:** Most projects send data to AWS to run AI models—terrible for battery, privacy, and useless in a disaster zone. We compressed our Neural Network to **619 INT8 parameters (619 bytes of storage, 576 multiply-accumulates per inference)** to run entirely *on the physical device* — measured at **0.44 µs per inference on an x86-64 host**; the ESP32-S3 figure has not been measured — with zero cloud dependency.
3. **Predictive vs. Reactive:** Most monitors beep *after* a heat stroke. By fusing physiological data (HRV, SpO2) with environmental data (Temp, Humidity, PM2.5) in our TinyML model, we detect **Cardiovascular Drift** (heart rate rising while stroke volume drops) with the *intent* of warning ahead of a heat stroke. **Be precise here:** the 15–30-minute advance-warning window is a **hypothesis we have not yet validated** — `ROADMAP.md` lists thermal-chamber testing to validate it as future work. Present it as a design goal, not a delivered capability.
4. **Real-World Manufacturing Economics:** We didn't just build a prototype on an expensive $200 Zynq board. We ported the exact same hardware logic to the ultra-cheap ShrikeFi board ($5 ESP32 + $1 Renesas ForgeFPGA) over a **4-wire SPI link**, proving our medical-grade architecture can be mass-manufactured for pennies. (An older version of this pitch called that link a "custom 4-bit parallel protocol"; it never was — see section 3.)

### 5. Who is this for? (Target Demographics)
* **Frontline & Outdoor Workers:** Construction laborers, agricultural workers, traffic police, and disaster relief personnel.
* **Vulnerable Citizens:** Elderly individuals, asthma patients, and individuals with cardiovascular disease during extreme climate events.

### 6. The Vision (Closing the Pitch)
*"We have validated our custom hardware on the Xilinx Zynq-7000 baseline, deployed an ultra-affordable wearable architecture (prototype cost ~$51: ~$11.50 for the integrated ShrikeFi ForgeFPGA/ESP32 board + ~$40 sensors, projected <$20 at-scale bulk manufacturing BOM) on the ShrikeFi platform, and mapped the full migration path to Qualcomm Snapdragon Wear."*

---

# 2. THE DUAL-FPGA STRATEGY — WHY TWO FPGAs?

A common question from technical judges is: *"Why does your repository have both Xilinx Zynq-7000 and Renesas ForgeFPGA (ShrikeFi) implementations?"*

Here is the exact story and rationale to present:

```
+-----------------------------------------------------------------------------------+
|                            THE TWO-TARGET HARDWARE STORY                          |
+-----------------------------------------------------------------------------------+
|  1. XILINX ZYNQ-7000 (Reference Baseline)                                         |
|     - Role: High-End Prototyping & Cycle-Accurate Verification                    |
|     - Target Part: xc7z020clg400-1 (Vivado ML 2022.2)                             |
|     - Performance: Timing closed @ 69.45 MHz (WNS +5.603 ns, WHS +0.184 ns)       |
|     - Interconnect: 32-bit AXI4-Lite Memory-Mapped Bus                            |
|     - Purpose: Proved silicon math & zero DSP/BRAM footprint                      |
|                                                                                   |
|                                        ▼                                          |
|                                                                                   |
|  2. RENESAS ForgeFPGA + ESP32-S3 (ShrikeFi Target)                                |
|     - Role: Mass-Deployable, Ultra-Low-Cost (<$20 at-scale BOM) Pocket Wearable   |
|     - Target Part: SLG47910C (1120 5-input LUTs) + Dual-Core ESP32-S3             |
|     - Utilization: Only 363 / 1120 LUT5s (32.41%), 202 FFs, 75 CLBs, 0 DSP, 0 BRAM |
|     - Interconnect: Full-duplex 8-bit SPI, Mode 0 (1 MHz runtime; 16 MHz for FPGA configuration)|
|     - Purpose: Proved commercial feasibility for millions of workers              |
|                                                                                   |
|                                        ▼                                          |
|                                                                                   |
|  3. QUALCOMM SNAPDRAGON WEAR (Commercial Road-map)                                |
|     - Role: Mass-Market Integrated Smartwatch SoC (QCS6490 / Snapdragon Wear)     |
|     - Hardware Blocks: Hexagon DSP (PPG filtering) + NPU (INT8 TinyML Model)      |
+-----------------------------------------------------------------------------------+
```

### The "Formula 1 Rig vs. The Pocket Commuter" Analogy
* **Xilinx Zynq-7000 = The Formula 1 Wind Tunnel:** A heavy, multi-hundred-dollar development platform used by aerospace engineers to test aerodynamic wings under extreme conditions. It proved our Verilog RTL was mathematically flawless and met strict 20 ns timing resolution.
* **ShrikeFi (ForgeFPGA + ESP32-S3) = The Mass-Market Commuter:** You cannot hand a $250 development board to a construction worker in Delhi. ShrikeFi pairs a Renesas ForgeFPGA with an ESP32-S3 on a single integrated development board for ~$11.50. Combined with a ~$40 sensor suite, it delivers the exact same silicon-level acceleration for ~$51 (prototype), with a projected at-scale manufacturing BOM under $20.
* **Vendor-Agnostic Core:** We did **not** rewrite the filter or peak detector for ShrikeFi. The core Verilog math in `hardware/common/` is 100% vendor-agnostic and synthesized directly onto both Xilinx (6-input LUTs) and Renesas (5-input LUTs).

---

# 3. SYSTEM ARCHITECTURE & HARDWARE INTERCONNECTS

### 1. Zynq-7000: The 32-Bit AXI4-Lite Register Bus
On the Zynq SoC, the ARM Cortex-A9 CPU communicates with the FPGA fabric over an AMBA AXI4-Lite memory-mapped bus (`0x43C00000`):
* **Direct Register Map:** `REG_RED_RAW` (0x00), `REG_RED_FILTERED` (0x04), `REG_IBI_CYCLES` (0x08), `REG_STATUS_THRESH` (0x0C).
* **Decoupled Handshake:** Independent Address Write (`AW`) and Data Write (`W`) channels prevent interconnect deadlocks.
* **Write-1-to-Clear (W1C):** Bit [0] of `REG_STATUS_THRESH` latches when a heartbeat occurs and clears atomically on write, eliminating CPU race conditions.

### 2. ShrikeFi: The Full-Duplex 8-Bit SPI Link
The Renesas ForgeFPGA is a compact chip with limited I/O pins. A 32-bit AXI bus is physically impossible. The link is a **4-wire SPI bus in mode 0 (CPOL = 0, CPHA = 0), MSB first**, with the ESP32-S3 as controller and the ForgeFPGA as target:

> **Figure note:** `images/shrikefi_pinout.png` predates the SPI link and cannot be
> verified from the repository (it is a raster with no searchable labels). The pin
> table in [`../SHRIKEFI_LINK_PROTOCOL.md`](../SHRIKEFI_LINK_PROTOCOL.md) is
> authoritative; regenerate this figure from it before reuse.

* **Physical Wires (3.3V LVCMOS), ESP32 GPIO → FPGA pad:**
  * `SCK` — GPIO12 → `PIN_16` (`spi_sck`): SPI clock, generated by the ESP32-S3.
  * `SS_n` — GPIO10 → `PIN_17` (`spi_ss_n`): active-low chip select, **driven manually** around each transaction (`.spics_io_num = -1`).
  * `MOSI` — GPIO11 → `PIN_18` (`spi_mosi`): MCU → FPGA. Carries the raw 8-bit PPG sample.
  * `MISO` — GPIO13 → `PIN_19` (`spi_miso` + `PIN_19_OE`): FPGA → MCU. Carries `{beat_latched, filt_sample[6:0]}`.
  * Plus two non-SPI control lines: GPIO8 = FPGA enable (`EN`), GPIO9 = FPGA power (`PWR`). `led_user` is `PIN_7`; `OSC_EN` is the oscillator enable — a clock resource, not a GPIO.

> **Stale-protocol warning.** Earlier revisions of this section described a "4-bit parallel nibble link" with a `link_strobe`, a `link_dir`, a 4-bit `link_data` bus, a `fpga_irq` pin and a `0x1`–`0x8` command codebook. **That design was retired before it was ever built.** There is no strobe, no direction line, no separate interrupt pin, no command map and no command decode in the RTL or the firmware. Do not present it. If a judge asks why the docs contain it, the honest answer is: the interface was redesigned to SPI before implementation, and this file retained the old description for too long.

#### How a Sample Travels Across 4 Wires:
* **One transaction per sample, both directions at once.** SPI is full duplex, so there is no write phase and no read phase — the MCU shifts the sample out on MOSI while the FPGA shifts its reply back on MISO, on the same eight clock edges.
  1. MCU asserts `SS_n` low on GPIO10.
  2. MCU clocks 8 bits out on MOSI (the raw sample) and 8 bits in on MISO.
  3. MCU releases `SS_n` high.
  * The FPGA's `spi_target` module (`hardware/shrikefi/forgefpga_ppg_top.v`) shifts the received byte into `rx_data`, and the 8-tap moving-average filter consumes it on `rx_valid`.
* **The reply byte:**
  * Bit `[7]` = `beat_latched` — set by the peak detector on a systolic crest, cleared once it has been shifted out.
  * Bits `[6:0]` = the low seven bits of the 8-tap average. The FPGA's own detector uses the full 8-bit value internally, so detection is unaffected by the truncation.
  * **One transfer of pipeline latency:** the reply to transaction *k+1* carries the result of the sample sent in transaction *k*, because the filter, the beat latch and the transmit shift register each cost a clock.
* **There is no cycle count on the wire.** The peak detector holds a 32-bit `ibi_cycles` internally, but no SPI register carries it and the firmware never reads it. The MCU derives the inter-beat interval from its own `esp_timer_get_time()` deltas between rising beat flags.
* **Link Timing:**
  * **Runtime link (as shipped):** 1 MHz SPI, mode 0 — an 8-bit transaction takes **8 µs**, and one transaction runs per optical sample.
  * **FPGA configuration (as shipped):** the same four wires carry the 46,408-byte bitstream at **16 MHz**, streamed in 256-byte chunks with `SS_n` toggled per chunk.

> **Figure removed.** The waveform that used to appear here was drawn for the retired
> 4-bit parallel nibble bus and shows signals (`link_strobe`, `link_dir`, `irq_beat`,
> a 4-bit data bus) that do not exist in this design. The real frame is one 8-bit
> full-duplex SPI transaction per sample — see §3 and
> [`../SHRIKEFI_LINK_PROTOCOL.md`](../SHRIKEFI_LINK_PROTOCOL.md).

### 3. ESP32-S3 Dual-Core FreeRTOS Partitioning
The ESP32-S3 contains two Xtensa LX7 cores running at **160 MHz** (the configured frequency — `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ=160`; the LX7 is capable of 240 MHz but we do not run it there). We strictly partitioned the tasks using FreeRTOS core pinning:

```
+------------------------------------+    +------------------------------------+
|         CORE 0: ACQUISITION        |    |       CORE 1: INTELLIGENCE & UI    |
|   (The Dedicated Paramedic)        |    |      (The AI Chief Physician)      |
+------------------------------------+    +------------------------------------+
| • ~100 Hz MAX30102 Optical Sampling|    | • BME280 (Temp/Hum) & PMS5003      |
| • 8-Bit SPI Link Streaming         |    | • INT8 Quantized TinyML Inference  |
| • Beat-Flag Capture (1 µs timer)   |    | • Rule Engine (CTSI / PRSI Risk)   |
| • Rolling HRV Calculation (RMSSD)  |    | • SSD1306 OLED Real-Time Display   |
+------------------------------------+    +------------------------------------+
                  │                                         ▲
                  └─────── Thread-Safe Mutex Queue ─────────┘
```

---

# 4. THE CUSTOM VERILOG RTL — EXPLAINED FOR NON-HARDWARE PEOPLE

Digital hardware circuits do not "execute instructions" like C code or Python. They are physical arrays of logic gates, flip-flops, and wires operating simultaneously at 50,000,000 clock cycles per second.

```
                      RAW PPG SAMPLES (~100 Hz)
                                 │
                                 ▼
                     +───────────────────────+
                     |  moving_average_8tap  |  <--- 8-Tap O(1) Running Sum Filter
                     |  (No DSP, No BRAM)    |       Smooths wrist motion artifacts
                     +───────────────────────+
                                 │
                                 ▼
                     +───────────────────────+
                     |   ppg_peak_detector   |  <--- 4-State Systolic FSM
                     |   (20 ns Resolution)  |       Armed -> Rising -> Peak -> Refractory
                     +───────────────────────+
                                 │
                 ┌───────────────┴───────────────┐
                 ▼                               ▼
       [ ZYNQ-7000 TARGET ]            [ SHRIKEFI TARGET ]
    axi_ppg_accelerator.v              forgefpga_ppg_top.v
    (32-bit AXI4-Lite Bus)             (8-Bit SPI, Mode 0)
```

### The Core Hardware Modules & Architecture:

1. **`moving_average_8tap.v` (`hardware/common/` — The Smart Filter):**
   * *What it does:* Smooths high-frequency noise from skin contact and tremors on both Red and IR channels.
   * *Mathematical Formula:* $y[n] = y[n-1] + \frac{x[n] - x[n-8]}{8}$
   * *Why it's clever:* A standard filter adds 8 numbers every time. Our $O(1)$ running-sum filter only subtracts the oldest sample and adds the newest sample, using a simple bit-shift (`>> 3`) instead of a divider. Takes **0 DSP multiplier slices** and executes in **1 clock cycle (20 ns)**.

2. **`ppg_peak_detector.v` (`hardware/common/` — The Systolic Peak Radar):**
   * *What it does:* Tracks systolic pressure waves and locates each systolic crest to the nearest 50 MHz clock edge — **20 ns** — then counts 50 MHz cycles between crests in its internal 32-bit `ibi_cycles` register. **What it does not do is hand that count to the MCU:** no SPI register carries it, so on the shipped link the interval is timed by the ESP32's `esp_timer_get_time()` (1 µs) between rising beat flags.
   * *The 4-State Flowchart:*
     1. `STATE_ARMED (00)`: Waiting for the signal to rise above the dynamic threshold (`reg_threshold`).
     2. `STATE_RISING (01)`: Tracking the rising slope. To filter out quantization noise, it requires **two consecutive decreasing samples** before confirming the local maximum.
     3. `STATE_PEAK_FOUND (10)`: Captures the exact clock cycle count, sets `beat_detected` (which the top level latches into MISO bit 7), and resets the timer.
     4. `STATE_REFRACTORY (11)`: Enforces a 250 ms blanking window so the dicrotic notch does not trigger a false second heartbeat. Crucially, the signal must also **drop back below the dynamic threshold** before re-arming, preventing false triggers on elevated baselines.

3. **`axi_ppg_accelerator.v` (`hardware/zynq/` — The Zynq AXI4-Lite Wrapper):**
   * *What it does:* Memory-mapped AXI4-Lite slave wrapper for the Xilinx Zynq-7000 SoC (`0x43C00000`), providing register-level access (`REG_RED_RAW`, `REG_IBI_CYCLES`, `REG_STATUS_THRESH`) and Write-1-to-Clear interrupt management.

4. **`forgefpga_ppg_top.v` (`hardware/shrikefi/` — ShrikeFi Top-Level Gateway):**
   * *What it does:* Integrates an 8-bit SPI target (`spi_target`), the 8-tap moving-average filter, the systolic peak detector, and the beat-latch/SPI-response register inside the Renesas ForgeFPGA (SLG47910). **There is no link transceiver FSM, no command decoder and no nibble reassembler** — an older revision of this file claimed all three.
   * *Top-Level Sub-Blocks:*
     * **SPI Target (RX/TX):** Mode 0, MSB first. Shifts the received byte into `rx_data`/`rx_valid` for the filter, and shifts `{beat_latched, filt_sample[6:0]}` back out on MISO. Simple shift logic, not a protocol state machine.
     * **Beat Latch & Response Register:** Sets `beat_latched` when the peak detector fires and clears it once the byte has been transmitted, so the flag is guaranteed to reach the MCU exactly once per beat.
     * **Power-On Reset:** The ShrikeFi interconnect has **no reset pin** (nothing drives `PIN_13`), so the top level holds `rst_n` low for `POR_CYC` clocks from an internal counter instead of trusting an external line or the fabric's power-up state.
   * *Resource Footprint:* Uses only **363 out of 1120 LUT5s (32.41%)**, **202 Flip-Flops**, and **75 CLBs**, leaving ~68% of the LUT fabric free (CLB occupancy is 53.57%).

   > **Protocol note, for anyone reading an older copy of this file.** The **resource
   > figures above are current for the shipped SPI design.** Any version of this file
   > that mentions a nibble bus, a `link_strobe`, a `link_dir`, a `link_dout`, an
   > `fpga_irq` pin or a `CMD_*` opcode is carrying **retired 4-bit parallel-link
   > material that was never built** — the interface was redesigned to SPI before
   > implementation. Every such passage in this revision has been rewritten; if you
   > find one that has not, it is stale. See
   > `hardware/shrikefi/README.md` and `hardware/shrikefi/forgefpga_pins.pcf`.

![Renesas ForgeFPGA Workshop GUI Resources Report](../images/forgefpga_resources_report.png)

![Renesas SLG47910C Chip Top-Level Schematic](../images/forgefpga_chip_schematic.png)

![Renesas ForgeFPGA Resource Footprint](../images/forgefpga_utilization.png)

---

# 5. MASTER WAVEFORM ANALYSIS & HARDWARE TIMING GUIDE

A major difference between academic hackathon toys and real silicon engineering is **timing determinism**. This section provides a comprehensive, signal-by-signal dissection of our hardware waveforms for both platforms—giving you the exact explanation to deliver when a judge points at any signal on the oscilloscope or GTKWave simulation trace.

---

## 1. Xilinx Zynq-7000 Waveform (32-Bit AXI4-Lite Slave & Systolic Radar)

The Zynq simulation captures the full system lifecycle: AXI register configuration, decoupled bus handshaking, 8-tap running-sum noise filtering, and cycle-accurate heartbeat interval timestamping.

![Zynq AXI4-Lite Timing Waveform](../images/waveform_snapshot.png)

### Signal-by-Signal Breakdown Table:

| Signal Name | Direction / Type | Clock Domain | Engineering Role & Behavior in Waveform |
|---|:---:|:---:|---|
| **`clk`** | Input (50 MHz) | Core Clock | Master clock source ($T = 20.000\text{ ns}$). Every flip-flop in the accelerator updates on the rising edge of this clock. |
| **`rstn`** | Input (Active-Low) | Synchronous | System reset line. Held low for first 4 cycles ($t = 0 \to 80\text{ ns}$) to reset FSM states, clear sample FIFOs, and zero the interval timer. |
| **`s_axi_awvalid`** | Input (AXI AW) | `clk` | Address Write Valid. Asserted by CPU when sending a target register address (e.g., `0x0C` for threshold or `0x00` for PPG raw sample). |
| **`s_axi_awready`** | Output (AXI AW) | `clk` | Address Write Ready. Asserted by our accelerator to acknowledge that the address was latched into `aw_addr_latched`. |
| **`s_axi_wvalid`** | Input (AXI W) | `clk` | Data Write Valid. Asserted by CPU when presenting 32-bit write data on `s_axi_wdata`. |
| **`s_axi_wready`** | Output (AXI W) | `clk` | Data Write Ready. Asserted by accelerator to acknowledge that write data was latched into `w_data_latched`. |
| **`filter_red_out[7:0]`** | Internal / Reg | `clk` | Continuous 8-tap moving-average output. Shows optical noise reduction as raw stepped input samples are smoothed into a clean physiological pressure wave. |
| **`fsm_state[1:0]`** | Internal / State | `clk` | Systolic Peak Radar state: `ARMED (00)` $\to$ `RISING (01)` $\to$ `PEAK_FOUND (10)` $\to$ `REFRACTORY (11)`. |
| **`irq_beat`** | Output (Direct Pin) | `clk` | Hardware Interrupt Pulse. Generates an exact **1-clock-cycle pulse (20 ns)** at $t = 65\text{ cycles}$ the instant the wave reaches local maximum (`sample < prev_sample`). |
| **`reg_ibi_cycles[31:0]`** | Output / Reg | `clk` | Inter-Beat Interval (IBI) register. Latches the counter value (`0x00000CD1` = 3281 clock ticks = $65.62\text{ µs}$) and resets interval timer to zero. |

### Two Critical Engineering Mechanisms to Explain to Judges:

#### A. The Decoupled AXI Write Handshake (Anti-Deadlock Architecture)
* **The Problem:** In standard AXI4-Lite, if a slave expects address (`AW`) and data (`W`) in a rigid order, an out-of-order interconnect crossbar will stall and hang the entire ARM bus.
* **Our Solution:** Notice in the waveform that `s_axi_awvalid` pulses first at $t = 12$, but `s_axi_wvalid` is staggered and arrives 12 cycles later at $t = 24$.
* **How It Works in Silicon:** Internal register flags `aw_done` and `w_done` latch independently. Only when both flags are high does `write_execute` fire, updating the target register and returning `s_axi_bvalid = 1` (OKAY response).

#### B. The 4-State Systolic Peak Detector (Jitter-Free Peak Detection)
* **State 00 (`STATE_ARMED`):** The wave is below the programmable threshold (`reg_threshold = 120`). The detector ignores all baseline motion noise.
* **State 01 (`STATE_RISING`):** The filtered PPG signal crosses above threshold 120. The FSM tracks the rising slope and requires **two consecutive decreasing samples** to confirm a peak, eliminating single-sample noise glitches.
* **State 10 (`STATE_PEAK_FOUND`):** The true peak is confirmed. The FSM asserts `irq_beat = 1`, copies `interval_cnt` to `ibi_cycles`, and resets the timer.
* **State 11 (`STATE_REFRACTORY`):** The FSM enters a **250 ms blanking window** (`REFRACTORY_CYC = 12,500,000` cycles at 50 MHz). The signal must also completely drop back below the threshold before re-arming, preventing false triggers.

---

## 2. Renesas ForgeFPGA Waveform (ShrikeFi 8-Bit SPI Link)

On the ultra-low-cost ShrikeFi platform there is no 32-bit bus. The ESP32-S3 and the ForgeFPGA exchange **one 8-bit full-duplex SPI transaction per optical sample** — mode 0 (CPOL = 0, CPHA = 0), MSB first, chip select driven manually by firmware.

> **Figure removed.** The waveform that used to appear here was drawn for the retired
> 4-bit parallel nibble bus and shows signals (`link_strobe`, `link_dir`, `irq_beat`,
> a 4-bit data bus) that do not exist in this design. The real frame is one 8-bit
> full-duplex SPI transaction per sample — see §3 and
> [`../SHRIKEFI_LINK_PROTOCOL.md`](../SHRIKEFI_LINK_PROTOCOL.md).

### Signal-by-Signal Breakdown Table:

| Signal Name | FPGA Pad | MCU Pin | Role & Signal Dynamics in Simulation |
|---|:---:|:---:|---|
| **`clk`** | — (`OSC_EN`) | — | 50 MHz internal FPGA oscillator ($T = 20.000\text{ ns}$). `clk_en` drives `OSC_EN`; without it the core has no clock at all. |
| **`rst_n`** | *(no pin)* | — | **Internal, not a pin.** Generated by the top level's power-on counter (`por_cnt == POR_CYC`). There is no reset pad on the ShrikeFi interconnect — nothing drives `PIN_13`. |
| **`spi_sck`** | PIN_16 | GPIO12 | SPI clock generated by the ESP32-S3. Mode 0: MOSI/MISO are sampled on the rising edge, outputs shift on the falling edge. |
| **`spi_ss_n`** | PIN_17 | GPIO10 | Active-low chip select, **driven manually** by firmware around each 8-bit transaction. Held high between samples. |
| **`spi_mosi`** | PIN_18 | GPIO11 | MCU $\to$ FPGA: the raw 8-bit PPG sample, MSB first. |
| **`spi_miso`** | PIN_19 (+`PIN_19_OE`) | GPIO13 | FPGA $\to$ MCU: `{beat_latched, filt_sample[6:0]}`, MSB first. |
| **`filt_sample[7:0]`** | Internal | — | Real-time 8-tap moving-average output inside the fabric. Only `[6:0]` reaches the wire; the FPGA's own detector uses all 8 bits. |
| **`led_user`** | PIN_7 (+`PIN_7_OE`) | — | Blue user LED D12, held high for `LED_PULSE_CYC` (50 ms) after each detected beat. |

### Step-by-Step Protocol Walkthrough (Trace Anatomy):

#### Phase 1: Writing a Raw PPG Sample (8 SCK pulses = 8 µs at 1 MHz)
1. MCU asserts `spi_ss_n` low on GPIO10.
2. Eight `spi_sck` pulses. On every pulse the MCU shifts one bit out on MOSI **and** samples one bit in on MISO — one transaction, both directions.
3. For sample byte `0x78` (120), MOSI carries `0 1 1 1 1 0 0 0`, MSB first.
4. On the last rising edge, `spi_target` latches `rx_data = 0x78` and pulses `rx_data_valid` for one clock; the 8-tap moving-average filter consumes it.
5. MCU deasserts `spi_ss_n` high.

#### Phase 2: Hardware Beat Capture & the One-Bit Interrupt
* Filtered waveform reaches the systolic crest $\to$ peak-detector FSM sees the slope inversion $\to$ sets `beat_detected` $\to$ the top level sets `beat_latched <= 1'b1`.
* **The beat is not a separate wire.** It becomes **bit 7** of the very next MISO byte, `{beat_latched, filt_sample[6:0]}`. Because the MCU is already clocking that byte in, catching the beat costs nothing extra.
* `beat_latched` clears once the byte has been transmitted, so each beat appears on exactly one frame — no edge races, and no sticky interrupt register to acknowledge.

#### Phase 3: Reading the Filtered Sample and the Beat Flag
1. The reply to transaction *k+1* carries the result of the sample sent in transaction *k* — one transfer of pipeline latency, because the filter, the beat latch and the transmit shift register each cost a clock.
2. The link returns **only** bit 7 plus `filt_sample[6:0]`. The RTL's internal 32-bit `ibi_cycles` register (the `0x00000CD1` = 3281 value shown in the Zynq waveform above) **is never transmitted on ShrikeFi**.
3. Inter-beat intervals are therefore timed by the MCU: `shrikefi_link_driver.c` records `esp_timer_get_time()` on each **rising** beat flag and differences consecutive readings, multiplying by 50 to express the result in 20 ns ticks (`s_sim_ibi = delta_us * 50`).

> **Be precise about granularity here.** *Detection* happens in hardware at 50 MHz, so the crest is located to a 20 ns clock edge and there is no scheduler jitter in the detection. The *interval the firmware reports* is quantised by the MCU's **1 µs** timer. Claiming a 20 ns timestamp on the wire would be wrong, and a judge who knows SPI will catch it.

---

## 3. "POINT-AND-SHOOT" JUDGE DEFENSE CHEAT-SHEET

When presenting your timing diagrams, judges will probe your understanding with specific questions. Use these battle-tested responses:

### Q1: "Point to the exact moment a heartbeat is detected on the waveform."
* **Point to:** The rising edge of the latched beat flag — the frame where MISO bit 7 goes high. On the Zynq waveform, the equivalent is the `irq_beat` register asserting where `filter_red` crests and begins to decrease.
* **Say:** *"Right here at $t = 65\text{ cycles}$. Notice that `filter_red_out` reached its local maximum, and we waited for exactly **two consecutive dropping samples** to ensure it wasn't just quantization noise. Once confirmed, the systolic FSM set the beat flag, and the ShrikeFi top level latches it into MISO bit 7 so the MCU picks it up on the byte it is already clocking in."*

### Q2: "Why does the AXI waveform show Address Write (`AW`) and Data Write (`W`) at different times?"
* **Point to:** `s_axi_awvalid` pulsing at $t = 12$ and `s_axi_wvalid` pulsing at $t = 24$.
* **Say:** *"That is our decoupled AXI4-Lite handshake. Modern ARM AXI crossbars do not guarantee that address and data arrive simultaneously. By using independent `aw_done` and `w_done` status registers, our hardware guarantees zero deadlocks regardless of bus latency."*

### Q3: "How does the FPGA know whether a byte is a command or data?"
* **Say:** *"It doesn't, because there are no commands. The ShrikeFi link has no command map: every transaction is one 8-bit sample out on MOSI and one reply byte back on MISO, `{beat flag, filtered sample}`. No opcode, no register address, no length negotiation. That is deliberate — with exactly one thing to say in each direction, a protocol would be pure overhead. Our earlier documentation described a 4-bit link with a `0x1`–`0x8` command codebook; that design was retired before it was implemented, which is why you may have seen it in an older deck."*

### Q4: "Why do you need hardware timing when human heart rate is only ~1 Hz (60 BPM)?"
* **Say:** *"Heart Rate Variability (HRV) analysis — specifically **RMSSD** for parasympathetic stress detection — needs sub-millisecond precision between consecutive beats. If a FreeRTOS task detects the beat, the reading is quantised to the 10 ms scheduler tick plus Wi-Fi and flash-cache jitter, and that error is larger than the signal. So the FPGA detects the crest in synchronous logic at 50 MHz with no scheduler in the loop, and the ESP32 times the gap between hardware-set beat flags with its 1 µs hardware timer. Detection is jitter-free; interval timing is 10,000× finer than the RTOS tick. We are careful **not** to claim the FPGA hands over a 20 ns timestamp, because it does not."*

### Q5: "What is the real measured speed vs. theoretical maximum speed of your link?"
* **Say:** *"The link in the shipped firmware is 8-bit SPI at **1 MHz, mode 0**, with chip select driven manually: 8 µs per full-duplex transaction, one transaction per optical sample, so the link is idle for more than 99% of the sampling period. The same four wires carry the 46,408-byte FPGA bitstream at **16 MHz** during configuration. There is no bit-banged bring-up driver in the shipped code — that belonged to the retired parallel link."*

---

# 6. THE SENSOR SUITE — WHAT EACH SENSOR DOES & WHY

| Sensor | Vital Metric Measured | Why It Is Essential for Disasters |
|---|---|---|
| **MAX30102** | Red & IR Photoplethysmography (PPG), Blood Oxygen (SpO2), On-Chip Die Temperature | Detects heart rate, Heart Rate Variability (HRV), hypoxemia from smoke inhalation, and skin microclimate cooling during flood immersion. |
| **BME280** | Ambient Temperature (°C) & Relative Humidity (%) | Calculates the Steadman Heat Index. High humidity prevents sweat evaporation, causing lethal core temperature spikes even at 38°C. |
| **PMS5003** | Fine Particulate Matter (PM2.5 in $\mu\text{g}/\text{m}^3$) | Measures microscopic smoke and dust particles small enough to penetrate lungs into the bloodstream during crop fires, smog, and wildfires. |
| **SSD1306** | 128×64 Monochrome OLED Display | Provides real-time visual triaging (Green/Yellow/Orange/Red) directly on the worker's wrist without needing a paired phone or cellular signal. |

---

# 7. THE SOFTWARE & EDGE AI (TinyML) SIDE

Our firmware features a **Hybrid Risk Assessment Engine** combining deterministic clinical rules with an ultra-fast INT8 Quantized Neural Network.

```
       [ 6-INPUT SENSOR FUSION VECTOR ]
  ┌───────────────────────────────────────────────┐
  │ 1. Heart Rate (BPM)    4. Ambient Temp (°C)   │
  │ 2. HRV RMSSD (ms)      5. Relative Hum (%)    │
  │ 3. SpO2 Saturation (%) 6. PM2.5 (ug/m3)       │
  └──────────────────────┬────────────────────────┘
                         │
        ┌────────────────┴────────────────┐
        ▼                                 ▼
┌──────────────────────────────┐  ┌──────────────────────────────┐
│     CLINICAL RULE ENGINE     │  │   INT8 QUANTIZED TinyML NN   │
│  (disaster_risk_engine.c)    │  │   (nn_risk_model_int8.c)     │
├──────────────────────────────┤  ├──────────────────────────────┤
│ • Cardio-Thermal Strain (CTSI│  │ • 6 -> 24 -> 16 -> 3          │
│ • Pollution Strain (PRSI)    │  │ • INT8: 619 params, 576 MACs │
│ • Deterministic Safety Net   │  │ • 0.44 µs/inference (x86-64) │
└──────────────┬───────────────┘  └──────────────┬───────────────┘
               │                                 │
               └───────────────┬─────────────────┘
                               ▼
            [ UNIFIED TRIAGE & ADVISORY OUTPUT ]
            • Normal (Green)     • High (Orange)
            • Moderate (Yellow)  • Critical (Red Alert)
```

### 1. The Clinical Rule Engine (`disaster_risk_engine.c`)
Calculates medical indices developed specifically for occupational heat and smog strain:
* **Cardio-Thermal Strain Index (CTSI):** Combines ambient heat index with cardiovascular drift ($\Delta\text{HR}$) and HRV collapse. If Heat Index > 54°C and HR > 130 BPM with RMSSD < 10 ms, flags **CRITICAL HEAT RISK**.
* **Pollution Respiratory Strain Index (PRSI):** Fuses PM2.5 particulate concentration with blood oxygen desaturation. If PM2.5 > 300 $\mu\text{g}/\text{m}^3$ and $\text{SpO}_2 < 88\%$, flags **CRITICAL POLLUTION RISK**.

### 2. INT8 Quantized Neural Network (`nn_risk_model_int8.c`)
* **Architecture:** Multi-Layer Perceptron — **6 input neurons $\rightarrow$ 24 hidden neurons (ReLU) $\rightarrow$ 16 hidden neurons (ReLU) $\rightarrow$ 3 output neurons (Sigmoid)** — producing Heat, Pollution and Flood risk scores. Total **619 INT8 parameters, stored in 619 bytes**, and **576 multiply-accumulates per inference** (6×24 + 24×16 + 16×3 = 144 + 384 + 48).
  * **Correcting an older claim:** previous revisions of this file described a **6 $\rightarrow$ 12 $\rightarrow$ 3** network with "123 bytes". That was a superseded single-hidden-layer model. The shipped architecture is 6 → 24 → 16 → 3, as declared in `firmware/core/nn_risk_model.h` (`NN_HIDDEN1_SIZE 24`, `NN_HIDDEN2_SIZE 16`) and in `train_nn_risk_model.py` (`IN, H1, H2, OUT = 6, 24, 16, 3`). If you quote 123 bytes you are quoting a model that is not in the repository.
* **INT8 Quantization:** Weights use symmetric quantization with zero-point 0; activations use asymmetric uint8 with per-tensor scales and zero-points. The kernel stores weights as bytes but **dequantises to float for the multiply-accumulate**, so "619 bytes" describes *storage*, not a float-free compute core — do not claim "zero floating point in core".
* **Measured inference latency:** **0.44 µs per inference, measured on an x86-64 host at `-O2`.** That is the **only** figure anyone has measured. The ESP32-S3 target has *not* been timed, so do not say "under 1 microsecond on an ESP32-S3 or ARM CPU" — an older revision of this file did, and it was an assumption presented as a measurement.
* **Knowledge Distillation Training:**
  * The deterministic Rule Engine served as the **"Teacher"**.
  * `train_nn_risk_model.py` synthesises **70,000 scenarios** — 40,000 uniform, 10,000 clustered on class-boundary thresholds, 12,000 around named disaster archetypes, and 8,000 on the derived heat-index edges — then holds out **7,000 for validation** (a 90/10 split; the script prints `Dataset: 70000 samples (63000 train / 7000 val)`). Older drafts quoted "62,000 scenarios" and a "6,200-scenario" validation set; those numbers are not in the script.
  * Training runs for **1200 epochs by default** (`--epochs`, default 1200) — not "over 100 epochs".
  * Achieved **88.47% classification accuracy** on the held-out validation set. Be precise about what that means: the labels come from the same rule engine that generated the training labels, so this measures agreement with our own teacher, not clinical accuracy.

---

# 8. KILLER ANALOGIES CHEAT-SHEET (For Judges & Team Defense)

Use these exact analogies when explaining the project to non-technical judges or team members:

### 1. The "CEO and the Automated Assembly Line" (Why FPGA Acceleration?)
> *"Think of the microcontroller CPU as a brilliant CEO. If you force the CEO to manually inspect a hundred raw optical readings every second, they have to stop everything, do math, and burn battery. They become overwhelmed and have no time to make big decisions.  
> Instead, we used the FPGA fabric to build an **automated factory conveyor belt**. The raw light signals pass through our hardware filter and peak detector, which smooth the wave and flag each systolic crest in silicon — the crest is located to a 20 ns clock edge with no operating system in the loop. The belt hands the CEO one byte per sample with a beat flag on top; the CEO then reads its own microsecond timer to get the interval and says: 'Heart rate is 128, IBI is 468 ms.' Its entire brain is free to run our TinyML Neural Network."*

### 2. The "Formula 1 Wind Tunnel vs. The Pocket Commuter" (Why Two FPGAs?)
> *"The Xilinx Zynq-7000 was our Formula 1 wind tunnel: an industrial-grade testing platform where we proved our custom Verilog circuits were mathematically sound and closed timing at 69.45 MHz.  
> But you can't give a $250 development board to an agricultural worker in rural Bihar. The ShrikeFi platform (Renesas ForgeFPGA + ESP32-S3) is our production pocket commuter: it runs the exact same Verilog filter logic on a $1.50 micro-FPGA, projecting the entire at-scale device cost under $20 while retaining hardware acceleration."*

### 3. The "Smart Elevator Scale" (Why is the Filter $O(1)$?)
> *"Imagine calculating the average weight of 8 people inside an elevator. When a new person steps in and the oldest person leaves, a naive algorithm asks all 8 people to step on the scale again, adds up their weights, and divides by 8. That wastes time.  
> Our $O(1)$ hardware filter acts like a smart scale: it remembers the previous running sum, subtracts the weight of the person who left, and adds the new person. It does this in a single 20-nanosecond clock cycle, regardless of how large the filter window is."*

### 4. The "Two-Way Note Pass" (How the ShrikeFi SPI Link Works)
> *"On large chips, components talk over 32 parallel copper tracks — a 32-lane highway. On our compact micro-FPGA we have four wires: clock, chip-select, and one lane each way. But it's a two-way street: the ESP32 sends the sensor sample out on one wire at the same moment the FPGA sends its answer back on the other. Eight clock ticks and both have exchanged a full byte, in a single transaction. No commands, no handshake syllables — one byte out, one byte back, eight microseconds, and the heartbeat is one bit inside the byte that was coming back anyway."*

### 5. The "Paramedic & The AI Chief Physician" (Dual-Core FreeRTOS Partitioning)
> *"On the ESP32-S3, Core 0 is our dedicated paramedic in the ambulance: it never leaves the patient while sampling optical vitals, and it exchanges one SPI frame per sample with the FPGA without ever dropping a beat.  
> Core 1 is the Chief Physician at the hospital: it takes the clean data, gathers environmental readings, runs the AI Neural Network, calculates disaster risk scores, and updates the wrist display."*

### 6. The "Teacher and the Student" (How the AI was Trained)
> *"Our clinical Rule Engine is a strict medical professor: consistent and inspectable, but heavy and slow to calculate.  
> Our Neural Network is an eager student. We gave the student 70,000 flashcards of extreme heatwaves and toxic smog scenarios — including 12,000 built around named disasters and 8,000 sitting exactly on the decision edges — and held 7,000 back as an unseen exam. The professor graded every flashcard. By learning from its mistakes over up to 1200 epochs of gradient descent, the student learned to make the same triage decisions in 619 bytes."*

---

# 9. LIKELY JUDGE QUESTIONS & WINNING DEFENSE ANSWERS

### Q1: "Why did you build custom Verilog RTL instead of just doing everything in C on the ESP32-S3?"
* **Answer:** *"Three reasons: Determinism, CPU offloading, and Power. Software signal processing on an RTOS suffers from interrupt latency and task jitter when WiFi or sensor interrupts fire. Our FPGA accelerator processes raw PPG samples at hardware clock speeds with zero CPU overhead, allowing the microcontroller to stay in low-power sleep modes between assessments."*

### Q2: "How did you manage to fit your design into Renesas ForgeFPGA's tiny 1120 LUT capacity?"
* **Answer:** *"Our architecture was designed from day one to be ultra-lean. By using an $O(1)$ running-sum filter with bit-shift division and an accumulator-based peak detector, our entire ShrikeFi design consumes **363 of 1120 LUT5s (32.41%)** and **202 flip-flops**, with zero DSP multiplier blocks, zero Block RAM, and zero PLL."*

### Q3: "Why SPI rather than I2C for the FPGA link?"
* **Answer:** *"Because the link is exactly what SPI is for, and it is the one bus the FPGA and the MCU already both have. I2C would need addressing and register semantics for a device that has no registers — we have one 8-bit value out and one 8-bit value back, per sample. SPI gives us that as a shift register: mode 0, MSB first, an 8-bit full-duplex transaction in 8 µs at 1 MHz, with chip select held low around each sample. There is no command map, no strobe, no direction turnaround and no separate interrupt pin — the beat flag rides in bit 7 of the byte we were already reading, and the ESP32-S3's hardware SPI peripheral means the CPU cost per sample is essentially zero. The same four wires also carry the FPGA bitstream at 16 MHz during configuration."*
  * **Correction to an older answer:** this question used to be *"What makes your 4-bit link better than standard SPI or I2C?"* with an answer arguing that a "custom 4-bit parallel nibble link" beat SPI. That was wrong on two counts: the 4-bit link was retired before it was built, and the design **is** SPI. Never argue against the bus you are actually using.

### Q4: "What happens if the PM2.5 air quality sensor gets clogged or malfunctions?"
* **Answer:** *"Our system uses cross-sensor validation. If the PMS5003 reports hazardous PM2.5 = 500 but the user's blood oxygen is a healthy 99% and heart rate is 65 BPM, our sensor fusion engine recognizes the biometric mismatch, suppresses panic sirens, and flags a 'Sensor Check Advisory' on the OLED display."*

### Q5: "How does this project transition to Qualcomm Snapdragon Wear?"
* **Answer:** *"Our architecture is explicitly decoupled for platform migration. The Verilog DSP pipeline in `hardware/common/` maps directly onto Qualcomm's Hexagon DSP core, while our C-based INT8 Neural Network executes natively on the Qualcomm Snapdragon Neural Processing Engine (SNPE) / NPU without architectural changes."*

---

# 10. CLINICAL REFERENCES & SCIENTIFIC FOUNDATIONS

| Clinical Index | Scientific Source | Formulation & Thresholds in SIH26181 |
|---|---|---|
| **Steadman Heat Index** | Robert G. Steadman (1979), *"The Assessment of Sultriness"*, Journal of Applied Meteorology. | Combines Ambient Temp + Humidity. Index $> 35^\circ\text{C}$ triggers Caution, $> 40^\circ\text{C}$ triggers Moderate; $> 54^\circ\text{C}$ triggers Critical Heat Hazard. |
| **Cardio-Thermal Strain (CTSI)** | Moran et al., Physiological Strain Index (PSI) adapted for multi-sensor edge wearables. | Fuses Heat Index with Cardiovascular Drift ($\text{HR} > 130\text{ BPM}$) and HRV collapse ($\text{RMSSD} < 10\text{ ms}$). |
| **Pollution Respiratory Strain (PRSI)** | WHO Global Air Quality Guidelines (2021) & EPA AQI Technical Assistance Document. | Fuses $\text{PM2.5} > 300\,\mu\text{g}/\text{m}^3$ (EPA "Hazardous" ceiling) with clinical hypoxemia ($\text{SpO}_2 < 88\%$). |
| **Hypothermia & Cold Shock** | Golden & Tipton, *"Essentials of Sea Survival"*, Cold Water Immersion Stages. | Tracks skin temperature proxy $< 32^\circ\text{C}$ (High) and $< 28^\circ\text{C}$ (Critical) combined with initial cold-shock tachycardia followed by severe bradycardia ($\text{HR} < 50\text{ BPM}$). |

---

# 11. GLOSSARY OF TERMS

* **AXI4-Lite:** Advanced eXtensible Interface; standard memory-mapped point-to-point bus protocol for ARM SoC chips.
* **CTSI:** Cardio-Thermal Strain Index; our custom index quantifying physiological heat stress.
* **FSM:** Finite State Machine; a sequential digital hardware circuit transitioning between distinct operating states.
* **IBI:** Inter-Beat Interval; the elapsed time between consecutive systolic peaks. On ShrikeFi it is **timed by the MCU's 1 µs `esp_timer_get_time()`** between hardware-set beat flags, and reported in 20 ns tick units (`shrikefi_link_driver.c` multiplies the µs delta by 50). The FPGA locates the peak at 20 ns resolution but does not transmit a cycle count.
* **INT8 Quantization:** Compressing 32-bit floating-point neural network weights into 8-bit signed integers for compact storage and fast integer ALU execution.
* **Nibble:** A 4-bit aggregation of binary data (half an 8-bit byte). **Listed for completeness only — the shipped ShrikeFi link does not use nibbles.** It is 8-bit SPI.
* **PRSI:** Pollution Respiratory Strain Index; our custom index quantifying respiratory distress during smoke/smog events.
* **RMSSD:** Root Mean Square of Successive Differences; the clinical gold standard for measuring parasympathetic Heart Rate Variability (HRV).
* **SoC:** System on Chip; an integrated circuit combining CPU cores, memory, peripherals, and FPGA fabric.
* **SPI (Serial Peripheral Interface):** A synchronous 4-wire serial bus — clock (`SCK`), chip select (`SS_n`), and one data line each way (`MOSI`/`MISO`). A transaction transfers data in both directions simultaneously. The ShrikeFi MCU↔FPGA link is SPI mode 0 (CPOL = 0, CPHA = 0), 8-bit, MSB first.

