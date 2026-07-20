#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <WiFi.h>
#include "Config.h"
#include "SystemState.h"
#include "Kinematics.h"
#include "MotorController.h"
#include "Globals.h"
#include "WebInterface.h"

#if !SIMULATION_MODE
#include "driver/pcnt.h"
#endif

// =============================================================================
// VARIABLES GLOBALES DE UART Y PROTECCIÓN
// =============================================================================
volatile int32_t uart_raw_ticks[8] = {0};
volatile uint8_t uart_boot_id = 0;
volatile uint32_t last_valid_frame_time = 0;
uint32_t dropped_frames = 0;

uint8_t active_boot_id = 0;
bool boot_id_initialized = false;
portMUX_TYPE uartMux = portMUX_INITIALIZER_UNLOCKED;

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
                crc ^= 0x8C; // Polinomio Dallas/Maxim reflejado
            }
            inbyte >>= 1;
        }
    }
    return crc;
}

// =============================================================================
// TRANSMISIÓN DE PWM Y DIRECCIÓN AL ESP32 SUB (Motores 4 a 7)
// =============================================================================
void transmitPWMToEncoder() {
    static uint8_t tx_seq = 0;
    uint8_t tx_frame[37];
    tx_frame[0] = 0xCC;
    tx_frame[1] = 0xDD;
    tx_frame[2] = 0x20;
    tx_frame[3] = tx_seq;
    
    float pwms[8];
    for (int i = 0; i < 8; i++) {
        pwms[i] = (float)motors[i].getPWM();
    }
    
    // Serializar por copia directa de memoria (Little-Endian)
    memcpy(&tx_frame[4], pwms, 32);
    
    // CRC se calcula sobre SEQ + PAYLOAD (33 bytes)
    tx_frame[36] = calculateCRC8(&tx_frame[3], 33);
    
    Serial2.write(tx_frame, 37);
    tx_seq++;
}

// =============================================================================
// PARSER NO BLOQUEANTE DE TELEMETRÍA (ENCODER -> MAIN)
// =============================================================================
enum ParserState {
    STATE_SEARCH_SOF1,
    STATE_SEARCH_SOF2,
    STATE_READ_LEN,
    STATE_READ_SEQ,
    STATE_READ_BOOT_ID,
    STATE_READ_PAYLOAD,
    STATE_READ_CRC
};

ParserState parser_state = STATE_SEARCH_SOF1;
uint8_t rx_buffer[32];
int rx_index = 0;
uint32_t last_rx_byte_time = 0;
uint8_t rx_seq = 0;
uint8_t rx_boot_id = 0;

bool has_prev_seq = false;
uint8_t prev_seq = 0;

void parseIncomingUART() {
    while (Serial2.available() > 0) {
        uint8_t c = Serial2.read();
        uint32_t now = millis();
        
        // Timeout de 5 ms por byte para resincronización automática
        if (parser_state != STATE_SEARCH_SOF1 && (now - last_rx_byte_time > 5)) {
            parser_state = STATE_SEARCH_SOF1;
        }
        last_rx_byte_time = now;
        
        switch (parser_state) {
            case STATE_SEARCH_SOF1:
                if (c == 0xAA) {
                    parser_state = STATE_SEARCH_SOF2;
                }
                break;
                
            case STATE_SEARCH_SOF2:
                if (c == 0xBB) {
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
                parser_state = STATE_READ_BOOT_ID;
                break;
                
            case STATE_READ_BOOT_ID:
                rx_boot_id = c;
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
                    
                    // CRC se calcula sobre SEQ + BOOT_ID + PAYLOAD (34 bytes)
                    uint8_t crc_data[34];
                    crc_data[0] = rx_seq;
                    crc_data[1] = rx_boot_id;
                    memcpy(&crc_data[2], rx_buffer, 32);
                    
                    uint8_t calc_crc = calculateCRC8(crc_data, 34);
                    if (calc_crc == rx_crc) {
                        // Sección crítica rápida para actualizar valores globales
                        portENTER_CRITICAL(&uartMux);
                        int32_t rx_ticks[8];
                        memcpy(rx_ticks, rx_buffer, 32);
                        #if SIMULATION_MODE
                        // En modo simulación copiamos los 8 encoders desde el modelo físico del esclavo
                        for (int i = 0; i < 8; i++) {
                            uart_raw_ticks[i] = rx_ticks[i];
                        }
                        #else
                        // En modo real, los encoders 0-3 son leídos localmente por PCNT en MAIN.
                        // Solo copiamos los encoders remotos 4-7 recibidos por UART.
                        for (int i = 4; i < 8; i++) {
                            uart_raw_ticks[i] = rx_ticks[i];
                        }
                        #endif
                        uart_boot_id = rx_boot_id;
                        last_valid_frame_time = now;
                        portEXIT_CRITICAL(&uartMux);
                        
                        // Control de frames perdidos
                        if (has_prev_seq) {
                            uint8_t expected_seq = prev_seq + 1;
                            if (rx_seq != expected_seq) {
                                int diff = (rx_seq >= expected_seq) ? (rx_seq - expected_seq) : (256 - expected_seq + rx_seq);
                                dropped_frames += diff;
                            }
                        }
                        prev_seq = rx_seq;
                        has_prev_seq = true;
                    }
                    parser_state = STATE_SEARCH_SOF1;
                }
                break;
        }
    }
}

