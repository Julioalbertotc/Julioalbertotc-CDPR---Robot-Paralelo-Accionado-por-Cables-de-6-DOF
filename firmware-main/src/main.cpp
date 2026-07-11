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
                crc ^= 0x8C; // PolinomioDallas/Maxim reflejado
            }
            inbyte >>= 1;
        }
    }
    return crc;
}

// =============================================================================
// TRANSMISIÓN DE PWM EN MODO SIMULACIÓN (MAIN -> ENCODER)
// =============================================================================
#if SIMULATION_MODE
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
#endif

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
                        memcpy((void*)uart_raw_ticks, rx_buffer, 32);
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
// NÚCLEO 2: TAREA DE CONTROL DE LAZO CERRADO (1 kHz)
// =============================================================================
void controlLoopTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1000 / CONTROL_LOOP_HZ); // 1 ms

    uint32_t homing_timer = 0;

    while (true) {
        float dt = 1.0f / (float)CONTROL_LOOP_HZ;

        // 1. Sección Crítica Rápida: Copiar valores del búfer UART
        int32_t local_raw[8];
        uint8_t local_boot_id;
        uint32_t last_rx;
        
        portENTER_CRITICAL(&uartMux);
        for (int i = 0; i < 8; i++) {
            local_raw[i] = uart_raw_ticks[i];
        }
        local_boot_id = uart_boot_id;
        last_rx = last_valid_frame_time;
        portEXIT_CRITICAL(&uartMux);

        // 2. Manejo del Paro de Emergencia por Comando
        if (request_estop) {
            current_state = STATE_ESTOP;
            request_estop = false;
        }

        // 3. Monitoreo de timeout UART (50 ms)
        if (millis() - last_rx > 50) {
            current_state = STATE_ESTOP;
        }

        // 4. Pasar ticks crudos a las instancias de controlador
        for (int i = 0; i < 8; i++) {
            motors[i].setRawTicks(local_raw[i]);
        }

        // 5. Ejecutar máquina de estados de control y STBY físico
        if (current_state == STATE_ESTOP) {
#if !SIMULATION_MODE
            digitalWrite(STBY_PIN, LOW); // Apagado instantáneo de puentes H
#endif
            for (int i = 0; i < 8; i++) {
                // Mantener posición actual como target
                motors[i].setTargetTicks(motors[i].getCurrentTicks());
                motors[i].update(dt);
            }
        } else {
#if !SIMULATION_MODE
            digitalWrite(STBY_PIN, HIGH); // Habilitar puentes H
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
                        
                        // Guardar BOOT_ID inicial y aplicar offset local
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
                    // Validación de protección de reinicio del ENCODER
                    if (boot_id_initialized && (local_boot_id != active_boot_id)) {
                        current_state = STATE_ESTOP;
                        boot_id_initialized = false; 
                        Serial.println("[ALERTA CRÍTICA] Se detectó reinicio del ESP32 ENCODER. E-STOP forzado.");
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

        // 6. Escribir el registro consolidado de direcciones I2C (MCP23017)
#if !SIMULATION_MODE
        if (USE_MCP23017) {
            Wire.beginTransmission(MCP23017_ADDR);
            Wire.write(0x12); // Registro GPIOA del MCP23017
            Wire.write(mcp_direction_register);
            Wire.endTransmission();
        }
#endif

        // 7. Si SIMULATION_MODE = 1, enviar PWMs calculados de vuelta al ENCODER
#if SIMULATION_MODE
        transmitPWMToEncoder();
#endif

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
    Serial.println("UART2 de interconexión con ESP32 ENCODER inicializado.");

#if !SIMULATION_MODE
    if (USE_MCP23017) {
        // Inicializar puerto I2C para el expansor MCP23017
        Wire.begin(I2C_SDA, I2C_SCL, 400000); // Fast mode 400 kHz
        
        // Configurar MCP23017 pines de puerto A (GP0-GP7) como salidas
        Wire.beginTransmission(MCP23017_ADDR);
        Wire.write(0x00); // Dirección IODIRA
        Wire.write(0x00); // Todo salidas
        Wire.endTransmission();
        Serial.println("I2C MCP23017 inicializado.");
    } else {
        Serial.println("Modo GPIO directo para dirección de motores activo.");
    }
    
    // Inicializar pin de desactivación de drivers TB6612FNG (Standby)
    pinMode(STBY_PIN, OUTPUT);
    digitalWrite(STBY_PIN, HIGH); // Puentes H activos por defecto
    Serial.println("Pin Standby de hardware inicializado.");
#endif

    // Inicializar motores
    for (int i = 0; i < 8; i++) {
        motors[i].begin(i);
    }

#if !SIMULATION_MODE
    Serial.println("Motores configurados por hardware.");
#else
    Serial.println("MODO SIMULACIÓN ACTIVO: Los motores y encoders se simulan en la placa ENCODER.");
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