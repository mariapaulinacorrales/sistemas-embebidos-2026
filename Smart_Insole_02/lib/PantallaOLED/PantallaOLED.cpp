// ================================================================
// IMPLEMENTACIÓN DE LA CLASE PANTALLA_OLED
// ================================================================

#include "PantallaOLED.h"
#include "driver/i2c.h"
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Configuración I2C (igual que el original)
#define I2C_PORT I2C_NUM_0 // Usamos el puerto I2C número 0
#define OLED_ADDR 0x3C // Dirección de la OLED: 0 1 1 1 1 0 0 
#define OLED_WIDTH 128 // Ancho en píxeles
#define OLED_PAGES 8 // 8 páginas x 8 píxeles = 64 píxeles de alto

// Comandos del SSD1306 (igual que el original)
#define OLED_CMD_DISPLAY_OFF        0xAE // Apagar pantalla
#define OLED_CMD_DISPLAY_ON         0xAF // Encender pantalla
#define OLED_CMD_SET_CONTRAST       0x81 
#define OLED_CMD_CHARGE_PUMP        0x8D // Activar bomba de carga (necesaria para 3.3V) 
#define OLED_CMD_CHARGE_PUMP_ON     0x14 // Valor para encender la bomba
#define OLED_CMD_MEM_MODE           0x20 // Modo de direccionamiento de memoria
#define OLED_CMD_MEM_HORIZONTAL     0x00 // Modo horizontal (automático)
#define OLED_CMD_SET_PAGE_ADDR      0xB0 // Base para seleccionar página (0xB0 + página)
#define OLED_CMD_SET_COL_LOW        0x00 // Bits bajos de la columna (0-15)
#define OLED_CMD_SET_COL_HIGH       0x10 // Bits altos de la columna (0-1)
#define OLED_CMD_SEG_REMAP          0xA1
#define OLED_CMD_COM_SCAN_DEC       0xC8
#define OLED_CMD_SET_DISPLAY_OFFSET 0xD3
#define OLED_CMD_SET_START_LINE     0x40
#define OLED_CMD_SET_MUX_RATIO      0xA8
#define OLED_CMD_SET_CLK_DIV        0xD5
#define OLED_CMD_SET_PRECHARGE      0xD9
#define OLED_CMD_SET_COM_PINS       0xDA
#define OLED_CMD_SET_VCOMH          0xDB
#define OLED_CMD_DISPLAY_RAM        0xA4
#define OLED_CMD_NORMAL_DISPLAY     0xA6

