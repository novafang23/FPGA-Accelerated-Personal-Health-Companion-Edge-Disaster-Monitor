# ShrikeFi Hardware Wiring & Pinout Guide
**Project:** SIH26181 — FPGA-Accelerated Personal Health Companion & Edge Disaster Monitor  
**Target Hardware:** Vicharak ShrikeFi (ESP32-S3 Dual-Core + Renesas ForgeFPGA SLG47910)

---

## 1. Quick Reference: Complete System Pinout

| Peripheral / Interface | Signal Name | ESP32-S3 Pin | FPGA Pin | Voltage Level | Notes |
|---|---|:---:|:---:|:---:|---|
| **I2C Bus** | **SDA** | **GPIO 1** | — | 3.3V | Shared by OLED, MAX30102, BME280 |
| **I2C Bus** | **SCL** | **GPIO 2** | — | 3.3V | 400 kHz Fast-Mode I2C clock |
| **PMS5003 PM2.5** | **UART1 RX** | **GPIO 14** | — | 3.3V | Connects to PMS5003 Pin 5 (TX) |
| **PMS5003 PM2.5** | **UART1 TX** | **GPIO 18** | — | 3.3V | Connects to PMS5003 Pin 6 (RX) *(optional)* |
| **FPGA 4-Bit Link** | `link_strobe` | **GPIO 4** | PIN_14 | 3.3V | Internal PCB trace (Strobe Clock) |
| **FPGA 4-Bit Link** | `link_dir` | **GPIO 5** | PIN_15 | 3.3V | Internal PCB trace (0=Write, 1=Read) |
| **FPGA 4-Bit Link** | `link_data[0]` | **GPIO 6** | PIN_16 / 20 | 3.3V | Internal PCB trace (Data Bit 0 - LSB) |
| **FPGA 4-Bit Link** | `link_data[1]` | **GPIO 7** | PIN_17 / 21 | 3.3V | Internal PCB trace (Data Bit 1) |
| **FPGA 4-Bit Link** | `link_data[2]` | **GPIO 8** | PIN_18 / 22 | 3.3V | Internal PCB trace (Data Bit 2) |
| **FPGA 4-Bit Link** | `link_data[3]` | **GPIO 9** | PIN_19 / 23 | 3.3V | Internal PCB trace (Data Bit 3 - MSB) |
| **FPGA Beat IRQ** | `irq_beat` | **GPIO 10** | PIN_24 | 3.3V | Internal PCB trace (Active-High Beat Pulse) |
| **FPGA Reset** | `rst_n` | **GPIO 3** | PIN_13 | 3.3V | Active-Low FPGA system reset |
| **Power Input** | **5V (VBUS)** | **5V / VBUS** | — | **5.0V** | Powers PMS5003 fan & laser |
| **Power System** | **3.3V** | **3.3V** | VDD | **3.3V** | Powers all sensors, MCU, and FPGA |
| **Ground** | **GND** | **GND** | GND | 0V | Common ground across all modules |

> [!CAUTION]
> **CRITICAL VOLTAGE RULE: 3.3V ONLY**  
> All ESP32-S3 and Renesas ForgeFPGA I/O pins are strictly **3.3V LVCMOS**.  
> **NEVER** apply 5V to any GPIO pin or FPGA header pin. 5V must **ONLY** go to the PMS5003 power pins (Pin 1 & 2).

---

## 2. Sensor-by-Sensor Wiring Details

### A. I2C Bus Devices (Shared on GPIO 1 & GPIO 2)
The SSD1306 OLED, MAX30102/MAX30100 optical pulse sensor, and BME280 environmental sensor all share the **same two communication pins**:

```
 ESP32-S3                     Shared I2C Bus                     Sensors
┌─────────┐              ┌──────────────────────┐          ┌─────────────────┐
│  GPIO 1 │ ─── SDA ───▶ │ Breadboard Row (SDA) │ ───────▶ │ OLED SDA        │
│         │              │                      │ ───────▶ │ MAX30102 SDA    │
│         │              │                      │ ───────▶ │ BME280 SDA      │
│         │              ├──────────────────────┤          ├─────────────────┤
│  GPIO 2 │ ─── SCL ───▶ │ Breadboard Row (SCL) │ ───────▶ │ OLED SCL        │
│         │              │                      │ ───────▶ │ MAX30102 SCL    │
│         │              │                      │ ───────▶ │ BME280 SCL      │
└─────────┘              └──────────────────────┘          └─────────────────┘
```

