// ================================================================
// TC-003: Detección de impacto y logging
// Cubre RF-03, RF-05, RF-06, RF-09
// ================================================================
// Conexiones: FSR a GPIO34, Motor/BUZZER a GPIO25
// Presiona fuerte el FSR → debe aparecer [WARNING] en el monitor
// Desconecta el FSR → debe aparecer [ERROR] ERR_001
// ================================================================

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_timer.h"

#define FSR_CH          ADC_CHANNEL_6
#define PIN_MOTOR       GPIO_NUM_25
#define UMBRAL          700
#define FSR_MIN         50
#define FSR_MAX         950

int buf5[5] = {0}; 
int idx5 = 0;

int filtro5(int v) {
    buf5[idx5] = v; 
    idx5 = (idx5+1)%5;
    int s=0; 
    for(int i=0;i<5;i++) s+=buf5[i]; 
    return s/5;
}

void log_evento(const char *lvl, const char *cod, int val, const char *msg) {
    char l[128];
    int64_t t = esp_timer_get_time()/1000;
    if (cod) {
        snprintf(l,sizeof(l),"[%s] T=%lldms | FSR=%d | %s | %s\n",lvl,t,val,cod,msg);
    } else {
        snprintf(l,sizeof(l),"[%s] T=%lldms | FSR=%d | %s\n",lvl,t,val,msg);
    }
    uart_write_bytes(UART_NUM_0, l, strlen(l));
}

void app_main(void) {
    uart_config_t ucfg = {.baud_rate=115200,.data_bits=UART_DATA_8_BITS,
        .parity=UART_PARITY_DISABLE,.stop_bits=UART_STOP_BITS_1,
        .flow_ctrl=UART_HW_FLOWCTRL_DISABLE,.source_clk=UART_SCLK_DEFAULT};
    uart_param_config(UART_NUM_0,&ucfg);
    uart_driver_install(UART_NUM_0,512,0,0,NULL,0);

    adc_oneshot_unit_handle_t adc1;
    adc_oneshot_new_unit(&(adc_oneshot_unit_init_cfg_t){.unit_id=ADC_UNIT_1},&adc1);
    adc_oneshot_config_channel(adc1,FSR_CH,
        &(adc_oneshot_chan_cfg_t){.bitwidth=ADC_BITWIDTH_12,.atten=ADC_ATTEN_DB_11});
    adc_cali_handle_t cali;
    adc_cali_create_scheme_line_fitting(
        &(adc_cali_line_fitting_config_t){.unit_id=ADC_UNIT_1,
        .atten=ADC_ATTEN_DB_11,.bitwidth=ADC_BITWIDTH_12},&cali);

    gpio_config_t gcfg = {.pin_bit_mask=(1ULL<<PIN_MOTOR),.mode=GPIO_MODE_OUTPUT,
        .pull_up_en=GPIO_PULLUP_DISABLE,.pull_down_en=GPIO_PULLDOWN_DISABLE,
        .intr_type=GPIO_INTR_DISABLE};
    gpio_config(&gcfg);
    gpio_set_level(PIN_MOTOR,0);

    int impactos=0;
    uart_write_bytes(UART_NUM_0,"TC-003: Presiona el FSR fuerte para impacto\n",41);

    while (1) {
        int raw=0; 
        adc_oneshot_read(adc1,FSR_CH,&raw);
        int fil = filtro5(raw);
        int mv=0;
        adc_cali_raw_to_voltage(cali, fil, &mv);

        if (raw < FSR_MIN) {
            log_evento("ERROR","ERR_001",raw,"Sensor FSR desconectado");
        } else if (raw > FSR_MAX) {
            log_evento("WARNING","ERR_002",raw,"Saturacion del sensor");
        } else if (fil > UMBRAL) {
            impactos++;
            log_evento("WARNING",NULL,fil,"Impacto excesivo detectado");
            gpio_set_level(PIN_MOTOR,1);
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_level(PIN_MOTOR,0);
        } else {
            log_evento("INFO",NULL,fil,"Presion normal");
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}