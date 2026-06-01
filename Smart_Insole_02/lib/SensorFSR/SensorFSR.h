#ifndef _SENSOR_FSR_H_
#define _SENSOR_FSR_H_

#include <stdint.h>
#include <stdbool.h>

// ================================================================
// CLASE SENSOR_FSR - Sensor de fuerza plantar
// ================================================================
// Responsabilidades:
//   - Leer el valor del ADC (GPIO34 = ADC_CHANNEL_6)
//   - Aplicar filtro de media móvil de 5 muestras (RF-02)
//   - Detectar desconexión del sensor (RF-09)
//   - Convertir a milivoltios
// ================================================================

class SensorFSR {
public:
    // Constructor: recibe el canal ADC (ADC_CHANNEL_6 = 6)
    SensorFSR(uint8_t adc_channel);
    
    // Destructor
    ~SensorFSR();
    
    // Inicializa el ADC y la calibración
    void init(void);
    
    // Lee el valor crudo del ADC (0-4095)
    int leerRaw(void);
    
    // Lee el valor filtrado (media móvil de 5 muestras) → RF-02
    int leerFiltrado(void);
    
    // Convierte el valor filtrado a milivoltios
    int leerMilivoltios(void);
    
    // Verifica si el sensor está conectado (Raw > 50) → RF-09
    bool estaConectado(void);
    
    // Reinicia el buffer del filtro
    void reiniciarFiltro(void);

private:
    uint8_t _canal;             // Canal ADC (ej: ADC_CHANNEL_6)
    int _buffer[5];             // Buffer circular para filtro media móvil
    int _indice;                // Índice actual en el buffer
    int _raw;                   // Último valor crudo leído
    int _filtrado;              // Último valor filtrado
    int _milivoltios;           // Último valor en milivoltios
    
    // Aplica el filtro de media móvil internamente
    void _aplicarFiltro(void);
};

#endif // _SENSOR_FSR_H_