// Fuente 5x7
// Es como un diccionario
// Cada letra ocupa 5 columnas de ancho × 7 filas de alto
static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // Espacio
    {0x00,0x00,0x5F,0x00,0x00}, // !
    {0x00,0x07,0x00,0x07,0x00}, // "
    {0x14,0x7F,0x14,0x7F,0x14}, // #
    {0x24,0x2A,0x7F,0x2A,0x12}, // $
    {0x23,0x13,0x08,0x64,0x62}, // %
    {0x36,0x49,0x55,0x22,0x50}, // &
    {0x00,0x05,0x03,0x00,0x00}, // '
    {0x00,0x1C,0x22,0x41,0x00}, // (
    {0x00,0x41,0x22,0x1C,0x00}, // )
    {0x14,0x08,0x3E,0x08,0x14}, // *
    {0x08,0x08,0x3E,0x08,0x08}, // +
    {0x00,0x50,0x30,0x00,0x00}, // ,
    {0x08,0x08,0x08,0x08,0x08}, // -
    {0x00,0x60,0x60,0x00,0x00}, // .
    {0x20,0x10,0x08,0x04,0x02}, // /
    {0x3E,0x51,0x49,0x45,0x3E}, // 0
    {0x00,0x42,0x7F,0x40,0x00}, // 1
    {0x42,0x61,0x51,0x49,0x46}, // 2
    {0x21,0x41,0x45,0x4B,0x31}, // 3
    {0x18,0x14,0x12,0x7F,0x10}, // 4
    {0x27,0x45,0x45,0x45,0x39}, // 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 6
    {0x01,0x71,0x09,0x05,0x03}, // 7
    {0x36,0x49,0x49,0x49,0x36}, // 8
    {0x06,0x49,0x49,0x29,0x1E}, // 9
    {0x00,0x36,0x36,0x00,0x00}, // :
    {0x00,0x56,0x36,0x00,0x00}, // ;
    {0x08,0x14,0x22,0x41,0x00}, // <
    {0x14,0x14,0x14,0x14,0x14}, // =
    {0x00,0x41,0x22,0x14,0x08}, // >
    {0x02,0x01,0x51,0x09,0x06}, // ?
    {0x32,0x49,0x79,0x41,0x3E}, // @
    {0x7E,0x11,0x11,0x11,0x7E}, // A
    {0x7F,0x49,0x49,0x49,0x36}, // B
    {0x3E,0x41,0x41,0x41,0x22}, // C
    {0x7F,0x41,0x41,0x22,0x1C}, // D
    {0x7F,0x49,0x49,0x49,0x41}, // E
    {0x7F,0x09,0x09,0x09,0x01}, // F
    {0x3E,0x41,0x49,0x49,0x7A}, // G
    {0x7F,0x08,0x08,0x08,0x7F}, // H
    {0x00,0x41,0x7F,0x41,0x00}, // I
    {0x20,0x40,0x41,0x3F,0x01}, // J
    {0x7F,0x08,0x14,0x22,0x41}, // K
    {0x7F,0x40,0x40,0x40,0x40}, // L
    {0x7F,0x02,0x0C,0x02,0x7F}, // M
    {0x7F,0x04,0x08,0x10,0x7F}, // N
    {0x3E,0x41,0x41,0x41,0x3E}, // O
    {0x7F,0x09,0x09,0x09,0x06}, // P
    {0x3E,0x41,0x51,0x21,0x5E}, // Q
    {0x7F,0x09,0x19,0x29,0x46}, // R
    {0x46,0x49,0x49,0x49,0x31}, // S
    {0x01,0x01,0x7F,0x01,0x01}, // T
    {0x3F,0x40,0x40,0x40,0x3F}, // U
    {0x1F,0x20,0x40,0x20,0x1F}, // V
    {0x3F,0x40,0x38,0x40,0x3F}, // W
    {0x63,0x14,0x08,0x14,0x63}, // X
    {0x07,0x08,0x70,0x08,0x07}, // Y
    {0x61,0x51,0x49,0x45,0x43}, // Z
};

PantallaOLED::PantallaOLED() {}
PantallaOLED::~PantallaOLED() {}

// Enviar comando
void PantallaOLED::_enviarComando(uint8_t cmd) {
    i2c_cmd_handle_t handle = i2c_cmd_link_create(); // Crea una lista de instrucciones I2C
    
    // 1. Genera condición START (SDA baja mientras SCL está HIGH)
    i2c_master_start(handle); 

    // 2. Slave Address (0x3C) + R/W# = 0 (escritura) → 0x78
    //    (0x3C << 1) = 0x78, luego | I2C_MASTER_WRITE (0) = 0x78
    i2c_master_write_byte(handle, (OLED_ADDR << 1) | I2C_MASTER_WRITE, true); // 2. Dirección + escritura
    
    // 3. Control Byte: 0x00 significa "lo que sigue es un COMANDO"
    i2c_master_write_byte(handle, 0x00, true);

    // 4. El comando real (ej: 0xAE para apagar)
    i2c_master_write_byte(handle, cmd, true);

    // 5. STOP condition (SDA sube mientras SCL está HIGH)
    i2c_master_stop(handle); 

    // Se encarga automáticamente de verificar los ACKs
    i2c_master_cmd_begin(I2C_PORT, handle, pdMS_TO_TICKS(100)); // Ejecuta la transacción
    i2c_cmd_link_delete(handle); // Libera la memoria 
}

