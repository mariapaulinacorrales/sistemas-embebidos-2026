// ================================================================
// TC-005: Verificar watchdog (RNF-02)
// ================================================================
// El sistema debe reiniciarse automáticamente si el loop se cuelga > 5s
// ================================================================

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_task_wdt.h"

void app_main(void) {
    uart_config_t ucfg = {.baud_rate=115200,.data_bits=UART_DATA_8_BITS,
        .parity=UART_PARITY_DISABLE,.stop_bits=UART_STOP_BITS_1,
        .flow_ctrl=UART_HW_FLOWCTRL_DISABLE,.source_clk=UART_SCLK_DEFAULT};
    uart_param_config(UART_NUM_0,&ucfg);
    uart_driver_install(UART_NUM_0,512,0,0,NULL,0);

    const esp_task_wdt_config_t wdt_config = {
        .timeout_ms = 5000,      // 5 segundos de timeout
        .idle_core_mask = 0,     // Monitorear todos los núcleos
        .trigger_panic = true    
    };
    
    esp_task_wdt_init(&wdt_config);
    esp_task_wdt_add(NULL);      // Añadir la tarea actual

    char *msg = "TC-005: Sistema iniciado. El loop se bloqueara en 2s...\n";
    uart_write_bytes(UART_NUM_0, msg, strlen(msg));
    vTaskDelay(pdMS_TO_TICKS(2000));

    char *blk = "BLOQUEANDO el loop (sin resetear el watchdog)...\n";
    uart_write_bytes(UART_NUM_0, blk, strlen(blk));

    while (1) {
        // No llamamos esp_task_wdt_reset() → watchdog debe reiniciar
    }
}