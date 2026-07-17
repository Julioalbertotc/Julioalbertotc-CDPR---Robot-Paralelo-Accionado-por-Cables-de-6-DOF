#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include "Config.h"

class MotorController {
public:
    MotorController();
    
    // Inicializa la configuración de pines (PWM y Dirección)
    void begin(int motor_id);
    
    // Actualiza el lazo PID de posición
    void update(float dt);
    
    void setTargetPosition(float length_cm);
    void setTargetTicks(int32_t ticks);
    
    // Métodos para actualizar los ticks recibidos por UART y gestionar offsets locales
    void setRawTicks(int32_t ticks);
    void resetEncoderLocal();
    
    int32_t getCurrentTicks();
    float getCurrentLengthCm();
    int getPWM() const { return (int)pwm_output; }
    int32_t getTargetTicks() const { return target_ticks; }
    
private:
    int id;
    MotorPins pins;
    
    int32_t raw_ticks;      // Valor crudo recibido del ESP32 ENCODER
    int32_t tick_offset;    // Offset lógico guardado durante el homing
    int32_t target_ticks;   // Consigna objetivo
    
    float integral;
    float last_error;
    float pwm_output;
};

#endif // MOTOR_CONTROLLER_H
