#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "driver/dac_continuous.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"
#include "driver/timer.h"


#define MCP4132   ADC_CHANNEL_6
#define ADC_WIDTH       ADC_BITWIDTH_12 
#define ADC_ATTEN       ADC_ATTEN_DB_11 

#define UART_PORT       UART_NUM_0
#define TICK_1000MS  1000000ULL
static float señal = 0.0f;   

#define PIN_MOSI 23
#define PIN_MISO 25
#define PIN_CLK 18
#define PIN_CS 5

static adc_oneshot_unit_handle_t adc_handle; 
static adc_cali_handle_t adc_cali;            
spi_device_handle_t spi_device_handle;

static volatile bool bandera_tick = false;           
static char uart_buffer[64];
static int uart_idx = 0;

static bool IRAM_ATTR timer_isr_tick(void *arg) { 
    bandera_tick = true;      
    return true; 
}

void spi_bus_initi(void) {

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };
    spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 1000,   // 1 kHz
        .mode = 2,                    // SPI Mode 2 
        .spics_io_num = PIN_CS,
        .queue_size = 7
    };

    spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi_device_handle);
}

void mcp4132_write_register(uint8_t reg, uint8_t data) {
    uint8_t tx[2] = { reg, data };
    spi_transaction_t trans = {
        .length = 16,        // 16 bits = 2 bytes
        .tx_buffer = tx
    };
    spi_device_transmit(spi_device_handle, &trans);
}

void mcp4132_read_register(uint8_t reg, uint8_t data) {
    uint8_t rx[2] = { reg, data };
    spi_transaction_t trans = {
        .length = 16,        // 16 bits = 2 bytes
        .rx_buffer = rx
    };
    return((rx[0] >> 4) + (rx[1]));
    spi_device_transmit(spi_device_handle, &trans);
}

uint8_t mcp4132_set_cutoff_frequency(uint8_t bcd) {

    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

uint8_t mcp4132_set_wiper(uint8_t dec) {

    return ((dec / 10) << 4) | (dec % 10);  
}

int64_t ultimo_uart = esp_timer_get_time();

void app_main(void) {

    int64_t ahora = esp_timer_get_time();

    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1
    };
    adc_oneshot_new_unit(&init_cfg, &adc_handle);
    
    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_WIDTH,
        .atten = ADC_ATTEN
    };
    
    adc_oneshot_config_channel(adc_handle, MCP4132, &chan_cfg);
    
    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_WIDTH
    };
    adc_cali_create_scheme_line_fitting(&cali_cfg, &adc_cali);  
    
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };
    uart_param_config(UART_PORT, &uart_config);
    uart_driver_install(UART_PORT, 256, 0, 0, NULL, 0);

    timer_config_t timer_cfg_tick = { 
        .divider=80, 
        .counter_dir=TIMER_COUNT_UP, 
        .counter_en=TIMER_PAUSE, 
        .alarm_en=TIMER_ALARM_EN, 
        .auto_reload=true 
    };
    timer_init(TIMER_GROUP_0, TIMER_0, &timer_cfg_tick);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, TICK_1000MS);
    timer_isr_callback_add(TIMER_GROUP_0, TIMER_0, timer_isr_tick, NULL, 0);
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_start(TIMER_GROUP_0, TIMER_0);

    if (bandera_tick) {
            bandera_tick = false;
            
            int adc_raw = 0;
            int voltaje_mv = 0;
            adc_oneshot_read(adc_handle, MCP4132, &adc_raw);
            adc_cali_raw_to_voltage(adc_cali, adc_raw, &voltaje_mv);
            señal = voltaje_mv;
            
            if (señal >= 1400) {
                mcp4132_set_wiper (95); // N = 95
            } else if (señal < 900) {
                mcp4132_set_wiper (42); // N = 42
    }

     if ((ahora - ultimo_uart) >= 1000000) {
            ultimo_uart = ahora;
            printf("T=%.1f\n",
                   mcp4132_set_wiper ());
    }
}
}









