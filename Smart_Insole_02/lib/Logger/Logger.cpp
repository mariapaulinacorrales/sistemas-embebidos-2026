// ================================================================
// IMPLEMENTACIÓN DE LA CLASE LOGGER
// ================================================================

#include "Logger.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include <stdio.h>
#include <string.h>   

#define UART_PORT UART_NUM_0
#define UART_BAUD 115200

Logger::Logger() {}
Logger::~Logger() {}

// Inicializar UART para logs
void Logger::init(void) {
    uart_config_t uart_cfg = {
        .baud_rate = UART_BAUD,
        .data_bits = UART_DATA_8_BITS, // 8 bits
        .parity = UART_PARITY_DISABLE, // sin bit de paridad
        .stop_bits = UART_STOP_BITS_1, // 1 bit de parada
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, 
        .source_clk = UART_SCLK_DEFAULT 
    };
    uart_param_config(UART_PORT, &uart_cfg);
    uart_driver_install(UART_PORT, 1024, 1024, 20, NULL, 0);
}

// Obtener timestamp en milisegundos
static int64_t _timestamp(void) {
    return esp_timer_get_time() / 1000; // esp_timer_get_time() devuelve el tiempo en microsegundos
}

// Registrar evento INFO
void Logger::info(int valor, const char *mensaje) {
    char linea[128];
    snprintf(linea, sizeof(linea), "[INFO] T=%lldms | FSR=%d | %s\n",
             _timestamp(), valor, mensaje);
    uart_write_bytes(UART_PORT, linea, strlen(linea)); // strlen() es la longitud del mensaje
}

// Registrar evento WARNING (incluye códigos de error)
void Logger::warning(int valor, const char *codigo, const char *mensaje) {
    char linea[128];
    if (codigo) { // Si hay un código de error (ERR_002, VERIFY, NULL (no hay código específico))
        snprintf(linea, sizeof(linea), "[WARNING] T=%lldms | FSR=%d | %s | %s\n",
                 _timestamp(), valor, codigo, mensaje);
    } else {
        snprintf(linea, sizeof(linea), "[WARNING] T=%lldms | FSR=%d | %s\n",
                 _timestamp(), valor, mensaje);
    }
    uart_write_bytes(UART_PORT, linea, strlen(linea));
}

// Registrar evento ERROR (sensor desconectado)
void Logger::error(int valor, const char *codigo, const char *mensaje) {
    char linea[128];
    snprintf(linea, sizeof(linea), "[ERROR] T=%lldms | FSR=%d | %s | %s\n",
             _timestamp(), valor, codigo, mensaje);
    uart_write_bytes(UART_PORT, linea, strlen(linea));
}

// Reporte periódico de estado (cada 1 segundo)
void Logger::reportarEstado(int raw, int filtrado, int mv, int umbral, int impactos, int estado) {
    const char *estado_str;
    switch(estado) {
        case 0: estado_str = "OK"; break;
        case 1: estado_str = "ALERTA"; break;
        case 2: estado_str = "ERROR"; break;
        default: estado_str = "VERIF"; break;
    }
    char rep[128];
    snprintf(rep, sizeof(rep),
        "FSR_raw=%d | FSR_fil=%d | mV=%d | Umbral=%d | Impactos=%d | Estado=%s\n",
        raw, filtrado, mv, umbral, impactos, estado_str);
    uart_write_bytes(UART_PORT, rep, strlen(rep));
}