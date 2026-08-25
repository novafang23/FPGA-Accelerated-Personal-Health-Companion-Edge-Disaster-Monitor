/*
 * hrv_analysis.h — Heart Rate Variability Analysis Module
 * SIH26181: AI-Powered Personal Health Companion
 *
 * Computes RMSSD and SDNN from a circular buffer of Inter-Beat Intervals (IBI).
 * These metrics are key biomarkers for autonomic nervous system stress,
 * heat exhaustion, and physical fatigue detection.
 */

#ifndef HRV_ANALYSIS_H
#define HRV_ANALYSIS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HRV_BUFFER_SIZE 20  /* Rolling window of last 20 IBI intervals */

typedef struct {
    float ibi_ms[HRV_BUFFER_SIZE];  /* Inter-Beat Interval buffer (milliseconds)  */
    int   head;                      /* Next write position (circular)              */
    int   count;                     /* Number of valid entries (up to BUFFER_SIZE) */

    /* Computed metrics (updated by hrv_compute) */
    float rmssd;     /* Root Mean Square of Successive Differences (ms) */
    float sdnn;      /* Standard Deviation of NN intervals (ms)         */
    float mean_hr;   /* Mean Heart Rate (BPM)                           */
    float mean_ibi;  /* Mean IBI (ms)                                   */
} hrv_state_t;

/* Initialize / reset all HRV state to zero */
void  hrv_init(hrv_state_t *state);

/* Push a new IBI measurement (in milliseconds) into the circular buffer */
void  hrv_add_ibi(hrv_state_t *state, float ibi_ms);

/* Recompute RMSSD, SDNN, mean_hr, mean_ibi from the current buffer contents */
void  hrv_compute(hrv_state_t *state);

/* Accessors */
float hrv_get_rmssd(const hrv_state_t *state);
float hrv_get_sdnn(const hrv_state_t *state);
float hrv_get_mean_hr(const hrv_state_t *state);

/* Returns 1 when at least 3 IBI samples are buffered (minimum for meaningful HRV) */
int   hrv_is_ready(const hrv_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* HRV_ANALYSIS_H */
