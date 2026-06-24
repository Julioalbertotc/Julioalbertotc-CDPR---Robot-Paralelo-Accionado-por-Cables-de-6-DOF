#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include "Config.h"

// Registro global de salida del expansor I2C MCP23017 para ruteo rápido
extern uint8_t mcp_direction_register;

class MotorController {
public:
    MotorController();
    
    // Inicializa la configuración de pines y contadores hardware (PCNT)
    void begin(int motor_id);
    
    // Actualiza el control PID y simulación
    void update(float dt);
    
    void setTargetPosition(float length_cm);
    void setTargetTicks(int32_t ticks);
    
    int32_t getCurrentTicks();
    float getCurrentLengthCm();
    int getPWM() const { return (int)pwm_output; }
    int32_t getTargetTicks() const { return target_ticks; }
    
    void resetEncoder();
    void simulateStep(float dt);

private:
    int id;
    MotorPins pins;
    
    int32_t current_ticks;
    int32_t target_ticks;
    int32_t last_ticks;
    
    float integral;
    float last_error;
    float pwm_output;
    
    // Simulación
    float sim_current;
    float sim_omega;
    float sim_angle;
};

#endif // MOTOR_CONTROLLER_H
