#ifndef _HISTORIAL_H_
#define _HISTORIAL_H_

#include <stdint.h>

// ================================================================
// CLASE HISTORIAL - Almacenamiento persistente en NVS
// ================================================================
// Responsabilidades:
//   - Guardar contador de impactos en flash (persistente) (RF-05)
//   - Guardar umbral configurado (RF-10)
//   - Guardar logs de eventos (últimos 5 eventos WARNING)
//   - Cargar datos al iniciar el sistema
// ================================================================

class Historial {
public:
    // Constructor
    Historial();
    
    // Destructor
    ~Historial();
    
    // Inicializa el sistema NVS
    void init(void);
    
    // Guarda el contador de impactos en la flash
    void guardarContador(int contador);
    
    // Carga el contador de impactos desde la flash
    int cargarContador(void);
    
    // Guarda el umbral en la flash
    void guardarUmbral(int umbral);
    
    // Carga el umbral desde la flash
    int cargarUmbral(void);
    
    // Guarda un evento de log (solo para WARNING)
    void guardarLog(const char *log_texto);
    
    // Muestra el historial de logs por UART
    void mostrarHistorial(void);
    
    // Borra todos los datos guardados en la flash
    void reiniciarTodo(void);

private:
    static const char* NAMESPACE;      // "smart_insole" -> carpeta donde se guardan los datos. 
    static const char* KEY_UMBRAL;     // "umbral" -> nombre de la variable dentro del namespace (como etiquetas en una carpeta)
    static const char* KEY_IMPACT;     // "impactos"
    static const char* KEY_LOGCOUNT;   // "log_count"
    static const int MAX_LOGS;         // 5 eventos máximos
};

#endif // _HISTORIAL_H_