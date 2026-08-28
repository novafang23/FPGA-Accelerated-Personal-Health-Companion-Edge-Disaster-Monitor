/*
 * max30102.c
 * MAX30102 Pulse Oximetry & Heart-Rate Sensor Driver
 */

#include "max30102.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

/* Initialization */

int max30102_reset(max30102_t *dev) {
  if (!dev || !dev->i2c)
    return -1;

  /* Assert software reset */
  if (i2c_write_reg(dev->i2c, MAX30102_I2C_ADDR, MAX30102_REG_MODE_CONFIG,
                    MAX30102_MODE_RESET) != 0) {
    return -1;
  }

/* Poll until reset bit self-clears (typically < 1ms) */
    int timeout = 100;
    while (timeout-- > 0) {
        int val =
            i2c_read_reg(dev->i2c, MAX30102_I2C_ADDR, MAX30102_REG_MODE_CONFIG);
        if (val < 0)
            return -1;
        if (!(val & MAX30102_MODE_RESET))
            break;
#ifdef _WIN32
        Sleep(1);
#else
        usleep(1000);
#endif
    }
    return (timeout > 0) ? 0 : -1;
}

int max30102_init(max30102_t *dev, i2c_handle_t *i2c) {
  if (!dev || !i2c)
    return -1;

  dev->i2c = i2c;
  dev->initialized = 0;

  /* Verify part ID */
  int part_id = i2c_read_reg(i2c, MAX30102_I2C_ADDR, MAX30102_REG_PART_ID);
  if (part_id < 0 || (uint8_t)part_id != MAX30102_EXPECTED_PART_ID) {
    return -1; /* Wrong device or I2C failure */
  }

  /* Reset to known state */
  if (max30102_reset(dev) != 0)
    return -1;

  /*
   * FIFO Configuration:
   *   - 4-sample averaging (SMP_AVE = 010)
   *   - FIFO rollover enabled (prevents stale data lockup)
   *   - FIFO almost-full at 17 unread samples
   */
  if (i2c_write_reg(i2c, MAX30102_I2C_ADDR, MAX30102_REG_FIFO_CONFIG,
                    MAX30102_FIFO_SMP_AVE_4 | MAX30102_FIFO_ROLLOVER_EN |
                        MAX30102_FIFO_A_FULL_17) != 0) {
    return -1;
  }

  /*
   * SpO2 Configuration:
   *   - ADC range: 4096nA (good sensitivity for finger contact)
   *   - Sample rate: 100 Hz (with 4x averaging → 25 effective samples/sec)
   *   - Pulse width: 411µs (18-bit ADC resolution, best SNR)
   */
  if (i2c_write_reg(i2c, MAX30102_I2C_ADDR, MAX30102_REG_SPO2_CONFIG,
                    MAX30102_SPO2_ADC_RANGE_4096 | MAX30102_SPO2_SR_100 |
                        MAX30102_SPO2_PW_411) != 0) {
    return -1;
  }

  /*
   * LED Pulse Amplitude:
   *   Red (LED1): 0x24 = ~7.2mA (moderate, good for finger-tip)
   *   IR  (LED2): 0x24 = ~7.2mA (matched to Red for R-ratio accuracy)
   *
   * Can be tuned based on skin tone and ambient light conditions.
   * Range: 0x00 (0mA) to 0xFF (51mA).
   */
  if (i2c_write_reg(i2c, MAX30102_I2C_ADDR, MAX30102_REG_LED1_PA, 0x24) != 0) {
    return -1;
  }
  if (i2c_write_reg(i2c, MAX30102_I2C_ADDR, MAX30102_REG_LED2_PA, 0x24) != 0) {
    return -1;
  }

  /* Clear FIFO pointers before starting */
  i2c_write_reg(i2c, MAX30102_I2C_ADDR, MAX30102_REG_FIFO_WR_PTR, 0x00);
  i2c_write_reg(i2c, MAX30102_I2C_ADDR, MAX30102_REG_OVF_COUNTER, 0x00);
  i2c_write_reg(i2c, MAX30102_I2C_ADDR, MAX30102_REG_FIFO_RD_PTR, 0x00);

  /* Enable SpO2 mode (Red + IR active) */
  if (i2c_write_reg(i2c, MAX30102_I2C_ADDR, MAX30102_REG_MODE_CONFIG,
                    MAX30102_MODE_SPO2) != 0) {
    return -1;
  }

  /* Enable FIFO almost-full interrupt */
  if (i2c_write_reg(i2c, MAX30102_I2C_ADDR, MAX30102_REG_INT_ENABLE_1, 0x40) !=
      0) {
    return -1;
  }

  dev->initialized = 1;
  return 0;
}

/* FIFO Operations */

int max30102_fifo_available(max30102_t *dev) {
  if (!dev || !dev->initialized)
    return -1;

  int wr = i2c_read_reg(dev->i2c, MAX30102_I2C_ADDR, MAX30102_REG_FIFO_WR_PTR);
  int rd = i2c_read_reg(dev->i2c, MAX30102_I2C_ADDR, MAX30102_REG_FIFO_RD_PTR);
  if (wr < 0 || rd < 0)
    return -1;

  int count = wr - rd;
  if (count < 0)
    count += 32; /* FIFO is 32 entries deep */
  return count;
}

