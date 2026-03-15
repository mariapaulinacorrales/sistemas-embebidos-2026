// Ejercicio: Alarma con cuenta regresiva en 2 displays de 7 segmentos
// - Muestra el valor inicial al encender
// - S1 → inicia / pausa la cuenta regresiva
// - Al llegar a 00 → LED/buzzer parpadea a 2 Hz
// - S2 → silencia la alarma y reinicia al valor inicial

#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "esp_task_wdt.h"

// ================================================================
// VALOR INICIAL DE LA CUENTA REGRESIVA
// Cambiar este número para configurar el tiempo (entre 10 y 99)
// ================================================================
#define VALOR_INICIAL 30  // Cuenta regresiva empieza en 30 segundos

// Pines de los segmentos (compartidos por ambos displays)
#define SEG_A 36
#define SEG_B 39
#define SEG_C 34
#define SEG_D 35
#define SEG_E 32
#define SEG_F 33
#define SEG_G 25

// Pines de control de displays (via transistor NPN, igual que antes)
// 1 = display activo, 0 = display apagado
#define UNI_COM 26
#define DEC_COM 27

// Pines de botones y alarma
#define S1          14  // START/STOP
#define S2          12  // SILENCIAR y REINICIAR
#define PIN_ALARMA  13  // LED o buzzer de alarma

// Tiempos
#define UN_SEGUNDO   1000000  // 1 segundo en microsegundos
#define MEDIO_SEGUNDO 250000  // 250ms → parpadeo a 2Hz
                                 // 2 Hz = 2 parpadeos por segundo
                                 // 1 parpadeo = encendido + apagado
                                 // cada mitad dura 250ms → 500ms total = 2 Hz
#define DEBOUNCE_US  200000   // 200ms antirebote

// ================================================================
// VARIABLES VOLÁTILES
// static   → solo existen en este archivo
// volatile → no optimizar, pueden cambiar desde interrupciones
// ================================================================
static volatile bool bandera_s1        = false; // S1 fue presionado
static volatile bool bandera_s2        = false; // S2 fue presionado
static volatile bool bandera_segundo   = false; // El timer avisó: pasó 1 segundo
static volatile bool bandera_parpadeo  = false; // El timer avisó: cambiar estado alarma
static volatile uint64_t ultimo_s1     = 0;     // Antirebote S1
static volatile uint64_t ultimo_s2     = 0;     // Antirebote S2

// ================================================================
// ISR DEL BOTÓN S1 (START/STOP)
// ================================================================
static void IRAM_ATTR isr_s1(void *arg) {
    uint64_t ahora = 0;
    timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &ahora);
    if (ahora - ultimo_s1 >= DEBOUNCE_US) {
        bandera_s1 = true;
        ultimo_s1  = ahora;
    }
}

// ================================================================
// ISR DEL BOTÓN S2 (SILENCIAR/REINICIAR)
// ================================================================
static void IRAM_ATTR isr_s2(void *arg) {
    uint64_t ahora = 0;
    timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &ahora);
    if (ahora - ultimo_s2 >= DEBOUNCE_US) {
        bandera_s2 = true;
        ultimo_s2  = ahora;
    }
}

// ================================================================
// ISR DEL TIMER (ALARMA)
// Se ejecuta automáticamente cuando el timer llega al valor
// de alarma. Dependiendo del estado del sistema, puede avisar
// que pasó 1 segundo (cuenta regresiva) o que hay que
// cambiar el estado del parpadeo (alarma activa)
// Devuelve true → requisito del timer_isr_callback_add
// ================================================================
static bool IRAM_ATTR timer_isr(void *arg) {
    // Usamos un puntero al argumento para saber en qué modo estamos
    // 'arg' es el argumento que pasamos en timer_isr_callback_add
    // Si arg = 0 → modo cuenta regresiva (avisar cada 1 segundo)
    // Si arg = 1 → modo parpadeo (avisar cada 250ms)
    int modo = (int)arg; // (int) → "convertir arg de void* a int"
                         //Esto se llama castear: cambiar el tipo de un dato
                        // (void*)0 casteado a int → 0
                        // (void*)1 casteado a int → 1

    if (modo == 0) {
        bandera_segundo = true;  // Pasó 1 segundo → decrementar contador
                                // El timer está en modo cuenta regresiva
                                  // → avisamos que pasó 1 segundo
    } else {
        bandera_parpadeo = true; // Pasó 250ms → cambiar estado LED alarma
                                // El timer está en modo parpadeo (alarma activa)
                                  // → avisamos que hay que cambiar el estado del LED
    }
    return true;
}

