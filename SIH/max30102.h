/*
 * max30102.h — MAX30102 Pulse Oximetry & Heart-Rate Sensor Driver
 * SIH26181: AI-Powered Personal Health Companion (Qualcomm)
 *
 * The MAX30102 is a dual-wavelength (Red 660nm + IR 940nm) reflective
 * pulse oximetry and heart-rate monitor with integrated LEDs, photodetector,
 * and 18-bit ADC.
 *
 * I2C Address: 0x57 (7-bit, fixed)
 * Data output: 18-bit ADC samples in a 32-sample FIFO
 */

#ifndef MAX30102_H
#define MAX30102_H

#include <stdint.h>
#include "i2c_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- I2C Address ---- */
#define MAX30102_I2C_ADDR   0x57

/* ---- Register Map ---- */
/* Status */
#define MAX30102_REG_INT_STATUS_1   0x00
#define MAX30102_REG_INT_STATUS_2   0x01
#define MAX30102_REG_INT_ENABLE_1   0x02
#define MAX30102_REG_INT_ENABLE_2   0x03

/* FIFO */
#define MAX30102_REG_FIFO_WR_PTR    0x04
#define MAX30102_REG_OVF_COUNTER    0x05
#define MAX30102_REG_FIFO_RD_PTR    0x06
#define MAX30102_REG_FIFO_DATA      0x07

/* Configuration */
#define MAX30102_REG_FIFO_CONFIG    0x08
#define MAX30102_REG_MODE_CONFIG    0x09
#define MAX30102_REG_SPO2_CONFIG    0x0A
#define MAX30102_REG_LED1_PA        0x0C  /* Red LED pulse amplitude   */
#define MAX30102_REG_LED2_PA        0x0D  /* IR LED pulse amplitude    */
#define MAX30102_REG_MULTI_LED_1    0x11
#define MAX30102_REG_MULTI_LED_2    0x12

/* Temperature */
#define MAX30102_REG_TEMP_INT       0x1F
#define MAX30102_REG_TEMP_FRAC      0x20
#define MAX30102_REG_TEMP_CONFIG    0x21

/* Part ID */
#define MAX30102_REG_REV_ID         0xFE
#define MAX30102_REG_PART_ID        0xFF
#define MAX30102_EXPECTED_PART_ID   0x15

/* ---- Mode Configuration ---- */
#define MAX30102_MODE_HEART_RATE    0x02  /* Red LED only         */
#define MAX30102_MODE_SPO2          0x03  /* Red + IR LEDs        */
#define MAX30102_MODE_MULTI_LED     0x07  /* Multi-LED mode       */
#define MAX30102_MODE_RESET         0x40  /* Reset bit            */
#define MAX30102_MODE_SHDN          0x80  /* Shutdown bit         */

/* ---- SPO2 Configuration ---- */
/* ADC Range: 0=2048nA, 1=4096nA, 2=8192nA, 3=16384nA */
#define MAX30102_SPO2_ADC_RANGE_4096    (0x01 << 5)
/* Sample Rate: 0=50, 1=100, 2=200, 3=400, 4=800, 5=1000, ... */
#define MAX30102_SPO2_SR_100            (0x01 << 2)
#define MAX30102_SPO2_SR_200            (0x02 << 2)
#define MAX30102_SPO2_SR_400            (0x03 << 2)
/* LED Pulse Width: 0=69us(15bit), 1=118us(16bit), 2=215us(17bit), 3=411us(18bit) */
#define MAX30102_SPO2_PW_411            0x03  /* 18-bit resolution */
#define MAX30102_SPO2_PW_215            0x02  /* 17-bit resolution */

/* ---- FIFO Configuration ---- */
#define MAX30102_FIFO_SMP_AVE_4     (0x02 << 5)  /* Average 4 samples  */
#define MAX30102_FIFO_ROLLOVER_EN   (1 << 4)      /* FIFO rollover      */
#define MAX30102_FIFO_A_FULL_17     0x0F          /* Interrupt at 17 unread */

/* ---- Raw sample data ---- */
typedef struct {
    uint32_t red;   /* 18-bit Red ADC value */
    uint32_t ir;    /* 18-bit IR ADC value  */
} max30102_sample_t;

/* ---- Driver state ---- */
typedef struct {
    i2c_handle_t *i2c;
    int           initialized;
} max30102_t;

/*
 * Initialize the MAX30102 in SpO2 mode.
 *   dev  — pointer to driver state
 *   i2c  — initialized I2C handle
 * Returns 0 on success, -1 on failure (wrong part ID, I2C error).
 */
int max30102_init(max30102_t *dev, i2c_handle_t *i2c);

/*
 * Soft-reset the MAX30102.
 * Returns 0 on success.
 */
int max30102_reset(max30102_t *dev);

/*
 * Read number of available samples in the FIFO.
 * Returns sample count (0-32), or -1 on error.
 */
int max30102_fifo_available(max30102_t *dev);

/*
 * Read one Red+IR sample pair from the FIFO.
 * Returns 0 on success, -1 if FIFO empty or error.
 */
int max30102_read_sample(max30102_t *dev, max30102_sample_t *sample);

/*
 * Read multiple samples from the FIFO into a buffer.
 * Returns number of samples actually read, or -1 on error.
 */
int max30102_read_fifo(max30102_t *dev, max30102_sample_t *buf, int max_samples);

/*
 * Read the on-chip die temperature (°C).
 * Returns temperature as float, or -999.0f on error.
 */
float max30102_read_temperature(max30102_t *dev);

/*
 * Scale an 18-bit raw ADC reading to 8-bit for the FPGA pipeline.
 * The FPGA accelerator expects uint8_t inputs.
 */
static inline uint8_t max30102_scale_to_8bit(uint32_t raw_18bit) {
    return (uint8_t)(raw_18bit >> 10);  /* Keep top 8 of 18 bits */
}

/*
 * Enter/exit shutdown mode for power management.
 */
int max30102_shutdown(max30102_t *dev);
int max30102_wakeup(max30102_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* MAX30102_H */
