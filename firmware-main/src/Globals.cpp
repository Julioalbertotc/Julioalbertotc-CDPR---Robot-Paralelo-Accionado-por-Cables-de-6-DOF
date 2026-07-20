#include "Globals.h"

// Servidor Web y WebSockets
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Inicialización de variables globales y control de estado
SystemState current_state = STATE_UNHOMED;

// Poses objetivo (consignas)
float target_x = DEFAULT_START_POSE_POS.x;
float target_y = DEFAULT_START_POSE_POS.y;
float target_z = DEFAULT_START_POSE_POS.z;
float target_roll = DEFAULT_START_POSE_ROT.x;
float target_pitch = DEFAULT_START_POSE_ROT.y;
float target_yaw = DEFAULT_START_POSE_ROT.z;

// Poses estimadas actuales (basadas en encoders)
float current_x = DEFAULT_START_POSE_POS.x;
float current_y = DEFAULT_START_POSE_POS.y;
float current_z = DEFAULT_START_POSE_POS.z;
float current_roll = DEFAULT_START_POSE_ROT.x;
float current_pitch = DEFAULT_START_POSE_ROT.y;
float current_yaw = DEFAULT_START_POSE_ROT.z;

// Flags de solicitudes de comandos (WebSockets/Serial)
bool request_homing = false;
bool request_center = false;
bool request_estop = false;

// Instancia de los 8 controladores de motor
MotorController motors[8];
