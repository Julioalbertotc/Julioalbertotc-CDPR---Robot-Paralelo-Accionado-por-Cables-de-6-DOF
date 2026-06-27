#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <WiFi.h> // <-- LIBRERÍA DE WIFI AÑADIDA
#include "Config.h"
#include "SystemState.h"
#include "Kinematics.h"
#include "MotorController.h"
#include "Globals.h"
#include "WebInterface.h"

// =============================================================================
// NÚCLEO 2: TAREA DE CONTROL DE LAZO CERRADO (1 kHz)
// =============================================================================
void controlLoopTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1000 / CONTROL_LOOP_HZ); // 1 ms

    uint32_t homing_timer = 0;

    while (true) {
        float dt = 1.0f / (float)CONTROL_LOOP_HZ;

        // 1. Manejo del Paro de Emergencia
        if (request_estop) {
            current_state = STATE_ESTOP;
            request_estop = false;
        }

        // 2. Ejecutar máquina de estados de control y STBY físico
        if (current_state == STATE_ESTOP) {
#if !SIMULATION_MODE
            digitalWrite(STBY_PIN, LOW); // Apagado instantáneo de puentes H por hardware
#endif
            for (int i = 0; i < 8; i++) {
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
                        
                        for (int i = 0; i < 8; i++) {
                            motors[i].resetEncoder();
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

        // 3. Escribir el registro consolidado de direcciones I2C (MCP23017)
#if !SIMULATION_MODE
        Wire.beginTransmission(MCP23017_ADDR);
        Wire.write(0x12); // Registro GPIOA del MCP23017
        Wire.write(mcp_direction_register);
        Wire.endTransmission();
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
    Serial.println("\n--- CDPR CABLE ROBOT FIRMWARE ---");

#if !SIMULATION_MODE
    // Inicializar puerto I2C para el expansor MCP23017
    Wire.begin(I2C_SDA, I2C_SCL, 400000); // Fast mode 400 kHz
    
    // Configurar MCP23017 pines de puerto A (GP0-GP7) como salidas
    Wire.beginTransmission(MCP23017_ADDR);
    Wire.write(0x00); // Dirección IODIRA
    Wire.write(0x00); // Todo salidas
    Wire.endTransmission();
    
    // Inicializar pin de desactivación de drivers TB6612FNG (Standby)
    pinMode(STBY_PIN, OUTPUT);
    digitalWrite(STBY_PIN, HIGH); // Puentes H activos por defecto
    Serial.println("I2C MCP23017 y Pin Standby de hardware inicializados.");
#endif

    // Inicializar motores
    for (int i = 0; i < 8; i++) {
        motors[i].begin(i);
    }

#if !SIMULATION_MODE
    Serial.println("Periféricos hardware PCNT de ESP32 asignados a los 8 encoders.");
#else
    Serial.println("MODO SIMULACIÓN ACTIVO: Los encoders y motores se simulan en software.");
#endif

    // =============================================================================
    // INICIALIZACIÓN DE WI-FI (MODO PUNTO DE ACCESO - AP)
    // =============================================================================
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
    // IMPORTANTE: Asegúrate de borrar WiFi.softAP() dentro de esta función
    initWebInterface();

    // Crear tarea en Core 0
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

    // 1. Procesar entrada Serial JSON de la PC
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

    // 2. Transmitir telemetría periódica a WebSockets y Serial (20 Hz = 50 ms)
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
    delay(5);
}