// =============================================================================
// INICIALIZACIÓN DE CONTADORES DE HARDWARE PCNT (Solo en modo físico)
// =============================================================================
#if !SIMULATION_MODE
void initPCNT() {
    for (int i = 0; i < 8; i++) {
        if (!MOTOR_PINS[i].is_local) continue;
        
        pcnt_config_t pcnt_config;
        pcnt_config.pulse_gpio_num = MOTOR_PINS[i].encA;
        pcnt_config.ctrl_gpio_num = MOTOR_PINS[i].encB;
        pcnt_config.channel = PCNT_CHANNEL_0;
        pcnt_config.unit = (pcnt_unit_t)MOTOR_PINS[i].pcnt_unit;
        
        pcnt_config.pos_mode = PCNT_COUNT_INC;  
        pcnt_config.neg_mode = PCNT_COUNT_DEC;  
        
        pcnt_config.lctrl_mode = PCNT_MODE_KEEP;    
        pcnt_config.hctrl_mode = PCNT_MODE_REVERSE; 
        
        pcnt_config.counter_h_lim = 32767;
        pcnt_config.counter_l_lim = -32768;
        
        pcnt_unit_config(&pcnt_config);
        
        pcnt_counter_pause((pcnt_unit_t)MOTOR_PINS[i].pcnt_unit);
        pcnt_counter_clear((pcnt_unit_t)MOTOR_PINS[i].pcnt_unit);
        pcnt_counter_resume((pcnt_unit_t)MOTOR_PINS[i].pcnt_unit);
    }
}
#endif