// ================================================================
// FUNCIÓN PARA CONFIGURAR LA ALARMA DEL TIMER
// Recibe el tiempo en microsegundos hasta la próxima alarma
// ================================================================
void configurar_alarma(uint64_t tiempo_us) {
    uint64_t ahora = 0;
    timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &ahora);
    // Leemos el timer ahora para saber desde dónde contar
    // ahora = 3,000,000 (la regla llegó hasta aquí)

    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, ahora + tiempo_us);
    // La alarma se dispara cuando el timer llegue a: ahora + tiempo_us
    // ahora + UN_SEGUNDO = 3,000,000 + 1,000,000 = 4,000,000
    // "cuando la regla llegue a 4,000,000 → disparar alarma"

    timer_set_alarm(TIMER_GROUP_0, TIMER_0, TIMER_ALARM_EN);
    // Reactivar la alarma → se desactiva sola al dispararse,
    // por eso hay que reactivarla manualmente cada vez

}

// ================================================================
// FUNCIÓN PARA MOSTRAR UN DÍGITO EN EL DISPLAY
// Lógica negada: 0 = segmento encendido, 1 = apagado
// ================================================================
void mostrar_digito(int numero) {
    if (numero == 0) { gpio_set_level(SEG_A, 0); gpio_set_level(SEG_B, 0); gpio_set_level(SEG_C, 0); gpio_set_level(SEG_D, 0); gpio_set_level(SEG_E, 0); gpio_set_level(SEG_F, 0); gpio_set_level(SEG_G, 1); }
    if (numero == 1) { gpio_set_level(SEG_A, 1); gpio_set_level(SEG_B, 0); gpio_set_level(SEG_C, 0); gpio_set_level(SEG_D, 1); gpio_set_level(SEG_E, 1); gpio_set_level(SEG_F, 1); gpio_set_level(SEG_G, 1); }
    if (numero == 2) { gpio_set_level(SEG_A, 0); gpio_set_level(SEG_B, 0); gpio_set_level(SEG_C, 1); gpio_set_level(SEG_D, 0); gpio_set_level(SEG_E, 0); gpio_set_level(SEG_F, 1); gpio_set_level(SEG_G, 0); }
    if (numero == 3) { gpio_set_level(SEG_A, 0); gpio_set_level(SEG_B, 0); gpio_set_level(SEG_C, 0); gpio_set_level(SEG_D, 0); gpio_set_level(SEG_E, 1); gpio_set_level(SEG_F, 1); gpio_set_level(SEG_G, 0); }
    if (numero == 4) { gpio_set_level(SEG_A, 1); gpio_set_level(SEG_B, 0); gpio_set_level(SEG_C, 0); gpio_set_level(SEG_D, 1); gpio_set_level(SEG_E, 1); gpio_set_level(SEG_F, 0); gpio_set_level(SEG_G, 0); }
    if (numero == 5) { gpio_set_level(SEG_A, 0); gpio_set_level(SEG_B, 1); gpio_set_level(SEG_C, 0); gpio_set_level(SEG_D, 0); gpio_set_level(SEG_E, 1); gpio_set_level(SEG_F, 0); gpio_set_level(SEG_G, 0); }
    if (numero == 6) { gpio_set_level(SEG_A, 0); gpio_set_level(SEG_B, 1); gpio_set_level(SEG_C, 0); gpio_set_level(SEG_D, 0); gpio_set_level(SEG_E, 0); gpio_set_level(SEG_F, 0); gpio_set_level(SEG_G, 0); }
    if (numero == 7) { gpio_set_level(SEG_A, 0); gpio_set_level(SEG_B, 0); gpio_set_level(SEG_C, 0); gpio_set_level(SEG_D, 1); gpio_set_level(SEG_E, 1); gpio_set_level(SEG_F, 1); gpio_set_level(SEG_G, 1); }
    if (numero == 8) { gpio_set_level(SEG_A, 0); gpio_set_level(SEG_B, 0); gpio_set_level(SEG_C, 0); gpio_set_level(SEG_D, 0); gpio_set_level(SEG_E, 0); gpio_set_level(SEG_F, 0); gpio_set_level(SEG_G, 0); }
    if (numero == 9) { gpio_set_level(SEG_A, 0); gpio_set_level(SEG_B, 0); gpio_set_level(SEG_C, 0); gpio_set_level(SEG_D, 0); gpio_set_level(SEG_E, 1); gpio_set_level(SEG_F, 0); gpio_set_level(SEG_G, 0); }
}

