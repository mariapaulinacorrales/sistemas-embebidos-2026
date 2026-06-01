// ================================================================
// TC-002: Verificar activación del motor de vibración
// Cubre RF-04
// ================================================================
// Conexiones: Motor/Buzzer conectado a GPIO25 (a través de MOSFET)
// El motor/buzzer debe vibrar exactamente 200ms y apagarse solo
// Ciclo: 200ms ON → 1000ms OFF → repetir
// ================================================================

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include <string.h>

#define PIN_MOTOR GPIO_NUM_25

void app_main(void) {
    uart_config_t ucfg = {.baud_rate=115200, .data_bits=UART_DATA_8_BITS,
        .parity=UART_PARITY_DISABLE, .stop_bits=UART_STOP_BITS_1,
        .flow_ctrl=UART_HW_FLOWCTRL_DISABLE, .source_clk=UART_SCLK_DEFAULT};
    uart_param_config(UART_NUM_0, &ucfg);
    uart_driver_install(UART_NUM_0, 512, 0, 0, NULL, 0);

    gpio_config_t cfg = {.pin_bit_mask=(1ULL<<PIN_MOTOR),
        .mode=GPIO_MODE_OUTPUT, .pull_up_en=GPIO_PULLUP_DISABLE,
        .pull_down_en=GPIO_PULLDOWN_DISABLE, .intr_type=GPIO_INTR_DISABLE};
    gpio_config(&cfg);
    gpio_set_level(PIN_MOTOR, 0);

    uart_write_bytes(UART_NUM_0, "TC-002: Prueba de motor - ciclo 200ms ON / 1s OFF\n", 48);
    
    while (1) {
        char *on = "Motor ON (200ms)\n";
        uart_write_bytes(UART_NUM_0, on, strlen(on));
        int64_t inicio = esp_timer_get_time();
        gpio_set_level(PIN_MOTOR, 1);

        while ((esp_timer_get_time() - inicio) < 200000);

        gpio_set_level(PIN_MOTOR, 0);
        char *off = "Motor OFF\n";
        uart_write_bytes(UART_NUM_0, off, strlen(off));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}