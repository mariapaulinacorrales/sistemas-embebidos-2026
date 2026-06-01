// ================================================================
// TC-004: Prueba de pantalla OLED SSD1306 por I2C
// Cubre: RF-08
// ================================================================
// CONEXIONES:
//   GPIO21 (SDA) → pin SDA de la OLED
//   GPIO22 (SCL) → pin SCL de la OLED
//   3.3V → VCC de la OLED
//   GND  → GND de la OLED
// ================================================================

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "driver/uart.h"

#define I2C_PORT        I2C_NUM_0
#define I2C_SDA_PIN     GPIO_NUM_21
#define I2C_SCL_PIN     GPIO_NUM_22
#define I2C_FREQ_HZ     400000
#define OLED_ADDR       0x3C

#define OLED_CMD_DISPLAY_OFF    0xAE
#define OLED_CMD_DISPLAY_ON     0xAF
#define OLED_CMD_SET_CONTRAST   0x81
#define OLED_CMD_CHARGE_PUMP    0x8D
#define OLED_CMD_MEM_MODE       0x20
#define OLED_CMD_SET_PAGE_ADDR  0xB0
#define OLED_CMD_SET_COL_LOW    0x00
#define OLED_CMD_SET_COL_HIGH   0x10
#define OLED_CMD_SEG_REMAP      0xA1
#define OLED_CMD_COM_SCAN_DEC   0xC8
#define OLED_CMD_SET_DISPLAY_OFFSET 0xD3
#define OLED_CMD_SET_START_LINE 0x40
#define OLED_CMD_SET_MUX_RATIO  0xA8
#define OLED_CMD_SET_CLK_DIV    0xD5
#define OLED_CMD_SET_PRECHARGE  0xD9
#define OLED_CMD_SET_COM_PINS   0xDA
#define OLED_CMD_SET_VCOMH      0xDB
#define OLED_CMD_DISPLAY_RAM    0xA4
#define OLED_CMD_NORMAL_DISPLAY 0xA6

#define OLED_WIDTH  128
#define OLED_PAGES  8

static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // ' '
    {0x00,0x00,0x5F,0x00,0x00}, // '!'
    {0x00,0x07,0x00,0x07,0x00}, // '"'
    {0x14,0x7F,0x14,0x7F,0x14}, // '#'
    {0x24,0x2A,0x7F,0x2A,0x12}, // '$'
    {0x23,0x13,0x08,0x64,0x62}, // '%'
    {0x36,0x49,0x55,0x22,0x50}, // '&'
    {0x00,0x05,0x03,0x00,0x00}, // '''
    {0x00,0x1C,0x22,0x41,0x00}, // '('
    {0x00,0x41,0x22,0x1C,0x00}, // ')'
    {0x14,0x08,0x3E,0x08,0x14}, // '*'
    {0x08,0x08,0x3E,0x08,0x08}, // '+'
    {0x00,0x50,0x30,0x00,0x00}, // ','
    {0x08,0x08,0x08,0x08,0x08}, // '-'
    {0x00,0x60,0x60,0x00,0x00}, // '.'
    {0x20,0x10,0x08,0x04,0x02}, // '/'
    {0x3E,0x51,0x49,0x45,0x3E}, // '0'
    {0x00,0x42,0x7F,0x40,0x00}, // '1'
    {0x42,0x61,0x51,0x49,0x46}, // '2'
    {0x21,0x41,0x45,0x4B,0x31}, // '3'
    {0x18,0x14,0x12,0x7F,0x10}, // '4'
    {0x27,0x45,0x45,0x45,0x39}, // '5'
    {0x3C,0x4A,0x49,0x49,0x30}, // '6'
    {0x01,0x71,0x09,0x05,0x03}, // '7'
    {0x36,0x49,0x49,0x49,0x36}, // '8'
    {0x06,0x49,0x49,0x29,0x1E}, // '9'
    {0x00,0x36,0x36,0x00,0x00}, // ':'
    {0x00,0x56,0x36,0x00,0x00}, // ';'
    {0x08,0x14,0x22,0x41,0x00}, // '<'
    {0x14,0x14,0x14,0x14,0x14}, // '='
    {0x00,0x41,0x22,0x14,0x08}, // '>'
    {0x02,0x01,0x51,0x09,0x06}, // '?'
    {0x32,0x49,0x79,0x41,0x3E}, // '@'
    {0x7E,0x11,0x11,0x11,0x7E}, // 'A'
    {0x7F,0x49,0x49,0x49,0x36}, // 'B'
    {0x3E,0x41,0x41,0x41,0x22}, // 'C'
    {0x7F,0x41,0x41,0x22,0x1C}, // 'D'
    {0x7F,0x49,0x49,0x49,0x41}, // 'E'
    {0x7F,0x09,0x09,0x09,0x01}, // 'F'
    {0x3E,0x41,0x49,0x49,0x7A}, // 'G'
    {0x7F,0x08,0x08,0x08,0x7F}, // 'H'
    {0x00,0x41,0x7F,0x41,0x00}, // 'I'
    {0x20,0x40,0x41,0x3F,0x01}, // 'J'
    {0x7F,0x08,0x14,0x22,0x41}, // 'K'
    {0x7F,0x40,0x40,0x40,0x40}, // 'L'
    {0x7F,0x02,0x0C,0x02,0x7F}, // 'M'
    {0x7F,0x04,0x08,0x10,0x7F}, // 'N'
    {0x3E,0x41,0x41,0x41,0x3E}, // 'O'
    {0x7F,0x09,0x09,0x09,0x06}, // 'P'
    {0x3E,0x41,0x51,0x21,0x5E}, // 'Q'
    {0x7F,0x09,0x19,0x29,0x46}, // 'R'
    {0x46,0x49,0x49,0x49,0x31}, // 'S'
    {0x01,0x01,0x7F,0x01,0x01}, // 'T'
    {0x3F,0x40,0x40,0x40,0x3F}, // 'U'
    {0x1F,0x20,0x40,0x20,0x1F}, // 'V'
    {0x3F,0x40,0x38,0x40,0x3F}, // 'W'
    {0x63,0x14,0x08,0x14,0x63}, // 'X'
    {0x07,0x08,0x70,0x08,0x07}, // 'Y'
    {0x61,0x51,0x49,0x45,0x43}, // 'Z'
};

