// ================================================================
// TC-006: Prueba de comandos UART
// Cubre RF-10
// ================================================================
// Comandos disponibles:
//   SET_THRESHOLD:XXX  (ej: SET_THRESHOLD:500)
//   STATUS
// ================================================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

#define UART_PORT UART_NUM_0
#define UART_BAUD 115200
#define BUF_SIZE 256          // ← Aumentado de 64 a 256
#define RX_BUF_SIZE 1024      // ← Buffer de recepción más grande
#define TX_BUF_SIZE 1024      // ← Buffer de transmisión más grande

#define UMBRAL_DEFAULT 700
#define UMBRAL_MIN 50
#define UMBRAL_MAX 950

int umbral_actual = UMBRAL_DEFAULT;
char uart_buf[BUF_SIZE];
int uart_idx = 0;

void procesar_comando(char *cmd) {
    // Eliminar caracteres de nueva línea al final
    size_t len = strlen(cmd);
    while (len > 0 && (cmd[len-1] == '\n' || cmd[len-1] == '\r')) {
        cmd[len-1] = '\0';
        len--;
    }
    
    char *ptr = strstr(cmd, "SET_THRESHOLD:");
    if (ptr != NULL) {
        int nuevo = atoi(ptr + 14);
        if (nuevo >= UMBRAL_MIN && nuevo <= UMBRAL_MAX) {
            umbral_actual = nuevo;
            char msg[64];
            snprintf(msg, sizeof(msg), "OK: Umbral = %d\n", umbral_actual);
            uart_write_bytes(UART_PORT, msg, strlen(msg));
        } else {
            uart_write_bytes(UART_PORT, "Error: rango 50-950\n", 20);
        }
    } else if (strncasecmp(cmd, "STATUS", 6) == 0) {  // case insensitive
        char st[64];
        snprintf(st, sizeof(st), "UMBRAL_ACTUAL=%d\n", umbral_actual);
        uart_write_bytes(UART_PORT, st, strlen(st));
    } else if (strlen(cmd) > 0) {
        uart_write_bytes(UART_PORT, "Comandos: SET_THRESHOLD:XXX | STATUS\n", 37);
    }
}

void revisar_uart(void) {
    uint8_t byte;
    // Usar uart_read_bytes con timeout mayor
    int len = uart_read_bytes(UART_PORT, &byte, 1, pdMS_TO_TICKS(5));
    if (len <= 0) return;
    
    if (byte == '\n' || byte == '\r' || uart_idx >= BUF_SIZE - 1) {
        if (uart_idx > 0) {
            uart_buf[uart_idx] = '\0';
            procesar_comando(uart_buf);
        }
        uart_idx = 0;
    } else {
        uart_buf[uart_idx++] = (char)byte;
    }
}

void app_main(void) {

    uart_config_t uart_cfg = {
        .baud_rate = UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };
    
    // Configurar el puerto UART
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_cfg));
    
    // Instalar driver con buffers más grandes
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, RX_BUF_SIZE, TX_BUF_SIZE, 20, NULL, 0));
    
    // ── Configurar pines UART
    
    // Mensaje de inicio
    char msg[128];
    snprintf(msg, sizeof(msg), "\nTC-006: Prueba de comandos UART\n");
    uart_write_bytes(UART_PORT, msg, strlen(msg));
    uart_write_bytes(UART_PORT, "Comandos: SET_THRESHOLD:XXX | STATUS\n", 37);
    
    snprintf(msg, sizeof(msg), "Umbral actual: %d\n", umbral_actual);
    uart_write_bytes(UART_PORT, msg, strlen(msg));
    uart_write_bytes(UART_PORT, "\nEsperando comandos...\n", 22);

    while (1) {
        revisar_uart();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}