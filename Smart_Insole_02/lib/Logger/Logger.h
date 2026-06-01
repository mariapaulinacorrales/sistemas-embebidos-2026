#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <stdint.h>

// ================================================================
// CLASE LOGGER - Registro de eventos
// ================================================================
// Responsabilidades:
//   - Registrar eventos con timestamp (RF-06)
//   - Enviar logs por UART (RF-07)
//   - Diferenciar entre INFO, WARNING y ERROR
//   - Guardar eventos WARNING en el historial (NVS)
// ================================================================

class Logger {
public:
    // Constructor
    Logger();
    
    // Destructor
    ~Logger();
    
    // Inicializa el sistema de logging (UART)
    void init(void);
    
    // Registra un evento INFO (presión normal)
    void info(int valor, const char *mensaje);
    
    // Registra un evento WARNING (impacto o saturación)
    void warning(int valor, const char *codigo, const char *mensaje);
    
    // Registra un evento ERROR (sensor desconectado)
    void error(int valor, const char *codigo, const char *mensaje);
    
    // Envía un reporte periódico del estado (cada 1 segundo)
    void reportarEstado(int raw, int filtrado, int mv, int umbral, int impactos, int estado);
};

#endif // _LOGGER_H_