int max30102_read_sample(max30102_t *dev, max30102_sample_t *sample) {
  if (!dev || !dev->initialized || !sample)
    return -1;

  /*
   * In SpO2 mode, each FIFO entry = 6 bytes:
   *   [RED_MSB, RED_MID, RED_LSB, IR_MSB, IR_MID, IR_LSB]
   * Each value is 18-bit, stored in bits [17:0] of the 3 bytes.
   */
  uint8_t fifo_data[6];
  if (i2c_write_read(dev->i2c, MAX30102_I2C_ADDR, MAX30102_REG_FIFO_DATA,
                     fifo_data, 6) != 0) {
    return -1;
  }

  /* Extract 18-bit values (mask off upper 2 bits of MSB) */
  sample->red = ((uint32_t)(fifo_data[0] & 0x03) << 16) |
                ((uint32_t)fifo_data[1] << 8) | ((uint32_t)fifo_data[2]);

  sample->ir = ((uint32_t)(fifo_data[3] & 0x03) << 16) |
               ((uint32_t)fifo_data[4] << 8) | ((uint32_t)fifo_data[5]);

  return 0;
}

int max30102_read_fifo(max30102_t *dev, max30102_sample_t *buf,
                       int max_samples) {
  if (!dev || !buf || max_samples <= 0)
    return -1;

  int available = max30102_fifo_available(dev);
  if (available <= 0)
    return available;

  int to_read = (available < max_samples) ? available : max_samples;
  for (int i = 0; i < to_read; i++) {
    if (max30102_read_sample(dev, &buf[i]) != 0) {
      return i; /* Return how many we got before the error */
    }
  }
  return to_read;
}

/* Temperature */

float max30102_read_temperature(max30102_t *dev) {
  if (!dev || !dev->initialized)
    return -999.0f;

  /* Trigger a one-shot temperature measurement */
  if (i2c_write_reg(dev->i2c, MAX30102_I2C_ADDR, MAX30102_REG_TEMP_CONFIG,
                    0x01) != 0) {
    return -999.0f;
  }

/* Wait for measurement (typically ~30ms) */
    int timeout = 100;
    while (timeout-- > 0) {
        int val =
            i2c_read_reg(dev->i2c, MAX30102_I2C_ADDR, MAX30102_REG_TEMP_CONFIG);
        if (val < 0)
            return -999.0f;
        if (!(val & 0x01))
            break;
#ifdef _WIN32
        Sleep(1);
#else
        usleep(1000);
#endif
    }
    if (timeout <= 0)
        return -999.0f;

  /* Read integer + fractional parts */
  int temp_int =
      i2c_read_reg(dev->i2c, MAX30102_I2C_ADDR, MAX30102_REG_TEMP_INT);
  int temp_frac =
      i2c_read_reg(dev->i2c, MAX30102_I2C_ADDR, MAX30102_REG_TEMP_FRAC);
  if (temp_int < 0 || temp_frac < 0)
    return -999.0f;

  /* Temperature = TINT + (TFRAC × 0.0625) */
  return (float)(int8_t)temp_int + ((float)temp_frac * 0.0625f);
}

/* Power Management */

int max30102_shutdown(max30102_t *dev) {
  if (!dev || !dev->initialized)
    return -1;
  int mode =
      i2c_read_reg(dev->i2c, MAX30102_I2C_ADDR, MAX30102_REG_MODE_CONFIG);
  if (mode < 0)
    return -1;
  return i2c_write_reg(dev->i2c, MAX30102_I2C_ADDR, MAX30102_REG_MODE_CONFIG,
                       (uint8_t)(mode | MAX30102_MODE_SHDN));
}

int max30102_wakeup(max30102_t *dev) {
    if (!dev || !dev->initialized)
        return -1;
    int mode =
        i2c_read_reg(dev->i2c, MAX30102_I2C_ADDR, MAX30102_REG_MODE_CONFIG);
    if (mode < 0)
        return -1;
    return i2c_write_reg(dev->i2c, MAX30102_I2C_ADDR, MAX30102_REG_MODE_CONFIG,
                         (uint8_t)(mode & ~MAX30102_MODE_SHDN));
}

/* Adaptive LED Current Control */
int max30102_adjust_led_current(max30102_t *dev, uint32_t red_sample, uint32_t ir_sample) {
    if (!dev || !dev->initialized)
        return -1;

    const uint32_t TARGET_LOW  = (1u << 17);   /* ~50% of 18-bit range */
    const uint32_t TARGET_HIGH = (1u << 18) * 3 / 4;  /* ~75% of 18-bit range */
    const uint8_t STEP = 4;  /* Adjustment step (0.8mA per LSB) */

    uint8_t red_pa = i2c_read_reg(dev->i2c, MAX30102_I2C_ADDR, MAX30102_REG_LED1_PA);
    uint8_t ir_pa  = i2c_read_reg(dev->i2c, MAX30102_I2C_ADDR, MAX30102_REG_LED2_PA);
    if (red_pa == 0xFF || ir_pa == 0xFF)
        return -1;

    /* Adjust Red LED */
    if (red_sample < TARGET_LOW && red_pa < 255 - STEP) {
        red_pa += STEP;
    } else if (red_sample > TARGET_HIGH && red_pa > STEP) {
        red_pa -= STEP;
    }

    /* Adjust IR LED */
    if (ir_sample < TARGET_LOW && ir_pa < 255 - STEP) {
        ir_pa += STEP;
    } else if (ir_sample > TARGET_HIGH && ir_pa > STEP) {
        ir_pa -= STEP;
    }

    if (i2c_write_reg(dev->i2c, MAX30102_I2C_ADDR, MAX30102_REG_LED1_PA, red_pa) != 0)
        return -1;
    if (i2c_write_reg(dev->i2c, MAX30102_I2C_ADDR, MAX30102_REG_LED2_PA, ir_pa) != 0)
        return -1;

    return 0;
}
