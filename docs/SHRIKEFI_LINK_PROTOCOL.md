# ShrikeFi FPGA-MCU Link Protocol Specification

## 1. Overview
This document specifies the communication protocol between the **ESP32-S3 microcontroller** and the **Renesas ForgeFPGA (SLG47910)** accelerator on the **ShrikeFi** board.

Because the ForgeFPGA has no hard AXI bus or ARM processing system, all register operations are packetized across a **synchronous 4-bit parallel nibble link**, complemented by a dedicated hardware interrupt line.

```
  ESP32-S3 (Host MCU)                              Renesas ForgeFPGA
 ┌───────────────────┐                            ┌───────────────────────┐
 │                   │ ─── link_strobe (GPIO4) ─▶ │                       │
 │                   │ ─── link_dir    (GPIO5) ─▶ │ 4-Bit Link            │
 │                   │ ◀── link_din/dout(GPIO6-9)▶│ Transceiver Engine    │
 │                   │                            │          │            │
 │                   │ ◀── irq_beat   (GPIO10) ── │ ◀────────┘            │
 └───────────────────┘                            └───────────────────────┘
```

> [!WARNING]
> **3.3V ELECTRICAL WARNING:**  
> All FPGA and MCU I/O pins operate at **3.3V LVCMOS ONLY**. Applying 5V logic signals will permanently destroy both the ForgeFPGA and ESP32-S3 ICs.

---

## 2. Physical Interface & Pin Assignment

![ShrikeFi Pinout & Interconnect Diagram](images/shrikefi_pinout.png)

| Signal Name | FPGA Pin | ESP32-S3 GPIO | Direction (FPGA perspective) | Description |
|---|:---:|:---:|:---:|---|
| **`clk`** | PIN_12 | — | Input | 50 MHz FPGA system clock (on-chip oscillator) |
| **`rst_n`** | PIN_13 | GPIO 3 | Input | Active-Low system reset |
| **`link_strobe`** | PIN_14 | GPIO 4 | Input | Clock strobe driven by ESP32 on each nibble transfer |
| **`link_dir`** | PIN_15 | GPIO 5 | Input | Bus direction: `0` = ESP32 Write $\to$ FPGA, `1` = ESP32 Read $\leftarrow$ FPGA |
| **`link_data[0]`** | PIN_16 | GPIO 6 | Inout / Bidirectional | Data bit 0 (LSB) |
| **`link_data[1]`** | PIN_17 | GPIO 7 | Inout / Bidirectional | Data bit 1 |
| **`link_data[2]`** | PIN_18 | GPIO 8 | Inout / Bidirectional | Data bit 2 |
| **`link_data[3]`** | PIN_19 | GPIO 9 | Inout / Bidirectional | Data bit 3 (MSB) |
| **`irq_beat`** | PIN_24 | GPIO 10 | Output | Active-High interrupt asserted on systolic peak detection |

---

## 3. Data Framing & Command Map

Every transaction begins with a **1-nibble (4-bit) Command Header** sent while `link_dir = 0`:

| Command Code (`link_din[3:0]`) | Command Name | Payload Nibbles | Description |
|:---:|---|:---:|---|
| `0x1` | `CMD_WRITE_RED` | 2 (Write) | Write 8-bit raw Red sample (`[7:4]`, `[3:0]`) |
| `0x2` | `CMD_WRITE_IR` | 2 (Write) | Write 8-bit raw IR sample (`[7:4]`, `[3:0]`) |
| `0x3` | `CMD_WRITE_THRESH` | 2 (Write) | Write 8-bit systolic peak threshold (`[7:4]`, `[3:0]`) |
| `0x4` | `CMD_READ_RED` | 2 (Read) | Read 8-bit filtered Red output (`[7:4]`, `[3:0]`) |
| `0x5` | `CMD_READ_IR` | 2 (Read) | Read 8-bit filtered IR output (`[7:4]`, `[3:0]`) |
| `0x6` | `CMD_READ_IBI` | 8 (Read) | Read 32-bit IBI cycle timestamp across 8 nibbles |
| `0x7` | `CMD_CLEAR_IRQ` | 0 | Clear latched `irq_beat` interrupt (Write-1-to-Clear) |
| `0x8` | `CMD_READ_STATUS` | 1 (Read) | Read 4-bit status: `[0]=irq_beat`, `[1]=filter_valid` |

### Transaction Examples:

