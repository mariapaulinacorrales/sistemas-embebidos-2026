#ifndef _PANTALLA_OLED_H_
#define _PANTALLA_OLED_H_

#include <stdint.h>
#include <stddef.h>

// ================================================================
// CLASE PANTALLA_OLED - Pantalla SSD1306 por I2C
// ================================================================
// Muestra en tiempo real (RF-08):
//   - Presión actual (bits y porcentaje)
//   - Estado del sistema (OK/ALERTA/ERROR/VERIF)
//   - Contador de impactos
// ================================================================

class PantallaOLED {
public:
    // Constructor
    PantallaOLED();
    
    // Destructor
    ~PantallaOLED();
    
    // Inicializa la pantalla (secuencia de comandos I2C)
    void init(void);
    
    // Limpia toda la pantalla
    void limpiar(void);
    
    // Muestra la pantalla de inicio "SMART INSOLE V2.0 INICIANDO..."
    void mostrarInicio(void);
    
    // Actualiza la pantalla con los valores actuales
    void actualizar(int presion, int estado, int impactos);
    
    // Posiciona el cursor (fila 0-7, columna 0-127)
    void setCursor(uint8_t pagina, uint8_t columna);

private:
    // Envía un comando al OLED por I2C
    void _enviarComando(uint8_t cmd);
    
    // Envía datos de píxeles al OLED por I2C
    void _enviarDatos(const uint8_t *data, size_t len);
    
    // Escribe un carácter en la posición actual
    void _escribirChar(char c);
    
    // Escribe una cadena de texto
    void _escribirString(const char *s);
    
};

#endif // _PANTALLA_OLED_H_