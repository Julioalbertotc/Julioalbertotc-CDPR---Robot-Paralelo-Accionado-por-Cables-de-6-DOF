#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "SystemState.h"
#include "MotorController.h"

// Servidor Web y WebSockets
extern AsyncWebServer server;
extern AsyncWebSocket ws;

// Estado del sistema
extern SystemState current_state;

// Poses objetivo (consignas)
extern float target_x;
extern float target_y;
extern float target_z;
extern float target_roll;
extern float target_pitch;
extern float target_yaw;

// Poses estimadas actuales
extern float current_x;
extern float current_y;
extern float current_z;
extern float current_roll;
extern float current_pitch;
extern float current_yaw;

// Flags de solicitudes de comandos
extern bool request_homing;
extern bool request_center;
extern bool request_estop;

// Instancia de los 8 controladores de motor
extern MotorController motors[8];

#endif // GLOBALS_H