esp_err_t oled_cmd(uint8_t cmd) {
    i2c_cmd_handle_t h = i2c_cmd_link_create();
    i2c_master_start(h);
    i2c_master_write_byte(h, (OLED_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(h, 0x00, true);
    i2c_master_write_byte(h, cmd, true);
    i2c_master_stop(h);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, h, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(h);
    return ret;
}

esp_err_t oled_data(const uint8_t *d, size_t len) {
    i2c_cmd_handle_t h = i2c_cmd_link_create();
    i2c_master_start(h);
    i2c_master_write_byte(h, (OLED_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(h, 0x40, true);
    i2c_master_write(h, (uint8_t *)d, len, true);
    i2c_master_stop(h);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, h, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(h);
    return ret;
}

void oled_init(void) {
    oled_cmd(OLED_CMD_DISPLAY_OFF);
    oled_cmd(OLED_CMD_SET_CLK_DIV);    oled_cmd(0x80);
    oled_cmd(OLED_CMD_SET_MUX_RATIO);  oled_cmd(0x3F);
    oled_cmd(OLED_CMD_SET_DISPLAY_OFFSET); oled_cmd(0x00);
    oled_cmd(OLED_CMD_SET_START_LINE | 0x00);
    oled_cmd(OLED_CMD_CHARGE_PUMP);    oled_cmd(0x14);
    oled_cmd(OLED_CMD_SEG_REMAP);
    oled_cmd(OLED_CMD_COM_SCAN_DEC);
    oled_cmd(OLED_CMD_SET_COM_PINS);   oled_cmd(0x12);
    oled_cmd(OLED_CMD_SET_CONTRAST);   oled_cmd(0xCF);
    oled_cmd(OLED_CMD_SET_PRECHARGE);  oled_cmd(0xF1);
    oled_cmd(OLED_CMD_SET_VCOMH);      oled_cmd(0x40);
    oled_cmd(OLED_CMD_DISPLAY_RAM);
    oled_cmd(OLED_CMD_NORMAL_DISPLAY);
    oled_cmd(OLED_CMD_MEM_MODE);       oled_cmd(0x00);
    oled_cmd(OLED_CMD_DISPLAY_ON);
}

void oled_clear(void) {
    uint8_t zeros[OLED_WIDTH];
    memset(zeros, 0, sizeof(zeros));
    for (uint8_t p = 0; p < OLED_PAGES; p++) {
        oled_cmd(OLED_CMD_SET_PAGE_ADDR | p);
        oled_cmd(OLED_CMD_SET_COL_LOW  | 0x00);
        oled_cmd(OLED_CMD_SET_COL_HIGH | 0x00);
        oled_data(zeros, OLED_WIDTH);
    }
}

void oled_cursor(uint8_t pagina, uint8_t col) {
    oled_cmd(OLED_CMD_SET_PAGE_ADDR | (pagina & 0x07));
    oled_cmd(OLED_CMD_SET_COL_LOW  | (col & 0x0F));
    oled_cmd(OLED_CMD_SET_COL_HIGH | ((col >> 4) & 0x0F));
}

void oled_char(char c) {
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c < 32 || c > 90) c = ' ';
    int idx = c - 32;
    oled_data(font5x7[idx], 5);
    uint8_t sp = 0x00;
    oled_data(&sp, 1);
}

void oled_str(uint8_t pagina, uint8_t col, const char *txt) {
    oled_cursor(pagina, col);
    while (*txt) { oled_char(*txt++); }
}

void app_main(void) {
    uart_config_t ucfg = {.baud_rate=115200, .data_bits=UART_DATA_8_BITS,
        .parity=UART_PARITY_DISABLE, .stop_bits=UART_STOP_BITS_1,
        .flow_ctrl=UART_HW_FLOWCTRL_DISABLE, .source_clk=UART_SCLK_DEFAULT};
    uart_param_config(UART_NUM_0, &ucfg);
    uart_driver_install(UART_NUM_0, 512, 0, 0, NULL, 0);

    i2c_config_t i2c_conf = {.mode=I2C_MODE_MASTER, .sda_io_num=I2C_SDA_PIN,
        .scl_io_num=I2C_SCL_PIN, .sda_pullup_en=GPIO_PULLUP_ENABLE,
        .scl_pullup_en=GPIO_PULLUP_ENABLE, .master.clk_speed=I2C_FREQ_HZ};
    i2c_param_config(I2C_PORT, &i2c_conf);
    i2c_driver_install(I2C_PORT, I2C_MODE_MASTER, 0, 0, 0);

    uart_write_bytes(UART_NUM_0, "TC-004: Prueba OLED SSD1306\n", 28);
    
    oled_init();
    oled_clear();
    oled_str(0, 10, "PRUEBA OLED");
    oled_str(2, 15, "TC-004");
    oled_str(4, 5,  "SMART INSOLE");
    oled_str(6, 0,  "PASSED/FAILED?");
    
    uart_write_bytes(UART_NUM_0, "Si ves texto en la OLED -> PASSED\n", 34);
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}