// =============================================================================
// NÚCLEO 2: TAREA DE CONTROL DE LAZO CERRADO (1 kHz)
// =============================================================================
void controlLoopTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1000 / CONTROL_LOOP_HZ); // 1 ms

    uint32_t homing_timer = 0;

    while (true) {
        float dt = 1.0f / (float)CONTROL_LOOP_HZ;

        // 1. Leer encoders locales (si es modo real)
        int32_t local_raw[8] = {0};
        #if !SIMULATION_MODE
        for (int i = 0; i < 8; i++) {
            if (MOTOR_PINS[i].is_local) {
                int16_t raw_count = 0;
                pcnt_get_counter_value((pcnt_unit_t)MOTOR_PINS[i].pcnt_unit, &raw_count);
                local_raw[i] = raw_count;
            }
        }
        #endif

        // 2. Sección Crítica Rápida: Copiar valores del búfer UART
        uint8_t local_boot_id;
        uint32_t last_rx;
        
        portENTER_CRITICAL(&uartMux);
        #if SIMULATION_MODE
        for (int i = 0; i < 8; i++) {
            local_raw[i] = uart_raw_ticks[i];
        }
        #else
        for (int i = 4; i < 8; i++) {
            local_raw[i] = uart_raw_ticks[i]; // Carga encoders remotos 4-7
        }
        #endif
        local_boot_id = uart_boot_id;
        last_rx = last_valid_frame_time;
        portEXIT_CRITICAL(&uartMux);

        // 3. Manejo del Paro de Emergencia por Comando
        if (request_estop) {
            current_state = STATE_ESTOP;
            request_estop = false;
        }

        // 4. Monitoreo de timeout UART (50 ms)
        if (millis() - last_rx > 50) {
            current_state = STATE_ESTOP;
        }

        // 5. Pasar ticks crudos a las instancias de controlador
        for (int i = 0; i < 8; i++) {
            motors[i].setRawTicks(local_raw[i]);
        }

        // 6. Ejecutar máquina de estados de control y STBY físico local
        if (current_state == STATE_ESTOP) {
#if !SIMULATION_MODE
            digitalWrite(STBY_PIN, LOW); // Apagado instantáneo de puentes H locales
#endif
            for (int i = 0; i < 8; i++) {
                motors[i].setTargetTicks(motors[i].getCurrentTicks());
                motors[i].update(dt);
            }
        } else {
#if !SIMULATION_MODE
            digitalWrite(STBY_PIN, HIGH); // Habilitar puentes H locales
#endif
            switch (current_state) {
                case STATE_UNHOMED:
                    for (int i = 0; i < 8; i++) {
                        motors[i].setTargetTicks(motors[i].getCurrentTicks());
                        motors[i].update(dt);
                    }
                    
                    if (request_homing) {
                        current_state = STATE_HOMING;
                        request_homing = false;
                        homing_timer = 0;
                        
                        active_boot_id = local_boot_id;
                        boot_id_initialized = true;
                        
                        for (int i = 0; i < 8; i++) {
                            motors[i].resetEncoderLocal();
                        }
                        
                        float init_lengths[8];
                        calculateInverseKinematics(
                            DEFAULT_START_POSE_POS.x,
                            DEFAULT_START_POSE_POS.y,
                            DEFAULT_START_POSE_POS.z,
                            DEFAULT_START_POSE_ROT.x,
                            DEFAULT_START_POSE_ROT.y,
                            DEFAULT_START_POSE_ROT.z,
                            init_lengths
                        );
                        
                        for (int i = 0; i < 8; i++) {
                            motors[i].setTargetPosition(init_lengths[i]);
                        }
                    }
                    break;

                case STATE_HOMING:
                    for (int i = 0; i < 8; i++) {
                        motors[i].update(dt);
                    }
                    
                    homing_timer++;
                    if (homing_timer >= 500) {
                        current_state = STATE_READY;
                        target_x = DEFAULT_START_POSE_POS.x;
                        target_y = DEFAULT_START_POSE_POS.y;
                        target_z = DEFAULT_START_POSE_POS.z;
                        target_roll = DEFAULT_START_POSE_ROT.x;
                        target_pitch = DEFAULT_START_POSE_ROT.y;
                        target_yaw = DEFAULT_START_POSE_ROT.z;
                    }
                    break;

                case STATE_READY:
                    // Validación de protección de reinicio del ENCODER/SUB
                    if (boot_id_initialized && (local_boot_id != active_boot_id)) {
                        current_state = STATE_ESTOP;
                        boot_id_initialized = false; 
                        Serial.println("[ALERTA CRÍTICA] Se detectó reinicio del ESP32 SUB. E-STOP forzado.");
                    }

                    if (request_center) {
                        target_x = DEFAULT_START_POSE_POS.x;
                        target_y = DEFAULT_START_POSE_POS.y;
                        target_z = DEFAULT_START_POSE_POS.z;
                        target_roll = DEFAULT_START_POSE_ROT.x;
                        target_pitch = DEFAULT_START_POSE_ROT.y;
                        target_yaw = DEFAULT_START_POSE_ROT.z;
                        request_center = false;
                    }

                    float target_lengths[8];
                    calculateInverseKinematics(target_x, target_y, target_z, target_roll, target_pitch, target_yaw, target_lengths);
                    
                    for (int i = 0; i < 8; i++) {
                        motors[i].setTargetPosition(target_lengths[i]);
                        motors[i].update(dt);
                    }
                    
                    current_x = target_x;
                    current_y = target_y;
                    current_z = target_z;
                    current_roll = target_roll;
                    current_pitch = target_pitch;
                    current_yaw = target_yaw;
                    break;
            }
        }

        // 7. Enviar PWMs calculados de vuelta al ESP32 SUB siempre (para simulación y motores físicos 4-7)
        transmitPWMToEncoder();

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// =============================================================================
// NÚCLEO 1: SETUP Y BUCLE PRINCIPAL (WiFi, Servidor, Serial JSON)
// =============================================================================
void setup() {
    Serial.begin(SERIAL_BAUD);
    while(!Serial && millis() < 3000);
    Serial.println("\n--- CDPR CABLE ROBOT FIRMWARE (MAIN/MOTOR) ---");

    // Inicializar UART2 de interconexión
    Serial2.begin(INTERCONNECT_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    Serial.println("UART2 de interconexión con ESP32 SUB inicializado.");

#if !SIMULATION_MODE
    // Inicializar pin de desactivación de drivers TB6612FNG locales (Standby)
    pinMode(STBY_PIN, OUTPUT);
    digitalWrite(STBY_PIN, HIGH); // Puentes H activos por defecto
    Serial.println("Pin Standby local inicializado.");
    
    // Inicializar hardware PCNT para los encoders locales
    initPCNT();
    Serial.println("PCNT inicializado para encoders locales 0 a 3.");
#endif

    // Inicializar motores
    for (int i = 0; i < 8; i++) {
        motors[i].begin(i);
    }

#if !SIMULATION_MODE
    Serial.println("Motores locales configurados por hardware.");
#else
    Serial.println("MODO SIMULACIÓN ACTIVO: Los motores y encoders se simulan en la placa SUB.");
#endif

    // Inicialización del WiFi en modo Punto de Acceso (AP)
    Serial.println("\nIniciando Punto de Acceso WiFi...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, 0, AP_MAX_CONN);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("Punto de Acceso WiFi creado. SSID: ");
    Serial.println(AP_SSID);
    Serial.print(">>> INGRESA ESTA IP EN TU NAVEGADOR: ");
    Serial.println(IP);
    Serial.println("=================================================");

    // Inicializar Servidor Web y WebSockets
    initWebInterface();

    // Crear la tarea del control loop en el Core 0
    xTaskCreatePinnedToCore(
        controlLoopTask,
        "ControlLoopTask",
        4096,
        NULL,
        9,
        NULL,
        0
    );
    
    Serial.println("Tarea del Lazo de Control (1 kHz) iniciada en Core 0.");
}

void loop() {
    static uint32_t last_telemetry_time = 0;
    uint32_t now = millis();

    // 1. Parsear UART de encoders de forma no bloqueante
    parseIncomingUART();

    // 2. Procesar entrada Serial JSON de la PC
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, input);
        
        if (!error) {
            const char* cmd = doc["cmd"];
            if (cmd != nullptr) {
                if (strcmp(cmd, "pose") == 0 && current_state == STATE_READY) {
                    target_x = doc["x"] | target_x;
                    target_y = doc["y"] | target_y;
                    target_z = doc["z"] | target_z;
                    target_roll = doc["r"] | target_roll;
                    target_pitch = doc["p"] | target_pitch;
                    target_yaw = doc["yaw"] | target_yaw;
                } else if (strcmp(cmd, "homing") == 0) {
                    request_homing = true;
                } else if (strcmp(cmd, "center") == 0) {
                    request_center = true;
                } else if (strcmp(cmd, "estop") == 0) {
                    request_estop = true;
                }
            }
        }
    }

    // 3. Transmitir telemetría periódica a WebSockets y Serial (20 Hz = 50 ms)
    if (now - last_telemetry_time >= 50) {
        last_telemetry_time = now;
        
        float cur_lengths[8];
        float tgt_lengths[8];
        int32_t cur_ticks[8];
        int32_t tgt_ticks[8];
        int motor_pwms[8];

        for (int i = 0; i < 8; i++) {
            cur_lengths[i] = motors[i].getCurrentLengthCm();
            cur_ticks[i] = motors[i].getCurrentTicks();
            tgt_ticks[i] = motors[i].getTargetTicks();
            tgt_lengths[i] = (float)tgt_ticks[i] / TICKS_PER_CM;
            motor_pwms[i] = motors[i].getPWM();
        }

        // Broadcast a clientes Web
        broadcastTelemetry(
            current_x, current_y, current_z,
            current_roll, current_pitch, current_yaw,
            cur_lengths, tgt_lengths,
            cur_ticks, tgt_ticks,
            motor_pwms
        );

        // Envío de reporte JSON simplificado por Serial
        JsonDocument serialDoc;
        serialDoc["state"] = (int)current_state;
        serialDoc["dropped_frames"] = dropped_frames; // Reportar frames perdidos
        JsonObject pos = serialDoc["pose"].to<JsonObject>();
        pos["x"] = current_x;
        pos["y"] = current_y;
        pos["z"] = current_z;
        pos["r"] = current_roll;
        pos["p"] = current_pitch;
        pos["yaw"] = current_yaw;

        JsonArray lengths = serialDoc["lengths"].to<JsonArray>();
        JsonArray pwms = serialDoc["pwms"].to<JsonArray>();
        for (int i = 0; i < 8; i++) {
            lengths.add(cur_lengths[i]);
            pwms.add(motor_pwms[i]);
        }

        serializeJson(serialDoc, Serial);
        Serial.println();
    }

    ws.cleanupClients();
    delay(1); // Pequeña pausa para permitir procesamiento del planificador de FreeRTOS
}