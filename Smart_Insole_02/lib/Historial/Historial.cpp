// ================================================================
// IMPLEMENTACIÓN DE LA CLASE HISTORIAL
// ================================================================

#include "Historial.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/uart.h"
#include <stdio.h>
#include <string.h>

#define UART_PORT UART_NUM_0

// Definición (asignación de valores) de constantes estáticas
const char* Historial::NAMESPACE = "smart_insole";
const char* Historial::KEY_UMBRAL = "umbral";
const char* Historial::KEY_IMPACT = "impactos";
const char* Historial::KEY_LOGCOUNT = "log_count";
const int Historial::MAX_LOGS = 5;

Historial::Historial() {}
Historial::~Historial() {}

// Inicializar NVS
void Historial::init(void) {
    esp_err_t ret = nvs_flash_init();   // esp_err_t es un tipo de dato que representa un código de error. Si es ESP_OK, todo salió bien. Si es otro valor, algo falló.
                                        // nvs_flash_init(); inicializa la partición de memoria flash
    
    // ESP_ERR_NVS_NO_FREE_PAGES -> La partición NVS está llena o corrupta
    // ESP_ERR_NVS_NEW_VERSION_FOUND -> El formato de NVS es incompatible con el actual
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase(); // Borrar todo
        nvs_flash_init(); // Reiniciar desde cero
    }
}

// Guardar contador de impactos
void Historial::guardarContador(int contador) {
    nvs_handle_t handle;
    if (nvs_open(NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return; // Abrir el namespace con permisos de lectura y escritura
                                                                       // Se quiere leer y escribir
    nvs_set_i32(handle, KEY_IMPACT, (int32_t)contador); // Se guarda un número de 32 bits
    nvs_commit(handle); // Los datos se guardan físicamente, no temporalmente
    nvs_close(handle); // libera recursos (cierra sesión)
}

// Cargar contador de impactos
int Historial::cargarContador(void) {
    nvs_handle_t handle;
    int32_t valor = 0;
    if (nvs_open(NAMESPACE, NVS_READONLY, &handle) == ESP_OK) { // Abrir en modo solo lectura
        nvs_get_i32(handle, KEY_IMPACT, &valor); // se lee el valor y se guarda
        nvs_close(handle);
    }
    return (int)valor; // devolver como int normal
}

// Guardar umbral
void Historial::guardarUmbral(int umbral) {
    nvs_handle_t handle;
    if (nvs_open(NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return;
    nvs_set_i32(handle, KEY_UMBRAL, (int32_t)umbral);
    nvs_commit(handle);
    nvs_close(handle);
}

// Cargar umbral (valor por defecto 500 si no existe)
int Historial::cargarUmbral(void) {
    nvs_handle_t handle;
    int32_t valor = 2000;  // Valor por defecto si no existe
    if (nvs_open(NAMESPACE, NVS_READONLY, &handle) == ESP_OK) {
        nvs_get_i32(handle, KEY_UMBRAL, &valor);
        nvs_close(handle);
    }
    return (int)valor;
}

// Guardar un log en el buffer circular (máximo 5)
void Historial::guardarLog(const char *log_texto) {
    nvs_handle_t handle;
    if (nvs_open(NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return;
    
    // Leer el índice actual de logs (¿cuántos logs hay guardados?)
    int32_t log_count = 0;
    nvs_get_i32(handle, KEY_LOGCOUNT, &log_count);
    
    // Calcular posición circular (0,1,2,3,4,0,1,2,3,4,...) -> un buffer circular es donde SOLO se guardan los últimos 5 elementos. Cuando se llega al 5º, se empieza a sobrescribir desde el principio
    int posicion = log_count % MAX_LOGS; // MAX_LOGS = 5
    char clave[8];
    snprintf(clave, sizeof(clave), "log_%d", posicion); // "Log_0", "Log_1", ...
    
    // Guardar el log
    nvs_set_str(handle, clave, log_texto);
    
    // Incrementar contador
    log_count++;
    nvs_set_i32(handle, KEY_LOGCOUNT, log_count);
    
    nvs_commit(handle);
    nvs_close(handle);
}

// Mostrar historial de logs por UART
void Historial::mostrarHistorial(void) {
    nvs_handle_t handle;
    
    // Abrir el namespace
    if (nvs_open(NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
        uart_write_bytes(UART_PORT, "[NVS] Primera ejecucion - sin datos previos\n", 44);
        return;
    }
    
    // Mostrar contador de impactos
    int32_t impactos = 0;
    if (nvs_get_i32(handle, KEY_IMPACT, &impactos) == ESP_OK) {
        char msg[64];
        snprintf(msg, sizeof(msg), "[NVS] Impactos previos restaurados: %ld\n", impactos);
        uart_write_bytes(UART_PORT, msg, strlen(msg));
    }
    
    // Mostrar umbral
    int32_t umbral = 500;
    if (nvs_get_i32(handle, KEY_UMBRAL, &umbral) == ESP_OK) {
        char msg[64];
        snprintf(msg, sizeof(msg), "[NVS] Umbral restaurado: %ld bits\n", umbral);
        uart_write_bytes(UART_PORT, msg, strlen(msg));
    }
    
    // Mostrar logs guardados
    int32_t log_count = 0;
    if (nvs_get_i32(handle, KEY_LOGCOUNT, &log_count) != ESP_OK || log_count == 0) {
        uart_write_bytes(UART_PORT, "[NVS] Sin eventos previos guardados\n", 36);
        nvs_close(handle);
        return;
    }
    
    // Calcular cuántos Logs mostrar y el más antiguo
    int total = (log_count < MAX_LOGS) ? (int)log_count : MAX_LOGS;
    int inicio = (int)(log_count % MAX_LOGS);
    
    // Encabezado
    char header[64];
    snprintf(header, sizeof(header), "[NVS] === HISTORIAL PREVIO (%d eventos) ===\n", total);
    uart_write_bytes(UART_PORT, header, strlen(header));
    
    // Recorrer todos los Logs en orden cronológico
    for (int i = 0; i < total; i++) {
        int posicion = (inicio + i) % MAX_LOGS;
        char clave[8];
        snprintf(clave, sizeof(clave), "log_%d", posicion);
        
        char log_texto[128];
        size_t tamano = sizeof(log_texto);
        if (nvs_get_str(handle, clave, log_texto, &tamano) == ESP_OK) {
            uart_write_bytes(UART_PORT, log_texto, strlen(log_texto));
        }
    }
    
    uart_write_bytes(UART_PORT, "[NVS] === FIN DEL HISTORIAL ===\n\n", 33);
    nvs_close(handle);
}

// Borrar todos los datos guardados
void Historial::reiniciarTodo(void) {
    nvs_handle_t handle;
    if (nvs_open(NAMESPACE, NVS_READWRITE, &handle) == ESP_OK) {
        nvs_erase_all(handle); // Borra TODAS las claves del namespace -> se usa por comando "RESET_LOGS"
        nvs_commit(handle);
        nvs_close(handle);
    }
}