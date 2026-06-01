// ================================================================
// IMPLEMENTACIÓN DE LA CLASE SENSOR_FSR
// ================================================================

#include "SensorFSR.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

// Handles estáticos del ADC (compartidos entre todas las instancias)
static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle;
static bool calibrado = false;
static bool adc_inicializado = false;

#define FILTRO_MUESTRAS 5   // Número de muestras para el filtro (RF-02)

// Constructor: inicializa el buffer de filtro
SensorFSR::SensorFSR(uint8_t adc_channel) 
    : _canal(adc_channel), _indice(0), _raw(0), _filtrado(0), _milivoltios(0) {
    for (int i = 0; i < FILTRO_MUESTRAS; i++) _buffer[i] = 0;
}

SensorFSR::~SensorFSR() {}

// Inicializa el ADC y la calibración (solo una vez)
void SensorFSR::init(void) {
    if (!adc_inicializado) {
        // Inicializar ADC unitario
        adc_oneshot_unit_init_cfg_t adc_init = {};
        adc_init.unit_id = ADC_UNIT_1;
        adc_oneshot_new_unit(&adc_init, &adc_handle);
        
        // Configurar atenuación y resolución
        adc_oneshot_chan_cfg_t adc_cfg = {};
        adc_cfg.bitwidth = ADC_BITWIDTH_12;
        adc_cfg.atten = ADC_ATTEN_DB_11;  // Rango 0-3.6V
        adc_oneshot_config_channel(adc_handle, (adc_channel_t)_canal, &adc_cfg);
        
        // Calibración del ADC
        adc_cali_line_fitting_config_t cali_cfg = {};
        cali_cfg.unit_id = ADC_UNIT_1;
        cali_cfg.atten = ADC_ATTEN_DB_11;
        cali_cfg.bitwidth = ADC_BITWIDTH_12;
        if (adc_cali_create_scheme_line_fitting(&cali_cfg, &cali_handle) == ESP_OK) {
            calibrado = true;
        }
        adc_inicializado = true;
    }
}

// Lee el valor crudo del ADC
int SensorFSR::leerRaw(void) {
    adc_oneshot_read(adc_handle, (adc_channel_t)_canal, &_raw);
    return _raw;
}

// Aplica el filtro de media móvil (RF-02)
void SensorFSR::_aplicarFiltro(void) {
    _buffer[_indice] = _raw; // Guarda la nueva muestra
    _indice = (_indice + 1) % FILTRO_MUESTRAS; // Avanza el índice
    int suma = 0;
    for (int i = 0; i < FILTRO_MUESTRAS; i++) suma += _buffer[i]; // suma las 5
    _filtrado = suma / FILTRO_MUESTRAS; // Promedio
}

// Lee el valor filtrado
int SensorFSR::leerFiltrado(void) {
    leerRaw();
    _aplicarFiltro();
    return _filtrado;
}

// Convierte a milivoltios usando la calibración
int SensorFSR::leerMilivoltios(void) {
    if (calibrado) {
        adc_cali_raw_to_voltage(cali_handle, _filtrado, &_milivoltios);
    }
    return _milivoltios;
}

// Verifica si el sensor está conectado (RF-09)
bool SensorFSR::estaConectado(void) {
    leerRaw();
    return (_raw > 50);  // Si Raw > 50, hay sensor conectado
}

// Reinicia el buffer del filtro
void SensorFSR::reiniciarFiltro(void) {
    _indice = 0;
    for (int i = 0; i < FILTRO_MUESTRAS; i++) _buffer[i] = 0;
}