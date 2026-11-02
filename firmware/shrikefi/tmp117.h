/*
 * tmp117.h
 * TI TMP117 High-Accuracy Digital Temperature Sensor Driver
 * SIH26181 Health Companion & Edge Disaster Monitor
 *
 * The TMP117 is the dedicated clinical skin-temperature probe. It is the only
 * element in this project that measures a body temperature directly; the
 * MAX30102's on-die thermometer is a die/junction sensor dominated by emitter
 * self-heating and is used only as a fallback estimate.
 *
 * KEY FACTS (TI TMP117 datasheet, SBOS884)
 *   - 16-bit result, LSB = 7.8125 mC (1/128 C); resolution 0.0078 C
 *   - Accuracy +/-0.1 C (max) over -20..+50 C, which is the clinically useful
 *     band, and +/-0.2 C over the full -55..+150 C range
 *   - Default address 0x48, configurable 0x48..0x4B via ADD0/ADD1
 *   - Power: ~3.5 uA average in 1 Hz one-shot mode (this driver uses that)
 *   - Conversion time: 15.5 ms typical
 *
 * TEMPERATURE DECODING
 *   The result is a 16-bit TWO'S COMPLEMENT value in units of 1/128 C.
 *   Reading -0.5 C gives 0xFFC0, so it must be cast to int16_t BEFORE scaling;
 *   masking to 12 bits (a common mistake) silently turns every negative
 *   temperature into a large positive one.
 */

#ifndef TMP117_H
#define TMP117_H

#include <stdint.h>
#include <stdbool.h>
#include "esp32_i2c_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* I2C addresses, selected by the ADD0 pin (and ADD1 on some variants). */
#define TMP117_I2C_ADDR_0     0x48
#define TMP117_I2C_ADDR_1     0x49
#define TMP117_I2C_ADDR_2     0x4A
#define TMP117_I2C_ADDR_3     0x4B
#define TMP117_I2C_ADDR       TMP117_I2C_ADDR_0

/* Register map */
#define TMP117_REG_TEMP       0x00   /* 16-bit temperature result        */
#define TMP117_REG_CONFIG     0x01   /* configuration (16-bit)           */
#define TMP117_REG_THIGH      0x02
#define TMP117_REG_TLOW       0x03
#define TMP117_REG_EEPROM_UL  0x04
#define TMP117_REG_EEPROM1    0x05
#define TMP117_REG_EEPROM2    0x06
#define TMP117_REG_TEMP_OFF   0x07
#define TMP117_REG_EEPROM3    0x08
#define TMP117_REG_DEVICE_ID  0x0F   /* 16-bit; bits[11:0] must be 0x117 */

/* Device ID: the low 12 bits of register 0x0F are always 0x117. */
#define TMP117_DEVICE_ID_MASK 0x0FFFu
#define TMP117_DEVICE_ID_VAL  0x0117u

/* LSB size in Celsius. 1/128 = 0.0078125 */
#define TMP117_LSB_C          0.0078125f

/* Configuration bits we care about (register 0x01). */
#define TMP117_CFG_MOD_SHUTDOWN   (1u << 10)  /* 1 = shutdown, 0 = continuous */
#define TMP117_CFG_CONV_ONE_SHOT  (3u << 10)  /* 11b = one-shot in some tables */
#define TMP117_CFG_AVG_MASK       (3u << 5)
#define TMP117_CFG_CONV_MASK      (7u << 7)   /* 111b = 1 s conversion cycle   */

/* Plausibility window for a body-contact reading. Anything outside this is a
 * bad read (open bus, wrong device, EEPROM not loaded) and must NOT be shown as
 * a temperature. A powered TMP117 cannot read below the enclosure ambient, and
 * 45 C is beyond any survivable skin temperature at the sensor. */
#define TMP117_MIN_VALID_C    (-20.0f)
#define TMP117_MAX_VALID_C    ( 60.0f)

typedef struct {
    esp32_i2c_handle_t *i2c;
    uint8_t  addr;
    int      initialized;
    /* Cached last good reading, so a transient I2C failure does not blank the
     * displayed value mid-contact. */
    float    last_c;
    uint32_t error_count;
} tmp117_t;

/*
 * Initialise the probe.
 *   dev  - driver state
 *   i2c  - initialised I2C handle
 *   addr - I2C address (TMP117_I2C_ADDR unless ADD0 is strapped high)
 *
 * Returns 0 on success, -1 if the device does not answer or the device ID is
 * wrong (which is the check that distinguishes "no sensor fitted" from "a
 * different chip answering at 0x48").
 */
int tmp117_init(tmp117_t *dev, esp32_i2c_handle_t *i2c, uint8_t addr);

/*
 * Read the temperature once, in Celsius.
 * Returns 0 on success and writes *out_c; -1 on I2C failure or an implausible
 * result. On success dev->last_c is updated.
 */
int tmp117_read_temp(tmp117_t *dev, float *out_c);

/* Last successfully read temperature, or 0.0f if there has never been one. */
float tmp117_last_temp(const tmp117_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* TMP117_H */
