#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =============================================================================
// MODO SIMULACIÓN Y CONFIGURACIÓN GENERAL
// =============================================================================
#ifndef SIMULATION_MODE
#define SIMULATION_MODE 1
#endif

#define SERIAL_BAUD 115200
#define CONTROL_LOOP_HZ 1000 // 1 kHz
#define INTERCONNECT_BAUD 921600

// =============================================================================
// INTERCONEXIÓN UART (Hacia el ESP32 ENCODER)
// =============================================================================
#define UART_RX_PIN 16
#define UART_TX_PIN 17

// =============================================================================
// RED WI-FI (MODO PUNTO DE ACCESO - AP)
// =============================================================================
#define AP_SSID "CDPR_Cable_Robot"
#define AP_PASSWORD "12345678"
#define AP_CHANNEL 1
#define AP_MAX_CONN 4

// =============================================================================
// DIRECCIÓN I2C Y PINES DE EXPANSOR / SEGURIDAD
// =============================================================================
#define USE_MCP23017 1 // 1 para usar el expansor I2C MCP23017, 0 para usar GPIOs directos

#define I2C_SDA 21
#define I2C_SCL 22
#define MCP23017_ADDR 0x20

// Pin de hardware real para standby de los 4 TB6612FNG
#define STBY_PIN 5 // Se tira a LOW en ESTOP para apagar los puentes H en hardware

// =============================================================================
// GEOMETRÍA DEL ROBOT (Unidades en centímetros)
// =============================================================================
struct Vector3D {
    float x;
    float y;
    float z;
};

const Vector3D POLE_POSITIONS[8] = {
    {-22.5f, -21.0f, 45.0f}, // Cable 0 (Esquina 1, Polea A)
    {-21.0f, -22.5f, 45.0f}, // Cable 1 (Esquina 1, Polea B)
    { 21.0f, -22.5f, 45.0f}, // Cable 2 (Esquina 2, Polea A)
    { 22.5f, -21.0f, 45.0f}, // Cable 3 (Esquina 2, Polea B)
    { 22.5f,  21.0f, 45.0f}, // Cable 4 (Esquina 3, Polea A)
    { 21.0f,  22.5f, 45.0f}, // Cable 5 (Esquina 3, Polea B)
    {-21.0f,  22.5f, 45.0f}, // Cable 6 (Esquina 4, Polea A)
    {-22.5f,  21.0f, 45.0f}  // Cable 7 (Esquina 4, Polea B)
};

const Vector3D ANCHOR_POSITIONS[8] = {
    {-5.0f, -4.0f, 0.0f}, // Cable 0
    {-4.0f, -5.0f, 0.0f}, // Cable 1
    { 4.0f, -5.0f, 0.0f}, // Cable 2
    { 5.0f, -4.0f, 0.0f}, // Cable 3
    { 5.0f,  4.0f, 0.0f}, // Cable 4
    { 4.0f,  5.0f, 0.0f}, // Cable 5
    {-4.0f,  5.0f, 0.0f}, // Cable 6
    {-5.0f,  4.0f, 0.0f}  // Cable 7
};

const Vector3D DEFAULT_START_POSE_POS = {0.0f, 0.0f, 22.5f};
const Vector3D DEFAULT_START_POSE_ROT = {0.0f, 0.0f, 0.0f};

// =============================================================================
// ASIGNACIÓN DE PINES GPIO (ESP32 MAIN)
// =============================================================================
// Cada motor tiene:
// - pwm: Pin de PWM directo del ESP32 (8 pines independientes).
// - dir_mcp: Índice del bit en el MCP23017 (0 a 7).
// - dir_gpio: Pin GPIO de dirección directo en el ESP32 (cuando USE_MCP23017 = 0).
struct MotorPins {
    uint8_t pwm;
    uint8_t dir_mcp;
    uint8_t dir_gpio;
};

const MotorPins MOTOR_PINS[8] = {
    // Motor, PWM Pin, Bit MCP, GPIO Dirección (si no se usa MCP)
    { 2,  0, 4 },
    { 12, 1, 13 },
    { 14, 2, 15 },
    { 23, 3, 26 }, // Modificado para evitar GPIO 16 (UART RX2)
    { 27, 4, 32 }, // Modificado para evitar GPIO 17 (UART TX2)
    { 18, 5, 33 },
    { 19, 6, 21 }, // Si no se usa MCP, podemos usar pines I2C para DIR
    { 25, 7, 22 }
};

// =============================================================================
// PARÁMETROS DE LOS MOTORES Y REDUCCIÓN
// =============================================================================
const float GEAR_RATIO = 45.0f;
const float ENCODER_PPR = 11.0f;
const float DRUM_RADIUS_CM = 1.0f;
const float TICKS_PER_REV = ENCODER_PPR * 4.0f * GEAR_RATIO;
const float TICKS_PER_CM = TICKS_PER_REV / (2.0f * PI * DRUM_RADIUS_CM);

// Constantes físicas del motor DC para simulación interna
const float SIM_MOTOR_R = 7.5f;
const float SIM_MOTOR_L = 0.05f;
const float SIM_MOTOR_KT = 0.08f;
const float SIM_MOTOR_KE = 0.08f;
const float SIM_MOTOR_J = 0.0001f;
const float SIM_MOTOR_B = 0.0002f;
const float SIM_VOLTAGE = 12.0f;

// =============================================================================
// PARÁMETROS PID
// =============================================================================
const float PID_KP = 8.5f;
const float PID_KI = 0.1f;
const float PID_KD = 0.4f;
const float PID_MAX_PWM = 255.0f;
const float PID_MIN_PWM = -255.0f;
const float PID_INTEGRAL_LIMIT = 50.0f;

// Pre-tensión en reposo
const int HOLDING_TORQUE_PWM = 25;

#endif // CONFIG_H