#### A. Write 8-Bit PPG Sample (`0x78` = 120):
1. `link_dir = 0` (Write mode).
2. `link_din = 0x1` (`CMD_WRITE_RED`) $\rightarrow$ Pulse `link_strobe`.
3. `link_din = 0x7` (High Nibble) $\rightarrow$ Pulse `link_strobe`.
4. `link_din = 0x8` (Low Nibble) $\rightarrow$ Pulse `link_strobe`.  
*Total transaction time: 3 strobe cycles ($\approx 300\text{ ns}$).*

#### B. Read 32-Bit IBI Timestamp:
1. `link_dir = 0` (Write mode).
2. `link_din = 0x6` (`CMD_READ_IBI`) $\rightarrow$ Pulse `link_strobe`.
3. `link_dir = 1` (Switch to Read mode).
4. Read 8 consecutive nibbles: `[31:28]`, `[27:24]`, ..., `[3:0]` with `link_strobe` pulses.
5. Send `CMD_CLEAR_IRQ` to acknowledge and reset interrupt line.  
*Total transaction time: 9 strobe cycles ($\approx 900\text{ ns}$).*

![ShrikeFi 4-Bit Parallel Link Protocol Timing Waveform](images/shrikefi_waveform.png)

---

## 4. Timing & Latency Budget

* **Core FPGA Clock:** 50 MHz internal oscillator ($T_{\text{tick}} = 20.000\text{ ns}$).
* **Sampling Rate:** 50 Hz optical acquisition (1 sample every 20,000 µs).
* **Speed & Timing Modes:**
  * **Measured Bring-Up Baseline (Software Bit-Bang Driver):**
    * Strobe rate: ~500 kHz ($T_{\text{strobe}} \approx 2\text{ µs}$ using `esp_rom_delay_us(1)` high/low half-cycles).
    * Sample Write (3 strobe cycles): $\approx 6\text{ µs}$.
    * 32-Bit IBI Read (9 strobe cycles): $\approx 18\text{ µs}$.
    * Bus Duty Cycle: $\approx 0.09\%$ at 50 Hz optical sampling, consuming $< 0.1\%$ MCU processing overhead.
  * **Protocol Theoretical Maximum (Simulation-Verified / Hardware-Assisted Target):**
    * Strobe rate: 10 MHz ($T_{\text{strobe}} = 100\text{ ns}$, verified in `tb_forgefpga_system.v`).
    * Sample Write (3 strobe cycles): $300\text{ ns}$.
    * 32-Bit IBI Read (9 strobe cycles): $900\text{ ns}$.
    * Raw Link Throughput: $5.0\text{ MB/s}$ (40 Mbps).
* **MCU CPU Headroom:** $> 99.9\%$ of ESP32-S3 CPU cycles remain completely available for FreeRTOS dual-core multitasking, BLE/WiFi telemetry, and INT8 TinyML inference.

---

## 5. Comparison to the Zynq AXI4-Lite Interface

| Function | Xilinx Zynq-7000 (AXI4-Lite) | Renesas ForgeFPGA (ShrikeFi 4-Bit Link) |
|---|---|---|
| **Interconnect Architecture** | 32-bit memory-mapped bus (`0x43C00000`) | 4-bit parallel GPIO nibble bus |
| **Write Raw Red Sample** | Write 32-bit register `0x00` (`REG_RED_RAW`) | `CMD_WRITE_RED` + 2 Data Nibbles |
| **Write Raw IR Sample** | Write 32-bit register `0x10` (`REG_IR_RAW`) | `CMD_WRITE_IR` + 2 Data Nibbles |
| **Read Filtered Output** | Read 32-bit register `0x04` / `0x14` | `CMD_READ_RED` / `CMD_READ_IR` (2 Nibbles) |
| **Read 32-bit IBI Cycles** | Read 32-bit register `0x08` (`REG_IBI_CYCLES`) | `CMD_READ_IBI` (8 Data Nibbles) |
| **Set Dynamic Threshold** | Write `REG_STATUS_THRESH[15:8]` (`0x0C`) | `CMD_WRITE_THRESH` (2 Data Nibbles) |
| **Clear Interrupt Flag** | Write `1` to `REG_STATUS_THRESH[0]` (W1C) | Send `CMD_CLEAR_IRQ` (`0x7`) |
| **Hardware Interrupt Pin** | AXI Fabric Interrupt to ARM GIC | Dedicated direct GPIO interrupt line (`irq_beat`) |