#### Device Addresses & Power:
1. **SSD1306 OLED (128x64 Display):**
   * `VCC` $\to$ **3.3V**
   * `GND` $\to$ **GND**
   * `SDA` $\to$ **GPIO 1**
   * `SCL` $\to$ **GPIO 2**
   * *I2C Address:* `0x3C` (default) or `0x3D`

2. **MAX30102 / MAX30100 (PPG Pulse & SpO2 Sensor):**
   * `VIN / VCC` $\to$ **3.3V**
   * `GND` $\to$ **GND**
   * `SDA` $\to$ **GPIO 1**
   * `SCL` $\to$ **GPIO 2**
   * `INT` $\to$ *Leave unconnected (FIFO is polled at 50Hz)*
   * *I2C Address:* `0x57`

3. **BME280 (Temperature, Humidity, Pressure):**
   * `VIN` $\to$ **3.3V**
   * `GND` $\to$ **GND**
   * `SDA` $\to$ **GPIO 1**
   * `SCL` $\to$ **GPIO 2**
   * `CS`  $\to$ **3.3V** (selects I2C mode)
   * `SDO` $\to$ **GND** (selects address `0x76`) or **3.3V** (selects `0x77`)
   * *I2C Address:* `0x76`

---

### B. Plantower PMS5003 (Particulate Matter PM2.5 / PM10 Sensor)
The PMS5003 uses an 8-pin 1.25mm connector. Its fan and laser diode require **5V**, while its logic lines are **3.3V**.

```
    PMS5003 8-Pin Connector (Official Plantower Datasheet):
    ┌───┬───┬───┬───┬───┬───┬───┬───┐
    │ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │ 7 │ 8 │
    └───┴───┴───┴───┴───┴───┴───┴───┘
     VCC GND SET RX  TX  RST NC  NC
```

| PMS5003 Pin | Function per Datasheet | Connect To |
|:---:|---|---|
| **Pin 1** | **VCC (5V)** — Positive Power | **5V / VBUS** on ShrikeFi board (Breadboard Row 3) |
| **Pin 2** | **GND** — Negative Power | **GND** on ShrikeFi board (Breadboard Row 1) |
| **Pin 3** | **SET** — Mode select (3.3V) | **3.3V** (Row 2) or leave floating (normal run mode) |
| **Pin 4** | **RX** — Serial receive (3.3V) | **ESP32 GPIO 18 (TX)** *(optional)* |
| **Pin 5** | **TX** — Serial transmit (3.3V) | **ESP32 GPIO 14 (RX)** (Breadboard Row 6) |
| **Pin 6** | **RESET** — Module reset (3.3V) | *Leave unconnected (low reset)* |
| **Pin 7** | **NC** — Not Connected | *Leave unconnected* |
| **Pin 8** | **NC** — Not Connected | *Leave unconnected* |

* **Serial Configuration:** 9600 Baud, 8 Data Bits, No Parity, 1 Stop Bit.
* Firmware has internal pull-up enabled on GPIO 14 to prevent floating noise when disconnected.

---

## 3. Mini Breadboard (170-Tie Point) Wiring Map

On a 170-tie point mini breadboard, each numbered row has **5 connected holes** (`A-B-C-D-E` on left, `F-G-H-I-J` on right). 

Here is the cleanest way to allocate your rows:

```text
Row 1 (GND Hub)     : [1A: ShrikeFi GND] [1B: OLED GND] [1C: MAX30102 GND] [1D: BME280 GND] [1E: PMS5003 Pin 2 (GND)]
Row 2 (3.3V Hub)    : [2A: ShrikeFi 3.3V][2B: OLED VCC] [2C: MAX30102 VIN] [2D: BME280 VIN] [2E: PMS5003 Pin 3 (SET)]
Row 3 (5V Power Hub): [3A: ShrikeFi 5V]  [3B: PMS5003 Pin 1 (VCC)] [3C: Empty] [3D: Empty] [3E: Empty]
Row 4 (I2C SDA Hub) : [4A: ESP32 GPIO 1] [4B: OLED SDA] [4C: MAX30102 SDA] [4D: BME280 SDA] [4E: Empty]
Row 5 (I2C SCL Hub) : [5A: ESP32 GPIO 2] [5B: OLED SCL] [5C: MAX30102 SCL] [5D: BME280 SCL] [5E: Empty]
Row 6 (PMS5003 TX)  : [6A: ESP32 GPIO 14][6B: PMS5003 Pin 5 (TX)] [6C: Empty] [6D: Empty] [6E: Empty]
Row 7 (PMS5003 RX)  : [7A: ESP32 GPIO 18][7B: PMS5003 Pin 4 (RX)] [7C: Empty] [7D: Empty] [7E: Empty]
```

