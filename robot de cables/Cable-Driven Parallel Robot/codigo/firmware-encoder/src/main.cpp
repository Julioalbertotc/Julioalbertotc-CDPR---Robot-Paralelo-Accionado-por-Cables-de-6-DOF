#include <Arduino.h>
#include "Config.h"

#if !SIMULATION_MODE
#include "driver/pcnt.h"
#endif

// Identificador único de arranque generado pseudoaleatoriamente en setup()
uint8_t boot_id = 0;

// Variables de Simulación
struct SimMotor {
    float current = 0.0f;
    float omega = 0.0f;
    float angle = 0.0f;
    int32_t ticks = 0;
};
SimMotor sim_motors[8];

// =============================================================================
// CÁLCULO DE CRC8 (Dallas/Maxim 0x31)
// =============================================================================
uint8_t calculateCRC8(const uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        uint8_t inbyte = data[i];
        for (uint8_t j = 8; j; j--) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) {
                crc ^= 0x8C; // Dallas/Maxim polynomial reflected
            }
            inbyte >>= 1;
        }
    }
    return crc;
}

// =============================================================================
// PARSER DE UART PARA CONSIGNAS PWM (MAIN -> ENCODER, solo en SIMULATION_MODE)
// =============================================================================
#if SIMULATION_MODE
enum ParserState {
    STATE_SEARCH_SOF1,
    STATE_SEARCH_SOF2,
    STATE_READ_LEN,
    STATE_READ_SEQ,
    STATE_READ_PAYLOAD,
    STATE_READ_CRC
};

ParserState parser_state = STATE_SEARCH_SOF1;
uint8_t rx_buffer[32]; // Payload de 32 bytes
int rx_index = 0;
uint32_t last_rx_byte_time = 0;
float rx_pwms[8] = {0.0f};
uint32_t last_valid_pwm_time = 0;
uint8_t rx_seq = 0;

void parseIncomingUART() {
    while (Serial2.available() > 0) {
        uint8_t c = Serial2.read();
        uint32_t now = millis();
        
        // Timeout de 5 ms por byte para resincronización ante pérdida de flujo
        if (parser_state != STATE_SEARCH_SOF1 && (now - last_rx_byte_time > 5)) {
            parser_state = STATE_SEARCH_SOF1;
        }
        last_rx_byte_time = now;
        
        switch (parser_state) {
            case STATE_SEARCH_SOF1:
                if (c == 0xCC) {
                    parser_state = STATE_SEARCH_SOF2;
                }
                break;
                
            case STATE_SEARCH_SOF2:
                if (c == 0xDD) {
                    parser_state = STATE_READ_LEN;
                } else {
                    parser_state = STATE_SEARCH_SOF1;
                }
                break;
                
            case STATE_READ_LEN:
                if (c == 0x20) {
                    parser_state = STATE_READ_SEQ;
                } else {
                    parser_state = STATE_SEARCH_SOF1;
                }
                break;
                
            case STATE_READ_SEQ:
                rx_seq = c;
                rx_index = 0;
                parser_state = STATE_READ_PAYLOAD;
                break;
                
            case STATE_READ_PAYLOAD:
                rx_buffer[rx_index++] = c;
                if (rx_index >= 32) {
                    parser_state = STATE_READ_CRC;
                }
                break;
                
            case STATE_READ_CRC:
                {
                    uint8_t rx_crc = c;
                    
                    // CRC se calcula sobre SEQ + PAYLOAD
                    uint8_t crc_data[33];
                    crc_data[0] = rx_seq;
                    memcpy(&crc_data[1], rx_buffer, 32);
                    
                    uint8_t calc_crc = calculateCRC8(crc_data, 33);
                    if (calc_crc == rx_crc) {
                        memcpy(rx_pwms, rx_buffer, 32);
                        last_valid_pwm_time = now;
                    }
                    parser_state = STATE_SEARCH_SOF1;
                }
                break;
        }
    }
}

void simulateStep(int i, float pwm_val, float dt) {
    float v_in = (pwm_val / 255.0f) * SIM_VOLTAGE;
    
    // Ecuación diferencial del motor DC
    float di = (v_in - SIM_MOTOR_R * sim_motors[i].current - SIM_MOTOR_KE * sim_motors[i].omega) / SIM_MOTOR_L;
    sim_motors[i].current += di * dt;
    
    float torque = SIM_MOTOR_KT * sim_motors[i].current;
    float tension_load = 0.005f; // Carga de tensión constante en Nm
    float total_torque = torque - tension_load;
    
    float d_omega = (total_torque - SIM_MOTOR_B * sim_motors[i].omega) / SIM_MOTOR_J;
    sim_motors[i].omega += d_omega * dt;
    
    sim_motors[i].angle += sim_motors[i].omega * dt;
    sim_motors[i].ticks = (int32_t)(sim_motors[i].angle * TICKS_PER_REV / (2.0f * PI));
}
#endif

