/*
 * hrv_analysis.h
 * Heart Rate Variability Analysis Module
 */

#ifndef HRV_ANALYSIS_H
#define HRV_ANALYSIS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HRV_BUFFER_SIZE 20  /* Rolling window of last 20 IBI intervals */

typedef struct {
    float ibi_ms[HRV_BUFFER_SIZE];
    int   head;
    int   count;

    float rmssd;
    float sdnn;
    float mean_hr;
    float mean_ibi;
} hrv_state_t;

void  hrv_init(hrv_state_t *state);
void  hrv_add_ibi(hrv_state_t *state, float ibi_ms);
void  hrv_compute(hrv_state_t *state);

float hrv_get_rmssd(const hrv_state_t *state);
float hrv_get_sdnn(const hrv_state_t *state);
float hrv_get_mean_hr(const hrv_state_t *state);
int   hrv_is_ready(const hrv_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* HRV_ANALYSIS_H */