---

## 4. Renesas ForgeFPGA (SLG47910) Onboard Architecture

### Where is the FPGA?
* The ForgeFPGA is **soldered directly on the ShrikeFi PCB**.
* You **do not** need external jumper wires between the ESP32-S3 and the ForgeFPGA — the 4-bit bus lines (GPIO 4 through 10) are routed through high-speed internal PCB copper traces.

### 4-Bit Parallel Link Interface Details:
* **Protocol:** Synchronous 4-bit nibble transfers clocked by `link_strobe` (GPIO 4).
* **Speed:** 500 kHz software bit-bang driver / up to 10 MHz simulation-verified throughput (40 Mbps).
* **Commands:**
  * `0x1`: Send raw Red sample (8-bit)
  * `0x2`: Send raw IR sample (8-bit)
  * `0x3`: Set systolic detection threshold (8-bit, default `120`)
  * `0x6`: Read 32-bit IBI cycle count ($T_{\text{clk}} = 20\text{ ns}$)
  * `0x7`: Acknowledge and clear `irq_beat` interrupt
* **Hardware Interrupt:** When the systolic peak detector on the FPGA identifies a crest, it drives `irq_beat` (GPIO 10) HIGH. The ESP32 immediately services this interrupt to update Heart Rate and HRV metrics.

---

## 5. Software & Hardware Diagnostics

### A. I2C Bus Auto-Scan Output
At boot the firmware calls `esp32_i2c_hal_scan()`, which probes addresses 0x01–0x7E and logs every responder. The exact lines the code emits (`firmware/shrikefi/esp32_i2c_hal.c`) are:

```text
I (1234) I2C_SCAN: Scanning I2C bus (SDA=GPIO1, SCL=GPIO2)...
I (1250) I2C_SCAN:  -> Found device at 0x3C (SSD1306 OLED)
I (1256) I2C_SCAN:  -> Found device at 0x57 (MAX30100/MAX30102 PPG)
I (1262) I2C_SCAN:  -> Found device at 0x76 (BME280 Env)
I (1270) I2C_SCAN: Scan complete: 3 device(s) found.
```

> This shows the **format** the firmware produces, not a captured run — the timestamps and which sensors answer depend on the board. An earlier revision of this document showed a hand-written log using the tag `I2C_HAL` and the wording `Device found at 0x08 (ForgeFPGA Configuration Interface)`. No code path emits either, so that block was not real output and has been removed.

**The line worth looking for is whether `0x08` appears.** `esp32_i2c_hal_scan()` labels 0x08 as `ForgeFPGA` (`esp32_i2c_hal.c:88`), but the Renesas SLG47910 is not documented to expose a hard I2C configuration port, and this design's pin constraints (`hardware/shrikefi/forgefpga_pins.pcf`) declare no I2C or SPI configuration interface.

* **No `0x08` line** → expected. The FPGA configures itself from OTP/NVM or the onboard W25Q32JV QSPI flash at power-up. Nothing is wrong.
* **`0x08` appears** → something real is answering. `shrikefi_fpga_flash_init()` will then attempt an I2C bitstream write, which is **unverified**: the HAL's 8-bit register address cannot address a 46 KB image correctly.

`shrikefi_fpga_flash_init()` logs which case it concluded at boot — look for `No I2C configuration interface at 0x08` or `Device ACKed at I2C 0x08`.

### B. SpO2 Engine Tuning
The SpO2 calculation uses an **8-second rolling moving-average window** (`SPO2_MA_FILTER_SIZE = 8` in [`firmware/core/spo2_engine.h`](../firmware/core/spo2_engine.h)) with **slew-rate limiting ($\pm 2.5\%$ per second)** to eliminate sensor flicker and finger-motion artifacts while keeping true medical response fast and accurate.

---

## 6. How to Flash & Monitor

```powershell
# 1. Activate ESP-IDF environment
. C:\Espressif\frameworks\esp-idf-v5.5.5\export.ps1

# 2. Navigate to ShrikeFi project
cd C:\Users\abhin\OneDrive\Desktop\verilog\firmware\shrikefi

# 3. Flash and open serial monitor on COM5 (or your active COM port)
idf.py -p COM5 flash monitor
```

To exit the serial monitor: press `Ctrl + ]`.
