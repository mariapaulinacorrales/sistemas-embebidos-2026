// ================================================================
// TC-001: Verificar lectura y filtro del sensor FSR
// Cubre RF-01 y RF-02
// ================================================================
// Conexiones: FSR conectado a GPIO34 (ADC_CHANNEL_6)
// Abre monitor serial a 115200 baudios
// Presiona el FSR y observa los valores
// El valor "Filtrado" debe ser más suave que el "Raw"
// ================================================================

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include <string.h>

#define FSR_CH ADC_CHANNEL_6
#define FILTRO_N 5

int buffer[FILTRO_N] = {0};
int idx = 0;

int filtro(int nueva) {
    buffer[idx] = nueva;
    idx = (idx + 1) % FILTRO_N;
    int suma = 0;
    for (int i = 0; i < FILTRO_N; i++) suma += buffer[i];
    return suma / FILTRO_N;
}

void app_main(void) {
    uart_config_t ucfg = {.baud_rate=115200, .data_bits=UART_DATA_8_BITS,
        .parity=UART_PARITY_DISABLE, .stop_bits=UART_STOP_BITS_1,
        .flow_ctrl=UART_HW_FLOWCTRL_DISABLE, .source_clk=UART_SCLK_DEFAULT};
    uart_param_config(UART_NUM_0, &ucfg);
    uart_driver_install(UART_NUM_0, 512, 0, 0, NULL, 0);

    adc_oneshot_unit_handle_t adc1;
    adc_oneshot_new_unit(&(adc_oneshot_unit_init_cfg_t){.unit_id=ADC_UNIT_1}, &adc1);
    adc_oneshot_config_channel(adc1, FSR_CH,
        &(adc_oneshot_chan_cfg_t){.bitwidth=ADC_BITWIDTH_12, .atten=ADC_ATTEN_DB_11});

    adc_cali_handle_t cali;
    adc_cali_create_scheme_line_fitting(
        &(adc_cali_line_fitting_config_t){.unit_id=ADC_UNIT_1,
        .atten=ADC_ATTEN_DB_11, .bitwidth=ADC_BITWIDTH_12}, &cali);

    char buf[128];
    uart_write_bytes(UART_NUM_0, "TC-001: Prueba de FSR con filtro\n", 33);
    
    while (1) {
        int raw = 0, mv = 0;
        adc_oneshot_read(adc1, FSR_CH, &raw);
        adc_cali_raw_to_voltage(cali, raw, &mv);
        int fil = filtro(raw);
        snprintf(buf, sizeof(buf),
            "Raw=%4d | Filtrado=%4d | mV=%4d | %s\n",
            raw, fil, mv,
            raw < 50 ? "DESCONECTADO" : raw > 950 ? "SATURADO" : "OK");
        uart_write_bytes(UART_NUM_0, buf, strlen(buf));
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}