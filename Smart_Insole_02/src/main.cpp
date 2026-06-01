// ================================================================
// SMART INSOLE v2.0 - MAIN 
// ================================================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/i2c.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"

// Librerías del proyecto (cada una encapsula un subsistema)
#include "SensorFSR.h"
#include "ActuadorBuzzer.h"
#include "PantallaOLED.h"
#include "Logger.h"
#include "Historial.h"

// ================================================================
// PINES Y CONSTANTES DEL SISTEMA
// ================================================================
#define PIN_FSR_CH     6           // ADC_CHANNEL_6 (GPIO34) para el FSR
#define PIN_BUZZER     25          // GPIO25 para el buzzer
#define I2C_SDA_PIN    GPIO_NUM_21 // SDA de la OLED
#define I2C_SCL_PIN    GPIO_NUM_22 // SCL de la OLED

#define UMBRAL_DEFAULT      2100    // Umbral de impacto por defecto (bits ADC)
#define FSR_MIN_VALIDO      50      // Valor mínimo para considerar sensor conectado
#define FSR_MAX_VALIDO      3500     // Valor máximo antes de saturación
#define TIEMPO_DEBOUNCE_MS  500     // Tiempo mínimo entre impactos (ms)
#define DURACION_BUZZER_MS  200     // Duración del pitido del buzzer (ms)

// ================================================================
// ESTADOS DEL SISTEMA
// ================================================================
#define ESTADO_OK        0
#define ESTADO_ALERTA    1
#define ESTADO_ERROR     2
#define ESTADO_VERIFICAR 3

// ================================================================
// VARIABLES GLOBALES DEL SISTEMA
// ================================================================
static int umbral_actual = UMBRAL_DEFAULT;
static int contador_impactos = 0;
static int estado_sistema = ESTADO_OK;
static int64_t ultimo_log_saturacion = 0;

// Variables para el sensor
static int raw_actual = 0; // Valor crudo del ADC
static int filtrado_actual = 0; // Valor filtrado
static int mv_actual = 0; // Milivoltios equivalentes

// Variables de control
static int64_t ultimo_impacto = 0;
static int64_t ultimo_log_sensor = 0;
static int64_t tiempo_inicio_buzzer = 0;
static bool buzzer_activo = false;
static bool ha_tenido_lectura_normal = false;

// ================================================================
// DECLARACIÓN DE FUNCIONES AUXILIARES
// ================================================================
static void inicializarI2C(void);
static void verificarSensor(SensorFSR& sensor, Logger& logger);
static void verificarImpacto(ActuadorBuzzer& buzzer, Logger& logger, Historial& historial);
static void procesarComandosUART(Logger& logger, Historial& historial);

