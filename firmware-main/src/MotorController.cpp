#include "MotorController.h"
#include "Globals.h"

MotorController::MotorController() 
    : id(-1), raw_ticks(0), tick_offset(0), target_ticks(0),
      integral(0.0f), last_error(0.0f), pwm_output(0.0f) {
}

void MotorController::begin(int motor_id) {
    id = motor_id;
    pins = MOTOR_PINS[motor_id];
    
#if !SIMULATION_MODE
    // Configurar PWM por hardware usando LEDC
    ledcAttach(pins.pwm, 20000, 8); // Pin, 20 kHz, 8 bits
    
    // Si no se usa MCP23017, inicializar los pines directos de dirección
    if (!USE_MCP23017) {
        pinMode(pins.dir_gpio, OUTPUT);
        digitalWrite(pins.dir_gpio, LOW);
    }
#endif

    raw_ticks = 0;
    tick_offset = 0;
    target_ticks = 0;
    integral = 0.0f;
    last_error = 0.0f;
    pwm_output = 0.0f;
}

void MotorController::setRawTicks(int32_t ticks) {
    raw_ticks = ticks;
}

void MotorController::setTargetPosition(float length_cm) {
    setTargetTicks((int32_t)(length_cm * TICKS_PER_CM));
}

void MotorController::setTargetTicks(int32_t ticks) {
    target_ticks = ticks;
}

void MotorController::resetEncoderLocal() {
    tick_offset = raw_ticks;
}

int32_t MotorController::getCurrentTicks() {
    return raw_ticks - tick_offset;
}

float MotorController::getCurrentLengthCm() {
    return (float)getCurrentTicks() / TICKS_PER_CM;
}

void MotorController::update(float dt) {
    // Lazo PID de Posición
    float error = (float)(target_ticks - getCurrentTicks());
    
    // Término Integral con anti-windup
    integral += error * dt;
    integral = constrain(integral, -PID_INTEGRAL_LIMIT, PID_INTEGRAL_LIMIT);
    
    // Término Derivativo
    float derivative = (error - last_error) / dt;
    last_error = error;
    
    // Señal PID
    float pid_signal = (PID_KP * error) + (PID_KI * integral) + (PID_KD * derivative);
    
    // LÓGICA DE TORQUE DE RETENCIÓN ACTIVO (ANTI-HOLGURA)
    if (pid_signal > 0.0f) {
        if (pid_signal < (float)HOLDING_TORQUE_PWM) {
            pid_signal = (float)HOLDING_TORQUE_PWM;
        }
    } else {
        if (abs(error) < 50) { // Cerca del target (aprox 1.5mm)
            pid_signal = (float)HOLDING_TORQUE_PWM;
        }
    }

    // Limitador de PWM
    pwm_output = constrain(pid_signal, PID_MIN_PWM, PID_MAX_PWM);

#if !SIMULATION_MODE
    int pwm_val = (int)abs(pwm_output);
    ledcWrite(pins.pwm, pwm_val);
    
    // Actualizar registro de dirección
    if (USE_MCP23017) {
        if (pwm_output >= 0.0f) {
            mcp_direction_register |= (1 << pins.dir_mcp);
        } else {
            mcp_direction_register &= ~(1 << pins.dir_mcp);
        }
    } else {
        digitalWrite(pins.dir_gpio, pwm_output >= 0.0f ? HIGH : LOW);
    }
#endif
}