void app_main(void) {

    // ================================================================
    // WATCHDOG TIMER
    // 3 segundos → el while siempre lo alimenta mucho antes
    // ================================================================
    const esp_task_wdt_config_t wdt_cfg = {
        .timeout_ms = 3000,
        .idle_core_mask = 0,
        .trigger_panic = true
    };
    esp_task_wdt_init(&wdt_cfg);
    esp_task_wdt_add(NULL);

    // ================================================================
    // CONFIGURAR SEGMENTOS como salidas
    // ================================================================
    gpio_config_t seg_a_cfg = {
        .pin_bit_mask = (1ULL << SEG_A),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&seg_a_cfg);
    gpio_set_level(SEG_A, 1);  // Apagado inicial (HIGH = apagado cátodo)

    gpio_config_t seg_b_cfg = {
        .pin_bit_mask = (1ULL << SEG_B),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&seg_b_cfg);
    gpio_set_level(SEG_B, 1);

    gpio_config_t seg_c_cfg = {
        .pin_bit_mask = (1ULL << SEG_C),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&seg_c_cfg);
    gpio_set_level(SEG_C, 1);

    gpio_config_t seg_d_cfg = {
        .pin_bit_mask = (1ULL << SEG_D),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&seg_d_cfg);
    gpio_set_level(SEG_D, 1);

    gpio_config_t seg_e_cfg = {
        .pin_bit_mask = (1ULL << SEG_E),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&seg_e_cfg);
    gpio_set_level(SEG_E, 1);

    gpio_config_t seg_f_cfg = {
        .pin_bit_mask = (1ULL << SEG_F),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&seg_f_cfg);
    gpio_set_level(SEG_F, 1);

    gpio_config_t seg_g_cfg = {
        .pin_bit_mask = (1ULL << SEG_G),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&seg_g_cfg);
    gpio_set_level(SEG_G, 1);

    // ================================================================
    // CONFIGURAR PINES DE CONTROL DE DISPLAYS (multiplexación)
    // ================================================================
    gpio_config_t uni_com_cfg = {
        .pin_bit_mask = (1ULL << UNI_COM),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&uni_com_cfg);
    gpio_set_level(UNI_COM, 0);  // Apagado inicial (HIGH = display apagado)

    gpio_config_t dec_com_cfg = {
        .pin_bit_mask = (1ULL << DEC_COM),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&dec_com_cfg);
    gpio_set_level(DEC_COM, 0);  // Apagado inicial

    // ================================================================
    // CONFIGURAR PIN DE ALARMA (LED o buzzer) como salida
    // ================================================================
    gpio_config_t cfg_alarma = { 
        .pin_bit_mask = (1ULL << PIN_ALARMA), 
        .mode = GPIO_MODE_OUTPUT, 
        .pull_up_en = GPIO_PULLUP_DISABLE, 
        .pull_down_en = GPIO_PULLDOWN_DISABLE, 
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&cfg_alarma);
    gpio_set_level(PIN_ALARMA, 0); // Alarma apagada al inicio

    // ================================================================
    // CONFIGURAR BOTONES S1 y S2 como entradas
    // ================================================================
    gpio_config_t cfg_s1 = { 
        .pin_bit_mask = (1ULL << S1), 
        .mode = GPIO_MODE_INPUT, 
        .pull_up_en = GPIO_PULLUP_ENABLE, 
        .pull_down_en = GPIO_PULLDOWN_DISABLE, 
        .intr_type = GPIO_INTR_NEGEDGE };
    gpio_config(&cfg_s1);

    gpio_config_t cfg_s2 = { 
        .pin_bit_mask = (1ULL << S2), 
        .mode = GPIO_MODE_INPUT, 
        .pull_up_en = GPIO_PULLUP_ENABLE, 
        .pull_down_en = GPIO_PULLDOWN_DISABLE, 
        .intr_type = GPIO_INTR_NEGEDGE };
    gpio_config(&cfg_s2);

    // ================================================================
    // CONFIGURAR TIMER CON ALARMA
    // auto_reload = false → la alarma no se reinicia sola,
    // la reactivamos manualmente con configurar_alarma()
    // ================================================================
    timer_config_t timer_cfg = {
        .divider = 80,                 // 1 tick = 1 microsegundo
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,    // Alarma activada
        .auto_reload = false
    };
    timer_init(TIMER_GROUP_0, TIMER_0, &timer_cfg);

    uint64_t cero = 0;
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, cero);
    timer_isr_callback_add(TIMER_GROUP_0, TIMER_0, timer_isr, (void*)0, 0); 
    // ← Nombre de la función ISR que se ejecutará
    //   cuando se dispare la alarma
    // (void)*0 ← Argumento que le pasamos a la ISR ← NUEVO
    // (void*)0 → la nota dice "0" → modo cuenta regresiva
    // (void*)1 → la nota dice "1" → modo parpadeo

    // Registrar la ISR del timer
    // El argumento (void*)0 indica modo cuenta regresiva (1 segundo)
    // Más adelante lo cambiamos a (void*)1 para el parpadeo
    // (void*)0 → casteamos el número 0 a puntero void para pasarlo como argumento
    // 'castear' significa convertir un tipo de dato a otro
    // Dentro de la ISR lo convertimos de vuelta a int con: int modo = (int)arg

    timer_start(TIMER_GROUP_0, TIMER_0);

    // ================================================================
    // INSTALAR INTERRUPCIONES DE LOS BOTONES
    // ================================================================
    gpio_install_isr_service(0);
    gpio_isr_handler_add(S1, isr_s1, NULL);
    gpio_isr_handler_add(S2, isr_s2, NULL);

    // ================================================================
    // VARIABLES DEL SISTEMA
    // ================================================================
    int contador   = VALOR_INICIAL; // Cuenta regresiva empieza en el valor definido (30)
    bool corriendo = false;         // false = detenido, true = contando
    bool alarma_activa = false;     // true = llegó a 00, alarma sonando
    bool estado_alarma = false;     // Estado actual del LED de alarma (encendido/apagado)
                                    // Para el parpadeo, igual que led_state en el ejercicio del LED

    // ================================================================
    // LOOP PRINCIPAL
    // ================================================================
    while (1) {

        esp_task_wdt_reset(); // Alimentar el Watchdog

        // ============================================================
        // MANEJO DE S1 (START/STOP)
        // ============================================================
        if (bandera_s1) { // "¿la ISR del botón levantó esta bandera?"
                         // true = sí fue presionado → entrar al if

            bandera_s1 = false; // Bajar bandera

            if (!alarma_activa) {
                // Solo funciona si la alarma NO está sonando - el botón se presionó mientras estaba corriendo/pausado
                // !alarma_activa = "¿alarma_activa es FALSE?"
                // Es decir: "¿la alarma NO está activa?"
                corriendo = !corriendo;
                // Si estaba detenido → arranca
                // Si estaba corriendo → pausa

                if (corriendo) {
                    // Arrancó → configurar alarma para 1 segundo
                    configurar_alarma(UN_SEGUNDO);
                    // Quedó en true = ARRANCÓ
                    // → configurar la alarma para que avise en 1 segundo
                    // → el timer empieza a medir el primer segundo
                }
                // Si pausó → no configuramos alarma, el timer
                // se quedará esperando pero la bandera no se levantará
                // porque no reactivamos la alarma
            }
        }

        // ============================================================
        // MANEJO DE S2 (SILENCIAR Y REINICIAR)
        // ============================================================
        if (bandera_s2) {
            bandera_s2 = false; // Bajar bandera

            // S2 funciona en cualquier momento
            alarma_activa  = false;          // Apagar la alarma
            estado_alarma  = false;          // Apagar el LED de alarma
            gpio_set_level(PIN_ALARMA, 0);   // Apagar físicamente el pin
            corriendo      = false;          // Detener la cuenta
            contador       = VALOR_INICIAL;  // Reiniciar al valor inicial
            // No configuramos alarma → el sistema queda esperando S1
        }

        // ============================================================
        // MANEJO DE LA ALARMA DEL TIMER
        // ============================================================

        // -------------------------------------------------------
        // ¿Pasó 1 segundo? → decrementar la cuenta regresiva
        // -------------------------------------------------------
        if (bandera_segundo && corriendo && !alarma_activa) {
            bandera_segundo = false; // Bajar bandera

            contador--; // Decrementar la cuenta regresiva

            if (contador <= 0) {
                // ¡Llegó a 00! → activar alarma
                contador = 0;          // Garantizar que no baje de 0
                corriendo = false;     // Detener la cuenta
                alarma_activa = true;  // Activar modo alarma

                // Cambiar el argumento de la ISR del timer a modo parpadeo (1)
                // Para esto removemos la ISR anterior y agregamos una nueva
                // con el argumento cambiado
                timer_isr_callback_remove(TIMER_GROUP_0, TIMER_0);
                // timer_isr_callback_remove → quita la ISR registrada del timer
                // Es la función opuesta a timer_isr_callback_add

                timer_isr_callback_add(TIMER_GROUP_0, TIMER_0, timer_isr, (void*)1, 0);
                // Ahora la ISR del timer levantará bandera_parpadeo en vez de bandera_segundo
                // porque le pasamos (void*)1 como argumento

                configurar_alarma(MEDIO_SEGUNDO); // Primera alarma de parpadeo en 250ms
            } else {
                // Todavía hay tiempo → configurar alarma para el siguiente segundo
                configurar_alarma(UN_SEGUNDO);
            }
        }

        // -------------------------------------------------------
        // ¿Pasó 250ms? → cambiar estado del LED de alarma (parpadeo 2Hz)
        // -------------------------------------------------------
        if (bandera_parpadeo && alarma_activa) {
            bandera_parpadeo = false; // Bajar bandera

            estado_alarma = !estado_alarma;          // Cambiar estado: 0→1 o 1→0
            gpio_set_level(PIN_ALARMA, estado_alarma); // Aplicar al pin
            // Igual que en el ejercicio del LED parpadeante:
            // led_state = !led_state → gpio_set_level(LED_PIN, led_state)

            configurar_alarma(MEDIO_SEGUNDO); // Próximo parpadeo en 250ms
        }

        // ============================================================
        // MULTIPLEXACIÓN DE LOS 2 DISPLAYS
        // Igual que en el ejercicio del contador 0-99
        // Se hace en cada vuelta del while independientemente de todo lo demás
        // ============================================================
        int decena = contador / 10;
        /*
        // Ejemplos:
            30 / 10 = 3   → decena = 3  ✅
            27 / 10 = 2.7 → pero como es int, descarta el .7 → decena = 2  ✅
        */
        int unidad = contador % 10;
        /*
        30 % 10 = 0  → 30 ÷ 10 = 3 exacto, no sobra nada → unidad = 0  ✅
        27 % 10 = 7  → 27 ÷ 10 = 2, sobran 7             → unidad = 7  ✅
        5 % 10 = 5  → 5 ÷ 10  = 0, sobran 5             → unidad = 5  ✅
        */

        // Apagar ambos displays y limpiar segmentos
        gpio_set_level(UNI_COM, 0);
        gpio_set_level(DEC_COM, 0);
        gpio_set_level(SEG_A, 1); 
        gpio_set_level(SEG_B, 1);
        gpio_set_level(SEG_C, 1);
        gpio_set_level(SEG_D, 1); 
        gpio_set_level(SEG_E, 1); 
        gpio_set_level(SEG_F, 1);
        gpio_set_level(SEG_G, 1);

        // Mostrar decena (display izquierdo) 5ms
        mostrar_digito(decena);
        gpio_set_level(DEC_COM, 1);          // Activar display decenas
        vTaskDelay(pdMS_TO_TICKS(5));        // Mantener 5ms
        gpio_set_level(DEC_COM, 0);          // Apagar display decenas

        // Limpiar segmentos
        gpio_set_level(SEG_A, 1); 
        gpio_set_level(SEG_B, 1); 
        gpio_set_level(SEG_C, 1);
        gpio_set_level(SEG_D, 1); 
        gpio_set_level(SEG_E, 1); 
        gpio_set_level(SEG_F, 1);
        gpio_set_level(SEG_G, 1);

        // Mostrar unidad (display derecho) 5ms
        mostrar_digito(unidad);
        gpio_set_level(UNI_COM, 1);          // Activar display unidades
        vTaskDelay(pdMS_TO_TICKS(5));        // Mantener 5ms
        gpio_set_level(UNI_COM, 0);          // Apagar display unidades
    }
}