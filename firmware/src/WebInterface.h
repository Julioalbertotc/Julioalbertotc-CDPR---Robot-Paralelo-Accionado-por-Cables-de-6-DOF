#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "Config.h"
#include "SystemState.h"

// Variables compartidas con main.cpp
extern SystemState current_state;
extern float target_x, target_y, target_z;
extern float target_roll, target_pitch, target_yaw;
extern bool request_homing;
extern bool request_center;
extern bool request_estop;

// Objetos del servidor web
static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

// Función de procesamiento de comandos recibidos por WebSocket
inline void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, (char*)data);
        if (error) {
            Serial.print("Error deserializando JSON de WebSocket: ");
            Serial.println(error.c_str());
            return;
        }

        const char* cmd = doc["cmd"];
        if (cmd != nullptr) {
            if (strcmp(cmd, "pose") == 0) {
                // Solo aceptar nuevas poses si el sistema está listo (homing completado)
                if (current_state == STATE_READY) {
                    target_x = doc["x"] | target_x;
                    target_y = doc["y"] | target_y;
                    target_z = doc["z"] | target_z;
                    target_roll = doc["r"] | target_roll;
                    target_pitch = doc["p"] | target_pitch;
                    target_yaw = doc["yaw"] | target_yaw;
                }
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

// Evento principal del WebSocket
inline void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                    void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("Cliente WebSocket conectado desde: %s\n", client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.println("Cliente WebSocket desconectado");
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

// Inicializar la red WiFi en modo AP, LittleFS y arrancar servidores
inline void initWebInterface() {
    // 1. Inicializar LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("¡Error inicializando LittleFS! Formateando...");
    } else {
        Serial.println("LittleFS inicializado correctamente.");
    }

    // 2. Configurar modo Access Point (AP)
    //WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, 0, AP_MAX_CONN);
    //IPAddress IP = WiFi.softAPIP();
    //Serial.print("Punto de Acceso WiFi creado. SSID: ");
    //Serial.println(AP_SSID);
    //Serial.print("Dirección IP del ESP32: ");
    //Serial.println(IP);

    // 3. Configurar rutas estáticas
    // Servir index.html por defecto
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/index.html", "text/html");
    });
    
    // Servir style.css y main.js de forma directa
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/style.css", "text/css");
    });
    
    server.on("/main.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/main.js", "application/javascript");
    });

    // 4. Configurar WebSocket
    ws.onEvent(onEvent);
    server.addHandler(&ws);

    // 5. Iniciar Servidor Web
    server.begin();
    Serial.println("Servidor Web HTTP iniciado en puerto 80.");
}

// Enviar telemetría a todos los clientes WebSocket conectados
// Se llamará periódicamente desde el bucle secundario
inline void broadcastTelemetry(float cur_x, float cur_y, float cur_z,
                               float cur_roll, float cur_pitch, float cur_yaw,
                               float lengths[8], float target_lengths[8],
                               int32_t ticks[8], int32_t target_ticks[8],
                               int pwms[8]) {
    if (ws.count() == 0) return; // No hay clientes conectados

    JsonDocument doc;
    doc["state"] = (int)current_state;
    
    JsonObject pos = doc.createNestedObject("pose");
    pos["x"] = cur_x;
    pos["y"] = cur_y;
    pos["z"] = cur_z;
    pos["r"] = cur_roll;
    pos["p"] = cur_pitch;
    pos["yaw"] = cur_yaw;
    
    JsonObject t_pos = doc.createNestedObject("target_pose");
    t_pos["x"] = target_x;
    t_pos["y"] = target_y;
    t_pos["z"] = target_z;
    t_pos["r"] = target_roll;
    t_pos["p"] = target_pitch;
    t_pos["yaw"] = target_yaw;

    JsonArray lenArr = doc.createNestedArray("lengths");
    JsonArray tLenArr = doc.createNestedArray("target_lengths");
    JsonArray tickArr = doc.createNestedArray("ticks");
    JsonArray tTickArr = doc.createNestedArray("target_ticks");
    JsonArray pwmArr = doc.createNestedArray("pwms");

    for (int i = 0; i < 8; i++) {
        lenArr.add(lengths[i]);
        tLenArr.add(target_lengths[i]);
        tickArr.add(ticks[i]);
        tTickArr.add(target_ticks[i]);
        pwmArr.add(pwms[i]);
    }

    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}

#endif // WEB_INTERFACE_H
