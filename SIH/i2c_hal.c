/*
 * i2c_hal.c — I2C Hardware Abstraction Layer Implementation
 * SIH26181: AI-Powered Personal Health Companion (Qualcomm)
 *
 * On Zynq hardware (bare-metal), this wraps XIicPs register-level access.
 * For PC simulation, it provides stubs that return success.
 */

#include "i2c_hal.h"

#ifdef ZYNQ_HW  /* Real Zynq hardware implementation */

#include "xil_io.h"

/* Zynq I2C controller register offsets */
#define I2C_CR      0x00  /* Control Register                  */
#define I2C_SR      0x04  /* Status Register                   */
#define I2C_ADDR    0x08  /* I2C Address Register              */
#define I2C_DATA    0x0C  /* I2C Data Register                 */
#define I2C_ISR     0x10  /* Interrupt Status Register         */
#define I2C_TRANS   0x14  /* Transfer Size Register            */
#define I2C_SLV_MON 0x18  /* Slave Monitor Pause Register      */
#define I2C_TIMEOUT 0x1C  /* Timeout Register                  */

/* Control register bits */
#define CR_DIV_A_SHIFT 14
#define CR_DIV_B_SHIFT 8
#define CR_CLR_FIFO    (1 << 6)
#define CR_MS          (1 << 3)  /* Master mode   */
#define CR_ACKEN       (1 << 2)  /* ACK enable    */
#define CR_HOLD        (1 << 4)  /* Hold bus      */
#define CR_RW          (1 << 0)  /* 1=Read 0=Write */

/* Status register bits */
#define SR_BA          (1 << 8)  /* Bus Active    */
#define SR_TXDV        (1 << 6)  /* TX Data Valid */
#define SR_RXDV        (1 << 5)  /* RX Data Valid */

/* Simple busy-wait timeout */
#define I2C_TIMEOUT_LOOPS  100000

static int i2c_wait_not_busy(uint32_t base_addr) {
    int timeout = I2C_TIMEOUT_LOOPS;
    while ((Xil_In32(base_addr + I2C_SR) & SR_BA) && timeout > 0) {
        timeout--;
    }
    return (timeout > 0) ? 0 : -1;
}

int i2c_init(i2c_handle_t *handle, uint32_t base_addr, uint32_t clk_freq) {
    if (!handle) return -1;

    handle->base_addr   = base_addr;
    handle->clk_freq_hz = clk_freq;
    handle->initialized = 0;

    /*
     * Configure divisors for desired SCL frequency.
     * Zynq I2C input clock = 111.111 MHz (from PS CLK).
     * SCL freq = input_clk / (22 * (DIV_A+1) * (DIV_B+1))
     *
     * For 100 kHz: DIV_A=1, DIV_B=24 → ~101 kHz
     * For 400 kHz: DIV_A=0, DIV_B=12 → ~397 kHz
     */
    uint32_t cr = CR_MS | CR_ACKEN | CR_CLR_FIFO;
    if (clk_freq >= I2C_CLK_400KHZ) {
        cr |= (0U << CR_DIV_A_SHIFT) | (12U << CR_DIV_B_SHIFT);
    } else {
        cr |= (1U << CR_DIV_A_SHIFT) | (24U << CR_DIV_B_SHIFT);
    }
    Xil_Out32(base_addr + I2C_CR, cr);

    handle->initialized = 1;
    return 0;
}

int i2c_write(const i2c_handle_t *handle, uint8_t slave_addr,
              const uint8_t *data, size_t len) {
    if (!handle || !handle->initialized || !data || len == 0) return -1;

    uint32_t base = handle->base_addr;

    if (i2c_wait_not_busy(base) != 0) return -1;

    /* Clear FIFO */
    uint32_t cr = Xil_In32(base + I2C_CR);
    Xil_Out32(base + I2C_CR, cr | CR_CLR_FIFO);

    /* Set slave address (write direction, bit 0 = 0) */
    Xil_Out32(base + I2C_ADDR, (uint32_t)slave_addr);

    /* Load TX FIFO */
    for (size_t i = 0; i < len; i++) {
        Xil_Out32(base + I2C_DATA, data[i]);
    }

    /* Set transfer size */
    Xil_Out32(base + I2C_TRANS, (uint32_t)len);

    /* Start transfer: master, write */
    cr = Xil_In32(base + I2C_CR);
    cr &= ~CR_RW;  /* Write mode */
    cr &= ~CR_HOLD;
    Xil_Out32(base + I2C_CR, cr);

    /* Wait for completion */
    return i2c_wait_not_busy(base);
}

