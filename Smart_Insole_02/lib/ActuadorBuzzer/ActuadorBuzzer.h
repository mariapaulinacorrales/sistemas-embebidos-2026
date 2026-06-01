#ifndef _ACTUADOR_BUZZER_H_
#define _ACTUADOR_BUZZER_H_

#include <stdint.h>
#include <stdbool.h>

// ================================================================
// CLASE ACTUADOR_BUZZER - Control del actuador háptico
// ================================================================
// Reemplaza el motor de vibración original. Usa un buzzer pasivo
// que se activa durante 200ms al detectar un impacto (RF-04).
// ================================================================

class ActuadorBuzzer {
public:
    // Constructor: recibe el pin GPIO donde está conectado el buzzer (GPIO25)
    ActuadorBuzzer(uint8_t pin);
    
    // Destructor: apaga el buzzer y libera el pin
    ~ActuadorBuzzer();
    
    // Inicializa el pin GPIO como salida y apaga el buzzer
    void init(void);
    
    // Activa el buzzer por 200ms (misma lógica que activar_motor original)
    void activar(void);
    
    // Verifica si ya pasaron los 200ms y apaga el buzzer
    // (debe llamarse en el loop principal, como verificar_motor original)
    void verificar(void);

private:
    uint8_t _pin;               // Pin GPIO del buzzer
    bool _activo;               // Estado: true = sonando, false = apagado
    int64_t _tiempo_inicio;     // Timestamp de cuando se activó (en microsegundos)
};

#endif // _ACTUADOR_BUZZER_H_