// =============================================================================
// INICIALIZACIÓN DE CONTADORES DE HARDWARE PCNT (Solo en modo físico)
// =============================================================================
#if !SIMULATION_MODE
void initPCNT() {
    for (int i = 0; i < 8; i++) {
        pcnt_config_t pcnt_config;
        pcnt_config.pulse_gpio_num = ENCODER_PINS[i].encA;
        pcnt_config.ctrl_gpio_num = ENCODER_PINS[i].encB;
        pcnt_config.channel = PCNT_CHANNEL_0;
        pcnt_config.unit = (pcnt_unit_t)ENCODER_PINS[i].pcnt_unit;
        
        pcnt_config.pos_mode = PCNT_COUNT_INC;  
        pcnt_config.neg_mode = PCNT_COUNT_DEC;  
        
        pcnt_config.lctrl_mode = PCNT_MODE_KEEP;    
        pcnt_config.hctrl_mode = PCNT_MODE_REVERSE; 
        
        pcnt_config.counter_h_lim = 32767;
        pcnt_config.counter_l_lim = -32768;
        
        pcnt_unit_config(&pcnt_config);
        
        pcnt_counter_pause((pcnt_unit_t)ENCODER_PINS[i].pcnt_unit);
        pcnt_counter_clear((pcnt_unit_t)ENCODER_PINS[i].pcnt_unit);
        pcnt_counter_resume((pcnt_unit_t)ENCODER_PINS[i].pcnt_unit);
    }
}
#endif

// =============================================================================
// TAREA PRINCIPAL DE ADQUISICIÓN Y ENVÍO (Core 1)
// =============================================================================
void encoderTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1); // 1 ms (1 kHz exactos)
    uint8_t seq = 0;

    while (true) {
        int32_t current_ticks[8] = {0};

        #if SIMULATION_MODE
        // Leer y parsear entrada de PWMs de forma no bloqueante
        parseIncomingUART();
        
        // Timeout de seguridad en simulación: si no llega PWM, forzar a 0.0f
        if (millis() - last_valid_pwm_time > 50) {
            for (int i = 0; i < 8; i++) {
                rx_pwms[i] = 0.0f;
            }
        }
        
        // Correr integración física del motor DC
        float dt = 0.001f; // 1 ms
        for (int i = 0; i < 8; i++) {
            simulateStep(i, rx_pwms[i], dt);
            current_ticks[i] = sim_motors[i].ticks;
        }
        #else
        // Leer valores físicos desde PCNT
        for (int i = 0; i < 8; i++) {
            int16_t raw_count = 0;
            pcnt_get_counter_value((pcnt_unit_t)ENCODER_PINS[i].pcnt_unit, &raw_count);
            current_ticks[i] = raw_count;
        }
        #endif

        // Empaquetar el frame ENCODER -> MAIN
        uint8_t tx_frame[38];
        tx_frame[0] = 0xAA;
        tx_frame[1] = 0xBB;
        tx_frame[2] = 0x20;
        tx_frame[3] = seq;
        tx_frame[4] = boot_id;
        
        // Serialización del payload por copia directa de memoria (Little-Endian)
        memcpy(&tx_frame[5], current_ticks, 32);
        
        // Calcular CRC8 sobre bytes [3] a [36] (seq, boot_id, payload)
        tx_frame[37] = calculateCRC8(&tx_frame[3], 34);
        
        // Enviar por UART de interconexión
        Serial2.write(tx_frame, 38);
        
        seq++;
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    
    // UART2 de interconexión con MAIN
    Serial2.begin(INTERCONNECT_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    
    // Generación del BOOT_ID pseudoaleatorio único
    boot_id = (uint8_t)(esp_random() % 256);
    
    Serial.print("--- CDPR ENCODER SLAVE INITIALIZED ---\n");
    Serial.printf("BOOT_ID asignado: 0x%02X\n", boot_id);

    #if !SIMULATION_MODE
    initPCNT();
    Serial.println("Hardware PCNT configurado para los 8 encoders.");
    #else
    Serial.println("MODO SIMULACIÓN ACTIVO (Slave): Simulando física del motor por software.");
    #endif

    // Crear la tarea periódica a 1 kHz fijada al Core 1
    xTaskCreatePinnedToCore(
        encoderTask,
        "EncoderTask",
        4096,
        NULL,
        9, // Alta prioridad
        NULL,
        1
    );
}

void loop() {
    // El loop queda libre para tareas secundarias o de debug
    delay(100);
}