int i2c_read(const i2c_handle_t *handle, uint8_t slave_addr,
             uint8_t *data, size_t len) {
    if (!handle || !handle->initialized || !data || len == 0) return -1;

    uint32_t base = handle->base_addr;

    if (i2c_wait_not_busy(base) != 0) return -1;

    /* Clear FIFO */
    uint32_t cr = Xil_In32(base + I2C_CR);
    Xil_Out32(base + I2C_CR, cr | CR_CLR_FIFO);

    /* Set slave address */
    Xil_Out32(base + I2C_ADDR, (uint32_t)slave_addr);

    /* Set transfer size */
    Xil_Out32(base + I2C_TRANS, (uint32_t)len);

    /* Start transfer: master, read */
    cr = Xil_In32(base + I2C_CR);
    cr |= CR_RW;  /* Read mode */
    if (len > 1) cr |= CR_HOLD;
    else         cr &= ~CR_HOLD;
    Xil_Out32(base + I2C_CR, cr);

    /* Wait for completion and read FIFO */
    if (i2c_wait_not_busy(base) != 0) return -1;

    for (size_t i = 0; i < len; i++) {
        data[i] = (uint8_t)(Xil_In32(base + I2C_DATA) & 0xFF);
    }
    return 0;
}

int i2c_write_read(const i2c_handle_t *handle, uint8_t slave_addr,
                   uint8_t reg_addr, uint8_t *data, size_t len) {
    /* Write register address with HOLD, then read */
    if (!handle || !handle->initialized || !data) return -1;

    uint32_t base = handle->base_addr;

    if (i2c_wait_not_busy(base) != 0) return -1;

    /* Clear FIFO */
    uint32_t cr = Xil_In32(base + I2C_CR);
    Xil_Out32(base + I2C_CR, cr | CR_CLR_FIFO);

    /* Set slave address and HOLD bus */
    Xil_Out32(base + I2C_ADDR, (uint32_t)slave_addr);

    /* Write the register address byte */
    Xil_Out32(base + I2C_DATA, reg_addr);
    Xil_Out32(base + I2C_TRANS, 1);

    cr = Xil_In32(base + I2C_CR);
    cr &= ~CR_RW;    /* Write mode */
    cr |= CR_HOLD;   /* Hold bus for repeated start */
    Xil_Out32(base + I2C_CR, cr);

    if (i2c_wait_not_busy(base) != 0) return -1;

    /* Now read the data bytes */
    Xil_Out32(base + I2C_TRANS, (uint32_t)len);

    cr = Xil_In32(base + I2C_CR);
    cr |= CR_RW;      /* Read mode */
    cr &= ~CR_HOLD;   /* Release bus after this transfer */
    Xil_Out32(base + I2C_CR, cr);

    if (i2c_wait_not_busy(base) != 0) return -1;

    for (size_t i = 0; i < len; i++) {
        data[i] = (uint8_t)(Xil_In32(base + I2C_DATA) & 0xFF);
    }
    return 0;
}

int i2c_write_reg(const i2c_handle_t *handle, uint8_t slave_addr,
                  uint8_t reg_addr, uint8_t value) {
    uint8_t buf[2] = {reg_addr, value};
    return i2c_write(handle, slave_addr, buf, 2);
}

int i2c_read_reg(const i2c_handle_t *handle, uint8_t slave_addr,
                 uint8_t reg_addr) {
    uint8_t val = 0;
    if (i2c_write_read(handle, slave_addr, reg_addr, &val, 1) != 0) {
        return -1;
    }
    return (int)val;
}

#else  /* PC Simulation stubs */

int i2c_init(i2c_handle_t *handle, uint32_t base_addr, uint32_t clk_freq) {
    if (!handle) return -1;
    handle->base_addr   = base_addr;
    handle->clk_freq_hz = clk_freq;
    handle->initialized = 1;
    return 0;
}

int i2c_write(const i2c_handle_t *handle, uint8_t slave_addr,
              const uint8_t *data, size_t len) {
    (void)handle; (void)slave_addr; (void)data; (void)len;
    return 0;
}

int i2c_read(const i2c_handle_t *handle, uint8_t slave_addr,
             uint8_t *data, size_t len) {
    (void)handle; (void)slave_addr;
    for (size_t i = 0; i < len; i++) data[i] = 0;
    return 0;
}

int i2c_write_read(const i2c_handle_t *handle, uint8_t slave_addr,
                   uint8_t reg_addr, uint8_t *data, size_t len) {
    (void)handle; (void)slave_addr; (void)reg_addr;
    for (size_t i = 0; i < len; i++) data[i] = 0;
    return 0;
}

int i2c_write_reg(const i2c_handle_t *handle, uint8_t slave_addr,
                  uint8_t reg_addr, uint8_t value) {
    (void)handle; (void)slave_addr; (void)reg_addr; (void)value;
    return 0;
}

int i2c_read_reg(const i2c_handle_t *handle, uint8_t slave_addr,
                 uint8_t reg_addr) {
    (void)handle; (void)slave_addr; (void)reg_addr;
    return 0;
}

#endif /* ZYNQ_HW */