extern "C" void app_main() {

    // ── WATCHDOG ──────────────────────────────────────────────
    const esp_task_wdt_config_t wdt_cfg = {
        .timeout_ms=5000, .idle_core_mask=0, .trigger_panic=true
    };
    esp_task_wdt_init(&wdt_cfg);
    esp_task_wdt_add(NULL);

    // ── 1. INICIALIZAR PERIFÉRICOS COMUNES ───────────────────
    inicializarI2C();

    // ── 2. CREAR TODOS LOS OBJETOS ───────────────────────────
    SensorFSR sensor(PIN_FSR_CH);
    sensor.init();

    ActuadorBuzzer buzzer(PIN_BUZZER);
    buzzer.init();

    PantallaOLED pantalla;
    pantalla.init();
    pantalla.mostrarInicio();

    Logger logger;
    logger.init();

    Historial historial;
    historial.init();

    // ── 3. CARGAR DATOS GUARDADOS ────────────────────────────
    contador_impactos = historial.cargarContador();

    // Cargar umbral con migración desde valores antiguos -> Si se detecta un valor antiguo, se actualiza automáticamente al nuevo (2100)
    int umbral_cargado = historial.cargarUmbral();
    if (umbral_cargado == 500 || umbral_cargado == 1500) {
        umbral_actual = UMBRAL_DEFAULT;  // 2100
        historial.guardarUmbral(umbral_actual);
    } else {
        umbral_actual = umbral_cargado;
    }
    historial.mostrarHistorial();

    // ── 4. MENSAJE DE INICIO ─────────────────────────────────
    logger.info(0, "Sistema iniciado"); // Envía por UART: [INFO] T=0ms | FSR=0 | Sistema iniciado

    // ── 5. VARIABLES DE CONTROL DE TIEMPO ────────────────────
    int64_t ultimo_adc = esp_timer_get_time();
    int64_t ultimo_uart = esp_timer_get_time();
    int64_t ultimo_oled = esp_timer_get_time();

    // ── 6. BUCLE PRINCIPAL
    while (1) {
        esp_task_wdt_reset();
        int64_t ahora = esp_timer_get_time();

        // Leer comandos del usuario por UART
        procesarComandosUART(logger, historial);

        // Verificar buzzer (apagado automático)
        buzzer.verificar();

        // ── LECTURA DEL SENSOR (cada 10ms) ────────────────────
        if ((ahora - ultimo_adc) >= (10 * 1000)) {
            ultimo_adc = ahora;

            raw_actual = sensor.leerRaw();
            filtrado_actual = sensor.leerFiltrado();
            mv_actual = sensor.leerMilivoltios();

            verificarSensor(sensor, logger); // ¿Sensor conectado? ¿Saturado?
            verificarImpacto(buzzer, logger, historial); // ¿Hay impacto?
        }

        // ── REPORTE UART (cada 1 segundo) ─────────────────────
        if ((ahora - ultimo_uart) >= (1000 * 1000)) {
            ultimo_uart = ahora;
            logger.reportarEstado(raw_actual, filtrado_actual, mv_actual,
                                  umbral_actual, contador_impactos, estado_sistema);
        }

        // ── ACTUALIZAR PANTALLA (cada 300ms) ──────────────────
        if ((ahora - ultimo_oled) >= (300 * 1000)) {
            ultimo_oled = ahora;
            // Limitar el valor para la pantalla
            int presion_limitada = filtrado_actual;
            if (presion_limitada > 4095) presion_limitada = 4095;
            if (presion_limitada < 0) presion_limitada = 0;
            pantalla.actualizar(presion_limitada, estado_sistema, contador_impactos);
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// ================================================================
// IMPLEMENTACIÓN DE FUNCIONES AUXILIARES
// ================================================================

static void inicializarI2C(void) {
    i2c_config_t i2c_conf = {};
    i2c_conf.mode = I2C_MODE_MASTER;
    i2c_conf.sda_io_num = I2C_SDA_PIN;
    i2c_conf.scl_io_num = I2C_SCL_PIN;
    i2c_conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_conf.master.clk_speed = 400000;
    i2c_param_config(I2C_NUM_0, &i2c_conf);
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}

static void verificarSensor(SensorFSR& sensor, Logger& logger) {
    static int64_t ultimo_log_saturacion = 0;
    int64_t ahora = esp_timer_get_time() / 1000;
    
    if (raw_actual < FSR_MIN_VALIDO) {
        if (ha_tenido_lectura_normal) { // Se sabe o verifico que funciona correctamente
            estado_sistema = ESTADO_VERIFICAR;
            if ((ahora - ultimo_log_sensor) >= 1000) {
                ultimo_log_sensor = ahora;
                logger.warning(raw_actual, "VERIFY", "Sensor sin presionar - verificar conexion");
            }
        }
        // Si no ha tenido lectura normal, no cambia el estado (sigue OK)
    } else if (raw_actual > FSR_MAX_VALIDO) { // ¿Sensor saturado? (valor > 3500)
        estado_sistema = ESTADO_ALERTA;
        if ((ahora - ultimo_log_saturacion) >= 500) {
            ultimo_log_saturacion = ahora;
            logger.warning(raw_actual, "ERR_002", "Saturacion del sensor FSR");
        }
    } else {

        if (raw_actual > 100) { // Hay presión real
        ha_tenido_lectura_normal = true; // El sensor funciona
        }

    }
}


static void verificarImpacto(ActuadorBuzzer& buzzer, Logger& logger, Historial& historial) {
    // Solo detectar impacto si el sensor está en rango normal
    if (filtrado_actual > umbral_actual) {
        
        int64_t ahora = esp_timer_get_time() / 1000;
        if ((ahora - ultimo_impacto) > TIEMPO_DEBOUNCE_MS) {
            estado_sistema = ESTADO_ALERTA;
            contador_impactos++;
            ultimo_impacto = ahora;
            buzzer.activar(); // encender el buzzer
            logger.warning(filtrado_actual, NULL, "Impacto excesivo"); // Log
            historial.guardarContador(contador_impactos); // Guardar en flash
        }
    } else { // No hay impacto
        if (raw_actual >= FSR_MIN_VALIDO && raw_actual <= FSR_MAX_VALIDO) {
            estado_sistema = ESTADO_OK;
        }
    }
}

static void procesarComandosUART(Logger& logger, Historial& historial) {
    static char uart_buf[256];
    static int uart_idx = 0; // Índice actual en el buffer
    uint8_t byte;

    // Intenta leer 1 byte
    int len = uart_read_bytes(UART_NUM_0, &byte, 1, pdMS_TO_TICKS(5));
    if (len <= 0) return; // No hay datos

    // Si es fin de línea o buffer lleno, procesar
    if (byte == '\n' || byte == '\r' || uart_idx >= (int)sizeof(uart_buf) - 1) {
        if (uart_idx > 0) {
            uart_buf[uart_idx] = '\0'; // Terminar string

            // Busca comandos
            char *ptr = strstr(uart_buf, "SET_THRESHOLD:"); // Se verifica que se haya escrito el comando
            if (ptr != NULL) {
                int nuevo = atoi(ptr + 14); // se "salta" SET_THRESHOLD
                if (nuevo >= FSR_MIN_VALIDO && nuevo <= FSR_MAX_VALIDO) {
                    umbral_actual = nuevo;
                    historial.guardarUmbral(umbral_actual);
                    char msg[64];
                    snprintf(msg, sizeof(msg), "OK: Umbral = %d bits\n", umbral_actual);
                    uart_write_bytes(UART_NUM_0, msg, strlen(msg));
                    logger.info(0, "Umbral actualizado por UART");
                } else {
                    uart_write_bytes(UART_NUM_0, "Error: rango 50-3500\n", 20);
                }
            }
            else if (strcasecmp(uart_buf, "STATUS") == 0) { // Responder con estado actual
                char st[128]; 
                const char *estado_str;
                switch(estado_sistema) {
                    case ESTADO_OK:        estado_str = "OK"; break;
                    case ESTADO_ALERTA:    estado_str = "ALERTA"; break;
                    case ESTADO_ERROR:     estado_str = "ERROR"; break;
                    case ESTADO_VERIFICAR: estado_str = "VERIF"; break;
                    default:               estado_str = "?"; break;
                }
                snprintf(st, sizeof(st), "UMBRAL=%d | Impactos=%d | Estado=%s\n",
                         umbral_actual, contador_impactos, estado_str);
                uart_write_bytes(UART_NUM_0, st, strlen(st));
            }
            else if (strcasecmp(uart_buf, "RESET_LOGS") == 0) {
                historial.reiniciarTodo();
                contador_impactos = 0;
                uart_write_bytes(UART_NUM_0, "OK: Historial borrado. Contador reiniciado a 0.\n", 48);
                logger.info(0, "Historial NVS borrado por comando UART");
            }
            else if (strlen(uart_buf) > 0) { // Comando no reconocido
                uart_write_bytes(UART_NUM_0, "Comandos: SET_THRESHOLD:XXX | STATUS | RESET_LOGS\n", 50);
            }
        }
        uart_idx = 0; //  Reiniciar buffer
    } else {
        uart_buf[uart_idx++] = (char)byte; // Acumular carácter
    }
}