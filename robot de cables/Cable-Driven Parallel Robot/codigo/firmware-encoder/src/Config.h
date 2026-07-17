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
// ASIGNACIÓN DE PINES DE ENCODERS (PCNT)
// =============================================================================
struct EncoderPins {
    int encA;
    int encB;
    int pcnt_unit; // Unidad PCNT (0 a 7 en ESP32)
};

const EncoderPins ENCODER_PINS[8] = {
    { 34, 35, 0 }, // Motor 0
    { 36, 39, 1 }, // Motor 1
    { 32, 33, 2 }, // Motor 2
    { 25, 26, 3 }, // Motor 3
    { 27, 13, 4 }, // Motor 4
    { 23, 19, 5 }, // Motor 5
    {  4, 15, 6 }, // Motor 6
    { 18, 14, 7 }  // Motor 7 (Usamos pines libres de strapping y flash)
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
