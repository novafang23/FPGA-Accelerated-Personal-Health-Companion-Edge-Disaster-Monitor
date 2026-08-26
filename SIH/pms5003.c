/*
 * pms5003.c
 * PMS5003 Particulate Matter Sensor Driver Implementation
 */

#include "pms5003.h"

#ifdef ZYNQ_HW
#include "xil_io.h"

/* Zynq UART register offsets */
#define UART_SR     0x2C  /* Channel Status Register */
#define UART_FIFO   0x30  /* TX/RX FIFO              */
#define SR_RXEMPTY  (1 << 1)
#endif

/* Internal: Parse a complete frame */
static pms5003_data_t last_data;
static int has_valid_data = 0;

static uint16_t extract_u16(const uint8_t *buf, int offset) {
    return (uint16_t)((buf[offset] << 8) | buf[offset + 1]);
}

static int pms5003_parse_frame(pms5003_t *dev) {
    const uint8_t *buf = dev->rx_buf;

    /* Verify start bytes */
    if (buf[0] != PMS5003_START_BYTE_1 || buf[1] != PMS5003_START_BYTE_2) {
        return -1;
    }

    /* Verify checksum: sum of bytes 0–29 == bytes 30–31 */
    uint16_t checksum = 0;
    for (int i = 0; i < 30; i++) {
        checksum += buf[i];
    }
    uint16_t expected = extract_u16(buf, 30);
    if (checksum != expected) {
        return -1;  /* Checksum mismatch — corrupt frame */
    }

    /* Extract all fields */
    last_data.pm1_0_cf1  = extract_u16(buf, 4);
    last_data.pm2_5_cf1  = extract_u16(buf, 6);
    last_data.pm10_cf1   = extract_u16(buf, 8);

    last_data.pm1_0_atm  = extract_u16(buf, 10);
    last_data.pm2_5_atm  = extract_u16(buf, 12);
    last_data.pm10_atm   = extract_u16(buf, 14);

    last_data.count_0_3um = extract_u16(buf, 16);
    last_data.count_0_5um = extract_u16(buf, 18);
    last_data.count_1_0um = extract_u16(buf, 20);
    last_data.count_2_5um = extract_u16(buf, 22);
    last_data.count_5_0um = extract_u16(buf, 24);
    last_data.count_10um  = extract_u16(buf, 26);

    last_data.valid = 1;
    has_valid_data = 1;

    return 0;
}

/* Public API */

int pms5003_init(pms5003_t *dev, uint32_t uart_base) {
    if (!dev) return -1;

    dev->uart_base   = uart_base;
    dev->rx_pos      = 0;
    dev->synced      = 0;
    dev->initialized = 1;

    last_data.valid = 0;
    has_valid_data  = 0;

    return 0;
}

int pms5003_feed_byte(pms5003_t *dev, uint8_t byte) {
    if (!dev || !dev->initialized) return 0;

    if (!dev->synced) {
        /* Looking for start byte 1 */
        if (byte == PMS5003_START_BYTE_1) {
            dev->rx_buf[0] = byte;
            dev->rx_pos = 1;
            dev->synced = 1;
        }
        return 0;
    }

    if (dev->rx_pos == 1) {
        /* Expecting start byte 2 */
        if (byte == PMS5003_START_BYTE_2) {
            dev->rx_buf[1] = byte;
            dev->rx_pos = 2;
        } else {
            /* Resync */
            dev->synced = 0;
            dev->rx_pos = 0;
        }
        return 0;
    }

    /* Accumulate frame bytes */
    dev->rx_buf[dev->rx_pos++] = byte;

    if (dev->rx_pos >= PMS5003_FRAME_LEN) {
        /* Complete frame received — try to parse */
        dev->synced = 0;
        dev->rx_pos = 0;

        if (pms5003_parse_frame(dev) == 0) {
            return 1;  /* Valid frame decoded */
        }
    }

    return 0;
}

int pms5003_get_data(const pms5003_t *dev, pms5003_data_t *data) {
    (void)dev;
    if (!has_valid_data || !data) return -1;

    *data = last_data;
    return 0;
}

int pms5003_read_blocking(pms5003_t *dev, pms5003_data_t *data, int timeout) {
    if (!dev || !dev->initialized || !data) return -1;

#ifdef ZYNQ_HW
    int loops = timeout;
    while (loops-- > 0) {
        /* Check if UART RX FIFO has data */
        uint32_t status = Xil_In32(dev->uart_base + UART_SR);
        if (!(status & SR_RXEMPTY)) {
            uint8_t byte = (uint8_t)(Xil_In32(dev->uart_base + UART_FIFO) & 0xFF);
            if (pms5003_feed_byte(dev, byte) == 1) {
                *data = last_data;
                return 0;
            }
        }
    }
    return -1;  /* Timeout */
#else
    /* PC simulation: return synthetic "clean air" data */
    (void)timeout;
    data->pm1_0_cf1  = 8;
    data->pm2_5_cf1  = 12;
    data->pm10_cf1   = 18;
    data->pm1_0_atm  = 8;
    data->pm2_5_atm  = 12;
    data->pm10_atm   = 18;
    data->count_0_3um = 2100;
    data->count_0_5um = 610;
    data->count_1_0um = 80;
    data->count_2_5um = 8;
    data->count_5_0um = 2;
    data->count_10um  = 0;
    data->valid = 1;
    return 0;
#endif
}

/*
 * India NAQI (National Air Quality Index) breakpoints for PM2.5:
 *   Good:         0–30 µg/m³
 *   Satisfactory: 31–60
 *   Moderate:     61–90
 *   Poor:         91–120
 *   Very Poor:    121–250
 *   Severe:       >250
 */
const char* pms5003_aqi_category(uint16_t pm25) {
    if (pm25 <= 30)  return "Good";
    if (pm25 <= 60)  return "Satisfactory";
    if (pm25 <= 90)  return "Moderate";
    if (pm25 <= 120) return "Poor";
    if (pm25 <= 250) return "Very Poor";
    return "Severe";
}
