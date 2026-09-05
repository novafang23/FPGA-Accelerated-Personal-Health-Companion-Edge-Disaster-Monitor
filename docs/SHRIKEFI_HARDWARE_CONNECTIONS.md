# ShrikeFi Hardware Wiring & Pinout Guide
**Project:** SIH26181 — FPGA-Accelerated Personal Health Companion & Edge Disaster Monitor  
**Target Hardware:** Vicharak ShrikeFi (ESP32-S3 Dual-Core + Renesas ForgeFPGA SLG47910)

---

## 1. Quick Reference: Complete System Pinout

| Peripheral / Interface | Signal Name | ESP32-S3 Pin | FPGA Pin | Voltage Level | Notes |
|---|---|:---:|:---:|:---:|---|
| **I2C Bus** | **SDA** | **GPIO 1** | — | 3.3V | Shared by OLED, MAX30102, BME280 |
| **I2C Bus** | **SCL** | **GPIO 2** | — | 3.3V | 400 kHz Fast-Mode I2C clock |
| **PMS5003 PM2.5** | **UART1 RX** | **GPIO 17** | — | 3.3V | Connects to PMS5003 Pin 7 (TX) |
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
| **Pin 5** | **TX** — Serial transmit (3.3V) | **ESP32 GPIO 17 (RX)** (Breadboard Row 6) |
| **Pin 6** | **RESET** — Module reset (3.3V) | *Leave unconnected (low reset)* |
| **Pin 7** | **NC** — Not Connected | *Leave unconnected* |
| **Pin 8** | **NC** — Not Connected | *Leave unconnected* |

* **Serial Configuration:** 9600 Baud, 8 Data Bits, No Parity, 1 Stop Bit.
* Firmware has internal pull-up enabled on GPIO 17 to prevent floating noise when disconnected.

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
Row 6 (PMS5003 TX)  : [6A: ESP32 GPIO 17][6B: PMS5003 Pin 5 (TX)] [6C: Empty] [6D: Empty] [6E: Empty]
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
When the firmware boots, it automatically runs an I2C bus scan across all 127 addresses and prints the status to the serial console:

```text
I (1234) I2C_HAL: Scanning I2C bus (SDA=1, SCL=2, 400kHz)...
I (1250) I2C_HAL:   - Device found at 0x08 (ForgeFPGA Configuration Interface)
I (1270) I2C_HAL:   - Device found at 0x3C (SSD1306 OLED Display)
I (1290) I2C_HAL:   - Device found at 0x57 (MAX30102 / MAX30100 Pulse Oximeter)
I (1310) I2C_HAL:   - Device found at 0x76 (BME280 Environmental Sensor)
I (1320) I2C_HAL: I2C scan complete: 4 devices found.
```

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
