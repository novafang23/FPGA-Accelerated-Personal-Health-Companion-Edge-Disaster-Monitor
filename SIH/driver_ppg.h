/*
 * driver_ppg.h — PPG Accelerator Hardware Driver API
 * SIH26181: AI-Powered Personal Health Companion (Qualcomm)
 *
 * Provides register-level access to the axi_ppg_accelerator hardware IP
 * for both Red (heart rate) and IR (SpO2) PPG channels.
 */

#ifndef DRIVER_PPG_H
#define DRIVER_PPG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Red Channel (Heart Rate / PPG) */

/* Push a raw 8-bit Red PPG sample into the hardware filter pipeline */
void ppg_push_sample(uint8_t raw_val);

/* Read back the latest Red filtered sample from hardware */
uint8_t ppg_get_filtered(void);

/* Set the dynamic peak detection threshold (bits [15:8] of status register) */
void ppg_set_threshold(uint8_t threshold);

/* Read heart rate (BPM) if a new beat was detected; returns 0.0 if no new beat */
float ppg_read_heart_rate(void);

/* IR Channel (SpO2) */

/* Push a raw 8-bit IR PPG sample into the hardware filter pipeline */
void ppg_push_ir_sample(uint8_t raw_val);

/* Read back the latest IR filtered sample from hardware */
uint8_t ppg_get_ir_filtered(void);

/* Status Helpers */

/* Returns 1 if beat_flag is set in the status register */
int ppg_beat_detected(void);

/* Read the raw IBI cycle count from hardware */
uint32_t ppg_get_ibi_cycles(void);

/* Clear the beat_flag via Write-1-to-Clear, preserving the threshold field */
void ppg_clear_beat_flag(void);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_PPG_H */