// Enviar datos de píxeles (el contenido de la pantalla)
void PantallaOLED::_enviarDatos(const uint8_t *data, size_t len) {
    i2c_cmd_handle_t handle = i2c_cmd_link_create();
    i2c_master_start(handle); // SDA pasa a LOW y SCL en HIGH
    i2c_master_write_byte(handle, (OLED_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(handle, 0x40, true); // Para datos de píxeles (D/C#=1)
    i2c_master_write(handle, (uint8_t *)data, len, true); // Envía todos los bytes de una vez
    i2c_master_stop(handle); // SDA pasa a HIGH y SCL en HIGH
    i2c_master_cmd_begin(I2C_PORT, handle, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(handle);
}

// Inicialización - IDÉNTICA al original (sin delays extra)
void PantallaOLED::init(void) {
    _enviarComando(OLED_CMD_DISPLAY_OFF); // Apagar mientras configuramos
    _enviarComando(OLED_CMD_SET_CLK_DIV); // Configurar Reloj (Opcional) - Set Clock Div
    _enviarComando(0x80); // Oscillator frequency + divide ratio
    _enviarComando(OLED_CMD_SET_MUX_RATIO);  // Configurar multiplexado - Set Multiplex Ratio
    _enviarComando(0x3F); // 64 líneas
    _enviarComando(OLED_CMD_SET_DISPLAY_OFFSET); // Sin OFFSET (OPCIONAL)
    _enviarComando(0x00); // No offset
    _enviarComando(OLED_CMD_SET_START_LINE | 0x00); // Línea de inicio en línea 0
    _enviarComando(OLED_CMD_CHARGE_PUMP); // Charge Pump    
    _enviarComando(OLED_CMD_CHARGE_PUMP_ON); // Enable para charge pump ON
    _enviarComando(OLED_CMD_SEG_REMAP); // OPCIONAL: Cambia orientación horizontal
    _enviarComando(OLED_CMD_COM_SCAN_DEC); // OPCIONAL: Cambia orientación vertical
    _enviarComando(OLED_CMD_SET_COM_PINS); // Configurar pines COM
    _enviarComando(0x12); // Configuración de pines ALTERNATIVA
    _enviarComando(OLED_CMD_SET_CONTRAST); // OPCIONAL
    _enviarComando(0xCF); // Contraste máximo
    _enviarComando(OLED_CMD_SET_PRECHARGE); // OPCIONAL
    _enviarComando(0xF1);
    _enviarComando(OLED_CMD_SET_VCOMH); // OPCIONAL VCOMH      
    _enviarComando(0x40); 
    _enviarComando(OLED_CMD_DISPLAY_RAM); // Display RAM -> La pantalla muestra el contenido REAL de la RAM (vemos lo que escribimos)
    _enviarComando(OLED_CMD_NORMAL_DISPLAY); // Display normal, NO INVERTIDO
    _enviarComando(OLED_CMD_MEM_MODE); // SET MEMORY ADDRESSING MODE -> Al escribir un byte, la columna avanza sola. Al llegar a la última columna, pasa a la siguiente página.
    _enviarComando(OLED_CMD_MEM_HORIZONTAL); // HORIZONTAL ADDRESSING MODE
    _enviarComando(OLED_CMD_DISPLAY_ON); // Display ON
}

// Limpiar pantalla 
void PantallaOLED::limpiar(void) {
    uint8_t zeros[OLED_WIDTH]; // Fila de 128 bytes
    memset(zeros, 0x00, sizeof(zeros)); // se llenan todos los píxeles con CEROS (apagado)
    for (uint8_t p = 0; p < OLED_PAGES; p++) {  // Recorre las 8 páginas (de 0 a 7)
        _enviarComando(OLED_CMD_SET_PAGE_ADDR | p); // Selecciona la página actual
        _enviarComando(OLED_CMD_SET_COL_LOW  | 0x00); // Columna baja = 0 (empieza desde la columna 0) -> 4 bits menos significativos
        _enviarComando(OLED_CMD_SET_COL_HIGH | 0x00); // Columna alta = 0 (empieza desde la columna 0) -> 4 bits más significativos (solo se usan 3 bits)
        _enviarDatos(zeros, OLED_WIDTH); // Evía 128 bytes de ceros 
    }
}

// Posicionar cursor
void PantallaOLED::setCursor(uint8_t pagina, uint8_t columna) {
    _enviarComando(OLED_CMD_SET_PAGE_ADDR | (pagina & 0x07)); // pagina & 0x07 aseguran que la pagina esté entre 0 y 7. Si se pasa, lo corrige automáticamente.
    _enviarComando(OLED_CMD_SET_COL_LOW  | (columna & 0x0F)); // columna & 0x0F extrae los 4 bits menos significativos de la columna
    _enviarComando(OLED_CMD_SET_COL_HIGH | ((columna >> 4) & 0x0F)); // (columna >> 4) desplaza los bits 4 posiciones a la derecha y extrae los 4 bits más significativos 
}

// Escribir carácter 
void PantallaOLED::_escribirChar(char c) {
    if (c >= 'a' && c <= 'z') c -= 32; // convierte minúscula a mayúscula
                                       // En ASCII: 'a' = 97 decimal; 'A' = 65 decimal -> la diferencia es 32
    if (c < 32 || c > 90) c = ' '; // si no es válido, muestra espacio
    int idx = c - 32; // calcula índice en la tabla de fuentes
    _enviarDatos(font5x7[idx], 5); // envía los 5 bytes del carácter
    uint8_t sp = 0x00; // crea un byte de espacio
    _enviarDatos(&sp, 1); // envía 1 byte de separación
}

// Escribir string 
void PantallaOLED::_escribirString(const char *texto) {
    while (*texto) { // Mientras no se llegue al final del texto 
        _escribirChar(*texto++); // Escribe el carácter actual y avanza al siguiente
    }
}

// Mostrar inicio 
void PantallaOLED::mostrarInicio(void) {
    limpiar(); // Borra toda la pantalla
    setCursor(1, 10); _escribirString("SMART INSOLE"); // Fila 1 (página), Col 10
    setCursor(3, 20); _escribirString("V2.0"); // Fila 3 (página), Col 20
    setCursor(5, 5);  _escribirString("INICIANDO..."); // Fila 5 (página), Col 5
    vTaskDelay(pdMS_TO_TICKS(2000)); 
    limpiar(); // Borra para empezar
}

// Actualizar pantalla 
void PantallaOLED::actualizar(int presion, int estado, int impactos) {
    char linea[22];
    setCursor(0, 0);
    _escribirString("SMART INSOLE"); // Título
    int porcentaje = (presion * 100) / 4095; // Convierte bits (0-4095) a %
    if (porcentaje > 100) porcentaje = 100; // Saturación por seguridad
    if (porcentaje < 0) porcentaje = 0; // Valor mínimo
    snprintf(linea, sizeof(linea), "P:%d (%d%%)  ", presion, porcentaje);
    setCursor(2, 0);
    _escribirString(linea);
    const char *estado_str;
    if      (estado == 0)       estado_str = "ESTADO: OK     ";
    else if (estado == 1)       estado_str = "ESTADO: ALERTA ";
    else if (estado == 2)       estado_str = "ESTADO: ERROR  ";
    else                        estado_str = "VERIF SENSOR   ";
    setCursor(4, 0);
    _escribirString(estado_str);
    snprintf(linea, sizeof(linea), "IMPACT: %d     ", impactos);
    setCursor(6, 0);
    _escribirString(linea);
}
