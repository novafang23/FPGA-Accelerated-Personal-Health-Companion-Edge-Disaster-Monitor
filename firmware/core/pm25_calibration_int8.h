#ifndef PM25_CALIBRATION_INT8_H
#define PM25_CALIBRATION_INT8_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Calibration confidence flags */
#define PM25_CAL_CONFIDENCE_NORMAL         0x01U
#define PM25_CAL_CONFIDENCE_HUMIDITY_BIAS  0x02U
#define PM25_CAL_CONFIDENCE_OVER_RANGE     0x04U

/* Telemetry / calibration status structure */
typedef struct {
    float raw_pm;
    float calibrated_pm;
    float rh_bias_correction;
    uint8_t confidence_flag;
} pm25_cal_telemetry_t;

void pm25_calibration_init(void);
float pm25_calibrate_nn_int8(float raw_pm25, float temp_c, float humidity_pct);
bool pm25_is_humidity_distorted(float raw_pm25, float humidity_pct);

#ifdef __cplusplus
}
#endif

#endif /* PM25_CALIBRATION_INT8_H */
