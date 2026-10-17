# Hardware Settings & I2C Bus Stability (MAX30102 & SSD1306) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Eliminate MAX30102 LED blinking/reset churning and SSD1306 OLED pixel corruption caused by shared I2C bus errors, current transients, and desynchronized frame transmissions.

**Architecture:** 
1. Implement atomic, fault-tolerant SSD1306 page transfers with command re-sync on transmission failure to prevent partial-frame pixel corruption ("broken pixels").
2. Decouple MAX30102 transient read errors from hard sensor soft-resets so the LED driver does not repeatedly cycle off/on and churn the power rail.
3. Add a dedicated runtime I2C bus-clear and recovery function in `esp32_i2c_hal` to clear hung slave devices without restarting the MCU.
4. Provide an offline hardware diagnostics check and breadboard verification test in `run_host_test` / diagnostic CLI.

**Tech Stack:** ESP-IDF 5.5.5, FreeRTOS, C11, SSD1306 OLED driver, MAX30102 PPG driver, Renesas ForgeFPGA SPI link.

**Spec:** [docs/HARDWARE_ARCHITECTURE.md](file:///C:/Users/abhin/OneDrive/Desktop/verilog/docs/HARDWARE_ARCHITECTURE.md) and [docs/SHRIKEFI_HARDWARE_CONNECTIONS.md](file:///C:/Users/abhin/OneDrive/Desktop/verilog/docs/SHRIKEFI_HARDWARE_CONNECTIONS.md).

## Global Constraints
- All ESP32-S3 and ForgeFPGA GPIO pins are strictly 3.3V LVCMOS maximum.
- All common RTL and core firmware must remain standard-compliant and FreeRTOS safe.
- Single-precision float only on ESP32-S3 signal processing (no software double emulation).
- Zero CDN or online network dependencies.

## Review Focus
1. Partial I2C NACK during SSD1306 1024-byte framebuffer burst: Must immediately abort transmission and reset column/page registers rather than leaving distorted pixels.
2. Transient FIFO read error on MAX30102: Must retry FIFO pointers without calling `max30102_reset()` (which extinguishes LEDs and causes blinking).
3. Slave holding SDA low during transaction abort: Runtime recovery must issue 9 clock pulses on SCL and a STOP condition to free the bus.
4. Power rail transient dip during MAX30102 LED pulse: Firmware must ensure LED current changes are smooth (not toggling between 0 and 15.8mA rapidly).
5. Bootloader invocation (GPIO0 held low): Provide clear, foolproof instructions to distinguish normal run mode from ROM download mode.

---

### Task 1: SSD1306 Transmission Re-sync & Glitch Suppression

**Files:**
- Modify: `firmware/shrikefi/ssd1306.c:195-230`
- Modify: `firmware/shrikefi/ssd1306.h`
- Test: `firmware/shrikefi/main_shrikefi.c` (Profile 1 & Host simulation)

**Interfaces:**
- Consumes: `esp32_i2c_hal_write_raw(uint8_t dev_addr, const uint8_t *data, uint16_t len)`
- Produces: `int ssd1306_update(ssd1306_t *disp)` (hardened with atomic chunk tracking and immediate resync on failure)
- Produces: `int ssd1306_resync(ssd1306_t *disp)` (restores column/page pointers if an I2C transaction is dropped)

- [ ] **Step 1: Write host test asserting SSD1306 aborts cleanly on simulated I2C write failure**

In `firmware/shrikefi/main_shrikefi.c`, add an assertion in the host test verifying that when `esp32_i2c_hal` simulates a mid-frame write failure, `ssd1306_update` marks the display uninitialized and triggers `ssd1306_resync()` without writing invalid column boundaries.

- [ ] **Step 2: Run test to verify failure behavior**

Run: `gcc -Wall -Wextra -Werror -pedantic -std=c11 -Ifirmware/shrikefi -Ifirmware/core firmware/shrikefi/main_shrikefi.c ... -o $env:TEMP\valor_test.exe && & $env:TEMP\valor_test.exe`
Expected: Fails or triggers warning if resync interface is missing.

- [ ] **Step 3: Implement `ssd1306_resync` and abort handling in `firmware/shrikefi/ssd1306.c`**

In `ssd1306_update()`, if any chunk fails:
1. Immediately stop pushing further data chunks (prevents corrupting remaining pages).
2. Issue a re-sync command resetting column address range (0x00 to 0x7F) and page address range (0x00 to 0x07).
3. Return `I2C_HAL_ERROR` cleanly so caller can schedule a clean redraw.

- [ ] **Step 4: Run test to verify it passes**

Run host test and confirm Profile runs cleanly with 0 errors.

- [ ] **Step 5: Commit**

```bash
git add firmware/shrikefi/ssd1306.c firmware/shrikefi/ssd1306.h firmware/shrikefi/main_shrikefi.c
git commit -m "fix(ssd1306): add transmission resync and prevent broken pixel artifacts on I2C dropped packets"
```

---

### Task 2: MAX30102 Non-Blinking Transient Error Recovery

**Files:**
- Modify: `firmware/shrikefi/max30102.c:69-115`
- Modify: `firmware/shrikefi/main_shrikefi.c:640-665, 1135-1155`
- Test: `firmware/shrikefi/main_shrikefi.c` (Profile 12 & host pipeline)

**Interfaces:**
- Consumes: `max30102_fifo_available()`, `max30102_read_sample()`
- Produces: `int max30102_recover_fifo(max30102_t *dev)` (resets FIFO read/write pointers without turning off LEDs via `max30102_reset()`)

- [ ] **Step 1: Write host test for non-blinking recovery**

Verify in `main_shrikefi.c` host harness that when transient read errors occur (<5 errors), FIFO read/write pointers are cleared without executing full chip soft-reset (`MAX30102_MODE_RESET`).

- [ ] **Step 2: Run test to verify**

Run host test compiler and check behavior.

- [ ] **Step 3: Implement `max30102_recover_fifo` in `firmware/shrikefi/max30102.c` and update `main_shrikefi.c`**

1. Create `max30102_recover_fifo(max30102_t *dev)`:
   - Clears `MAX30102_REG_FIFO_WR_PTR` (0x04) = 0x00
   - Clears `MAX30102_REG_OVF_COUNTER` (0x05) = 0x00
   - Clears `MAX30102_REG_FIFO_RD_PTR` (0x06) = 0x00
   - Leaves `MAX30102_REG_MODE_CONFIG` and LED currents untouched!
2. In `main_shrikefi.c`:
   - On 1-4 consecutive I2C failures: attempt `max30102_recover_fifo()`. LEDs stay continuously illuminated.
   - Only on 10+ persistent failures (genuine disconnect) mark sensor uninitialized and back off.

- [ ] **Step 4: Run test to verify it passes**

Run host test and confirm Profile 9, 10, 11, 12 pass.

- [ ] **Step 5: Commit**

```bash
git add firmware/shrikefi/max30102.c firmware/shrikefi/max30102.h firmware/shrikefi/main_shrikefi.c
git commit -m "fix(max30102): recover FIFO pointers without resetting chip to prevent LED blinking churn"
```

---

### Task 3: Runtime I2C Bus Clear & Timeout Hardening

**Files:**
- Modify: `firmware/shrikefi/esp32_i2c_hal.c:20-75`
- Modify: `firmware/shrikefi/esp32_i2c_hal.h`
- Test: `firmware/shrikefi/main_shrikefi.c`

**Interfaces:**
- Produces: `int esp32_i2c_hal_recover_bus(void)` (runtime callable 9-pulse bus unlock)

- [ ] **Step 1: Write test for runtime bus recovery interface**

Expose `esp32_i2c_hal_recover_bus()` in `esp32_i2c_hal.h` and add host simulation check.

- [ ] **Step 2: Implement `esp32_i2c_hal_recover_bus`**

Allow runtime invocation of the 9-clock recovery pulses if an `ESP_ERR_TIMEOUT` occurs on the I2C bus.

- [ ] **Step 3: Run test to verify clean compilation and execution**

Run host test suite.

- [ ] **Step 4: Commit**

```bash
git add firmware/shrikefi/esp32_i2c_hal.c firmware/shrikefi/esp32_i2c_hal.h
git commit -m "feat(i2c): add runtime 9-clock bus-clear recovery for stuck slave SDA states"
```

---

### Task 4: Hardware Breadboard Verification & Diagnostic Guidance

**Files:**
- Modify: `docs/SHRIKEFI_HARDWARE_CONNECTIONS.md`
- Test: Documentation review and hardware checklist

- [ ] **Step 1: Add electrical decoupling and pull-up measurement guide**
Include concrete steps:
1. Measure resistance between 3.3V and SDA / SCL (should be between 1.8kΩ and 3.3kΩ total).
2. Place a 10µF + 100nF decoupling capacitor across 3.3V and GND right at the MAX30102 and OLED power pins on the breadboard to buffer the 15-25mA LED pulses.
3. Ensure GND connections from ESP32-S3 to all 4 sensors are star-connected or firmly seated to eliminate ground bounce.

- [ ] **Step 2: Commit**

```bash
git add docs/SHRIKEFI_HARDWARE_CONNECTIONS.md
git commit -m "docs: add electrical decoupling and breadboard noise suppression guidelines"
```
