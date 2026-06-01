// ================================================================
// IMPLEMENTACIÓN DE LA CLASE ACTUADOR_BUZZER
// ================================================================

#include "ActuadorBuzzer.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define DURACION_MS 200   // Duración del pitido en ms (RF-04)

// Constructor: guarda el pin GPIO
ActuadorBuzzer::ActuadorBuzzer(uint8_t pin) : _pin(pin), _activo(false), _tiempo_inicio(0) {}

// Destructor: apaga el buzzer
ActuadorBuzzer::~ActuadorBuzzer() {
    gpio_set_level((gpio_num_t)_pin, 0);
}

// Inicializa el pin GPIO como salida y lo deja en bajo (apagado)
void ActuadorBuzzer::init(void) {
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = (1ULL << _pin);
    cfg.mode = GPIO_MODE_OUTPUT;
    cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&cfg);
    gpio_set_level((gpio_num_t)_pin, 0);
}

// Activa el buzzer (enciende el GPIO)
void ActuadorBuzzer::activar(void) {
    if (!_activo) {
        gpio_set_level((gpio_num_t)_pin, 1);
        _activo = true;
        _tiempo_inicio = esp_timer_get_time();  // Guardar timestamp
    }
}

// Verifica si ya pasaron 200ms y apaga el buzzer
void ActuadorBuzzer::verificar(void) {
    if (_activo) {
        int64_t transcurrido_ms = (esp_timer_get_time() - _tiempo_inicio) / 1000;
        if (transcurrido_ms >= DURACION_MS) {
            gpio_set_level((gpio_num_t)_pin, 0);
            _activo = false;
        }
    }
}