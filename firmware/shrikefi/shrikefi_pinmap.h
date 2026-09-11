/*
 * shrikefi_pinmap.h
 * Consolidated GPIO & Peripheral Pin Assignment Header
 * SIH26181 Health Companion & Edge Disaster Monitor
 * Target: Vicharak ShrikeFi (ESP32-S3 + Renesas ForgeFPGA SLG47910)
 *
 * ============================================================
 *  CRITICAL VOLTAGE RULE — READ BEFORE WIRING ANYTHING
 * ============================================================
 *  All ESP32-S3 GPIO pins are strictly 3.3V LVCMOS.
 *  The ForgeFPGA GPIO header is also 3.3V ONLY.
 *  DO NOT apply 5V to any GPIO pin — this permanently
 *  damages the IC beyond repair (no warranty coverage).
 *
 *  5V is ONLY used for PMSA003 VCC (fan + laser power).
 *  Source 5V from the USB-C VBUS rail only.
 * ============================================================
 */

#ifndef SHRIKEFI_PINMAP_H
#define SHRIKEFI_PINMAP_H

/* =========================================================================
 * I2C BUS — Shared by OLED, MAX30102, BME280
 * =========================================================================
 * Physical wiring: SDA and SCL each need one 2.2kΩ pull-up to 3.3V.
 * On a custom PCB, place these pull-ups within 1cm of the connector.
 * ====================================================================== */
#define PIN_I2C_SDA          1    /* GPIO1  — SDA (all I2C devices share this) */
#define PIN_I2C_SCL          2    /* GPIO2  — SCL (all I2C devices share this) */
#define I2C_BUS_SPEED_HZ     400000  /* 400 kHz Fast-Mode */

/* I2C Device Addresses */
#define I2C_ADDR_SSD1306     0x3C  /* OLED display (alt: 0x3D if ADDR pin = HIGH) */
#define I2C_ADDR_SSD1306_ALT 0x3D
#define I2C_ADDR_MAX30102    0x57  /* PPG + SpO2 sensor (fixed, non-configurable)  */
#define I2C_ADDR_BME280      0x76  /* Environmental sensor (SDO=GND → 0x76)       */
#define I2C_ADDR_BME280_ALT  0x77  /* (SDO=3.3V → 0x77)                           */

/* =========================================================================
 * UART1 — PMSA003 Particulate Matter Sensor
 * =========================================================================
 * PMSA003 Power:
 *   VCC pin → 5V VBUS only (fan motor + laser diode require 5V)
 *   TX/RX logic → 3.3V (safe for ESP32-S3 direct connection, no level shifter)
 * Baud: 9600, 8N1, no flow control
 * The sensor outputs frames continuously (~1 Hz) in passive mode.
 * ====================================================================== */
#define PIN_PMSA003_RX       14   /* GPIO14 — UART1 RX (connects to PMSA003 TX pin) */
#define PIN_PMSA003_TX       18   /* GPIO18 — UART1 TX (connects to PMSA003 RX pin, optional) */
#define PMSA003_UART_NUM     1    /* UART peripheral number (UART_NUM_1) */
#define PMSA003_BAUD         9600

/* =========================================================================
 * Renesas ForgeFPGA (SLG47910) Interconnect — Official Vicharak Shrike-Fi Traces
 * =========================================================================
 * These 6 pins are internal PCB copper traces on the Vicharak Shrike-Fi board:
 *   - GPIO 8:  FPGA Power Control (PWR)
 *   - GPIO 9:  FPGA Hardware Enable (EN)
 *   - GPIO 10: SPI Chip Select (SS) / Dual-purpose Link Data 0
 *   - GPIO 11: SPI MOSI (SI) / Dual-purpose Link Data 1
 *   - GPIO 12: SPI Clock (SCK) / Dual-purpose Link Data 2
 *   - GPIO 13: SPI MISO (SO/CONFIG) / Dual-purpose Link Data 3 / Beat IRQ
 *
 * NOTE: These are completely internal to the Shrike-Fi PCB. No Zero PCB wiring needed!
 * ====================================================================== */
#define PIN_FPGA_PWR         8    /* GPIO8  — FPGA Power Control */
#define PIN_FPGA_EN          9    /* GPIO9  — FPGA Hardware Reset/Enable */
#define PIN_FPGA_SS          10   /* GPIO10 — SPI CS / Dual-Purpose Link D0 */
#define PIN_FPGA_MOSI        11   /* GPIO11 — SPI MOSI / Dual-Purpose Link D1 */
#define PIN_FPGA_SCK         12   /* GPIO12 — SPI SCK / Dual-Purpose Link D2 */
#define PIN_FPGA_MISO        13   /* GPIO13 — SPI MISO / Dual-Purpose Link D3 / Beat IRQ */

/* Dual-purpose link aliases */
#define PIN_FPGA_DATA0       PIN_FPGA_SS
#define PIN_FPGA_DATA1       PIN_FPGA_MOSI
#define PIN_FPGA_DATA2       PIN_FPGA_SCK
#define PIN_FPGA_DATA3       PIN_FPGA_MISO
#define PIN_FPGA_BEAT_IRQ    PIN_FPGA_MISO
#define PIN_FPGA_STROBE      PIN_FPGA_SCK
#define PIN_FPGA_DIR         PIN_FPGA_SS
#define PIN_FPGA_RST_N       PIN_FPGA_EN

/* =========================================================================
 * MAX30102 Interrupt (Optional — firmware uses polling at 50Hz instead)
 * =========================================================================
 * The MAX30102 INT pin is open-drain, active-low, requires a pull-up.
 * Currently unused: firmware polls FIFO at 50Hz via I2C.
 * To enable interrupt-driven mode, wire INT to this pin and add a
 * 100kΩ pull-up to 3.3V on the PCB.
 * ====================================================================== */
#define PIN_MAX30102_INT     -1   /* Not connected in current firmware revision */

/* =========================================================================
 * Power Rails Summary
 * =========================================================================
 *  Net Name    Voltage    Powers
 *  ----------  ---------  -----------------------------------------------
 *  VBUS        5.0V       PMSA003 VCC (Pin 1) only
 *  3V3         3.3V       ESP32-S3, ForgeFPGA, MAX30102, BME280, OLED, PMSA003 logic
 *  GND         0V         Common ground for all components
 * ====================================================================== */

#endif /* SHRIKEFI_PINMAP_H */
