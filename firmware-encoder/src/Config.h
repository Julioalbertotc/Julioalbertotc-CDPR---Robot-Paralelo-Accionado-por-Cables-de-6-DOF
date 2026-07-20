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
#define INTERCONNECT_BAUD 921600

// =============================================================================
// ASIGNACIÓN DE PINES UART DE INTERCONEXIÓN
// =============================================================================
#define UART_RX_PIN 16
#define UART_TX_PIN 17

// =============================================================================
// PIN DE SEGURIDAD LOCAL (ESP32 SUB)
// =============================================================================
#define STBY_PIN 5 // Habilitación física de drivers TB6612 locales en hardware

// =============================================================================
// ASIGNACIÓN DE PINES DE MOTORES Y ENCODERS LOCALES (MOTORES 4 A 7)
// =============================================================================
struct MotorPins {
    bool is_local;       // True si el motor se controla/lee en esta placa
    uint8_t pwm;         // Pin de PWM
    uint8_t dir1;        // Pin IN1
    uint8_t dir2;        // Pin IN2
    uint8_t encA;        // Pin Encoder Canal A
    uint8_t encB;        // Pin Encoder Canal B
    int pcnt_unit;       // Unidad PCNT
};

const MotorPins MOTOR_PINS[8] = {
    // Para Motores 0-3 locales en MAIN, colocamos is_local=false en SUB.
    { false, 0,  0,  0,  0,  0, -1 },  // Motor 0
    { false, 0,  0,  0,  0,  0, -1 },  // Motor 1
    { false, 0,  0,  0,  0,  0, -1 },  // Motor 2
    { false, 0,  0,  0,  0,  0, -1 },  // Motor 3
    // Motores 4-7 locales en SUB.
    { true,  2,  4, 15, 34, 35,  0 },  // Motor 4 (Local M4)
    { true, 12, 13, 14, 36, 39,  1 },  // Motor 5 (Local M5)
    { true, 25, 26, 27, 32, 33,  2 },  // Motor 6 (Local M6)
    { true, 18, 19, 23, 21, 22,  3 }   // Motor 7 (Local M7)
};

// =============================================================================
// PARÁMETROS DE LOS MOTORES Y REDUCCIÓN (Para Simulación)
// =============================================================================
const float GEAR_RATIO = 45.0f;
const float ENCODER_PPR = 11.0f;
const float DRUM_RADIUS_CM = 1.0f;
const float TICKS_PER_REV = ENCODER_PPR * 4.0f * GEAR_RATIO;

// Constantes físicas del motor DC para simulación
const float SIM_MOTOR_R = 7.5f;
const float SIM_MOTOR_L = 0.05f;
const float SIM_MOTOR_KT = 0.08f;
const float SIM_MOTOR_KE = 0.08f;
const float SIM_MOTOR_J = 0.0001f;
const float SIM_MOTOR_B = 0.0002f;
const float SIM_VOLTAGE = 12.0f;

#endif // CONFIG_H
