/*
 * ssd1306.h — SSD1306 0.96" OLED Display Driver (ESP-IDF Port)
 * SIH26181: AI-Powered Personal Health Companion & Edge Disaster Monitor
 *
 * Displays real-time vitals (HR, SpO2, temperature, PM2.5) and AI hazard alerts.
 * Uses a 128×64 pixel framebuffer with 5×7 font rendering.
 *
 * Panel: 0.96" 128×64 (confirmed against the physical board). The init sequence
 * sets SET_MUX_RATIO to 0x3F, which is the 64-row value -- a 0.91" 128×32 panel
 * would need 0x1F, so this driver is for the 128×64 part only. The BOM previously
 * described the wrong panel; see hardware/shrikefi/pcb/SIH26181_ShrikeFi_Wearable_BOM.csv.
 */

#ifndef SHRIKEFI_SSD1306_H
#define SHRIKEFI_SSD1306_H

#include <stdint.h>
#include <stddef.h>
#include "esp32_i2c_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Display dimensions */
#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  64
#define SSD1306_PAGES   (SSD1306_HEIGHT / 8)  /* 8 pages of 8 pixels each */

/* I2C Address */
#define SSD1306_I2C_ADDR  0x3C
#define SSD1306_I2C_ALT   0x3D

/* I2C control bytes */
#define SSD1306_CMD_SINGLE    0x80  /* Single command byte follows   */
#define SSD1306_CMD_STREAM    0x00  /* Multiple command bytes follow */
#define SSD1306_DATA_STREAM   0x40  /* Data bytes follow             */

/* Fundamental commands */
#define SSD1306_SET_CONTRAST        0x81
#define SSD1306_DISPLAY_ALL_ON_RES  0xA4
#define SSD1306_DISPLAY_ALL_ON      0xA5
#define SSD1306_NORMAL_DISPLAY      0xA6
#define SSD1306_INVERT_DISPLAY      0xA7
#define SSD1306_DISPLAY_OFF         0xAE
#define SSD1306_DISPLAY_ON          0xAF

/* Scrolling commands */
#define SSD1306_DEACTIVATE_SCROLL   0x2E

/* Addressing mode */
#define SSD1306_SET_MEM_ADDR_MODE   0x20
#define SSD1306_HORIZONTAL_MODE     0x00
#define SSD1306_VERTICAL_MODE       0x01
#define SSD1306_PAGE_MODE           0x02

/* Column and page address */
#define SSD1306_SET_COL_ADDR        0x21
#define SSD1306_SET_PAGE_ADDR       0x22

/* Hardware configuration */
#define SSD1306_SET_START_LINE      0x40
#define SSD1306_SET_SEG_REMAP       0xA1
#define SSD1306_SET_MUX_RATIO       0xA8
#define SSD1306_SET_COM_SCAN_DIR    0xC8
#define SSD1306_SET_DISPLAY_OFFSET  0xD3
#define SSD1306_SET_COM_PINS        0xDA

/* Timing */
#define SSD1306_SET_CLOCK_DIV       0xD5
#define SSD1306_SET_PRECHARGE       0xD9
#define SSD1306_SET_VCOM_DESELECT   0xDB

/* Charge pump */
#define SSD1306_CHARGE_PUMP_SETTING 0x8D
#define SSD1306_CHARGE_PUMP_ENABLE  0x14

/* Driver State */
typedef struct {
    esp32_i2c_handle_t *i2c;
    uint8_t             addr;
    uint8_t             framebuf[SSD1306_WIDTH * SSD1306_PAGES]; /* 1024 bytes */
    int                 initialized;
} ssd1306_t;

/*
 * Initialize the SSD1306 OLED display at given I2C address.
 * Returns 0 on success, -1 on failure.
 */
int ssd1306_init(ssd1306_t *disp, esp32_i2c_handle_t *i2c, uint8_t addr);

/*
 * Clear the entire framebuffer (all pixels off).
 */
void ssd1306_clear(ssd1306_t *disp);

/*
 * Flush the framebuffer to the display via I2C.
 * Returns 0 on success, -1 on failure.
 */
int ssd1306_update(ssd1306_t *disp);

/*
 * Set or clear a single pixel.
 */
void ssd1306_set_pixel(ssd1306_t *disp, int x, int y, int on);

/*
 * Draw a character at the given position using the built-in 5×7 font.
 */
void ssd1306_draw_char(ssd1306_t *disp, int x, int y, char ch);

/*
 * Draw a null-terminated string starting at (x, y).
 */
void ssd1306_draw_string(ssd1306_t *disp, int x, int y, const char *str);

/*
 * Draw a large (2x scaled) character for key metrics.
 */
void ssd1306_draw_char_large(ssd1306_t *disp, int x, int y, char ch);

/*
 * Draw a large string for key metrics.
 */
void ssd1306_draw_string_large(ssd1306_t *disp, int x, int y, const char *str);

/*
 * Draw a horizontal line.
 */
void ssd1306_draw_hline(ssd1306_t *disp, int x, int y, int width);

/*
 * Draw a filled rectangle.
 */
void ssd1306_fill_rect(ssd1306_t *disp, int x, int y, int w, int h, int on);

/*
 * Set display contrast (0x00–0xFF).
 */
int ssd1306_set_contrast(ssd1306_t *disp, uint8_t contrast);

/*
 * Turn display on or off (for power management).
 */
int ssd1306_display_on(ssd1306_t *disp, int on);

#ifdef __cplusplus
}
#endif

#endif /* SHRIKEFI_SSD1306_H */
