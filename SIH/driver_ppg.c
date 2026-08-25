/*
 * driver_ppg.c — PPG Accelerator Hardware Driver Implementation
 * SIH26181: AI-Powered Personal Health Companion (Qualcomm)
 *
 * Register Map:
 *   0x00  REG_RED_RAW       [7:0]  R/W  Red raw sample
 *   0x04  REG_RED_FILTERED  [7:0]  RO   Red filtered output
 *   0x08  REG_IBI_CYCLES    [31:0] RO   Inter-Beat Interval (clock cycles)
 *   0x0C  REG_STATUS_THRESH [0]    W1C  beat_flag; [15:8] R/W threshold
 *   0x10  REG_IR_RAW        [7:0]  R/W  IR raw sample
 *   0x14  REG_IR_FILTERED   [7:0]  RO   IR filtered output
 */

#include "driver_ppg.h"
#include "xil_io.h"
#include "xparameters.h"
#include <stdint.h>

#define PPG_ACCEL_BASEADDR  XPAR_AXI_PPG_ACCELERATOR_0_S_AXI_BASEADDR

#define REG_RED_RAW         0x00
#define REG_RED_FILTERED    0x04
#define REG_IBI_CYCLES      0x08
#define REG_STATUS_THRESH   0x0C
#define REG_IR_RAW          0x10
#define REG_IR_FILTERED     0x14

#define FPGA_CLK_FREQ_HZ    50000000ULL  /* 50 MHz system clock */

/* ================================================================
 *  Red Channel (Heart Rate)
 * ================================================================ */

// Push raw sample into hardware pipeline
void ppg_push_sample(uint8_t raw_val) {
    Xil_Out32(PPG_ACCEL_BASEADDR + REG_RED_RAW, (uint32_t)raw_val);
}

// Read back filtered sample
uint8_t ppg_get_filtered(void) {
    return (uint8_t)(Xil_In32(PPG_ACCEL_BASEADDR + REG_RED_FILTERED) & 0xFF);
}

// Set dynamic peak threshold without clobbering other bitfields
void ppg_set_threshold(uint8_t threshold) {
    uint32_t reg_val = (uint32_t)threshold << 8;
    Xil_Out32(PPG_ACCEL_BASEADDR + REG_STATUS_THRESH, reg_val);
}

// Calculate heart rate with 64-bit overflow protection
float ppg_read_heart_rate(void) {
    uint32_t status = Xil_In32(PPG_ACCEL_BASEADDR + REG_STATUS_THRESH);

    // Check if beat_flag (Bit 0) is set
    if (status & 0x01) {
        uint32_t ibi_cycles = Xil_In32(PPG_ACCEL_BASEADDR + REG_IBI_CYCLES);

        // Preserve threshold in [15:8] and assert bit 0 to clear flag
        // (Write-1-to-Clear)
        uint32_t current_thresh = status & 0xFF00;
        Xil_Out32(PPG_ACCEL_BASEADDR + REG_STATUS_THRESH, current_thresh | 0x01);

        // Ensure valid cycle count before math
        if (ibi_cycles > 0 && ibi_cycles != 0xFFFFFFFF) {
            uint64_t numerator = FPGA_CLK_FREQ_HZ * 60ULL; // 3,000,000,000 in uint64
            return (float)numerator / (float)ibi_cycles;
        }
    }
    return 0.0f; // No new beat
}

/* ================================================================
 *  IR Channel (SpO2)
 * ================================================================ */

// Push raw IR sample into hardware pipeline
void ppg_push_ir_sample(uint8_t raw_val) {
    Xil_Out32(PPG_ACCEL_BASEADDR + REG_IR_RAW, (uint32_t)raw_val);
}

// Read back IR filtered sample
uint8_t ppg_get_ir_filtered(void) {
    return (uint8_t)(Xil_In32(PPG_ACCEL_BASEADDR + REG_IR_FILTERED) & 0xFF);
}

/* ================================================================
 *  Status Helpers
 * ================================================================ */

// Check if beat_flag is set
int ppg_beat_detected(void) {
    return (Xil_In32(PPG_ACCEL_BASEADDR + REG_STATUS_THRESH) & 0x01) ? 1 : 0;
}

// Read raw IBI cycle count
uint32_t ppg_get_ibi_cycles(void) {
    return Xil_In32(PPG_ACCEL_BASEADDR + REG_IBI_CYCLES);
}

// Clear beat flag via Write-1-to-Clear, preserving threshold
void ppg_clear_beat_flag(void) {
    uint32_t status = Xil_In32(PPG_ACCEL_BASEADDR + REG_STATUS_THRESH);
    uint32_t current_thresh = status & 0xFF00;
    Xil_Out32(PPG_ACCEL_BASEADDR + REG_STATUS_THRESH, current_thresh | 0x01);
}