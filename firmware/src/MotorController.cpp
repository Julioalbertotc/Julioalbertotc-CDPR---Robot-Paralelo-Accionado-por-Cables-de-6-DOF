#include "MotorController.h"

#if !SIMULATION_MODE
#include "driver/pcnt.h"
#endif

// Definición de variable global del registro de dirección
uint8_t mcp_direction_register = 0;

MotorController::MotorController() 
    : id(-1), current_ticks(0), target_ticks(0), last_ticks(0),
      integral(0.0f), last_error(0.0f), pwm_output(0.0f),
      sim_current(0.0f), sim_omega(0.0f), sim_angle(0.0f) {
}

void MotorController::begin(int motor_id) {
    id = motor_id;
    pins = MOTOR_PINS[motor_id];
    
#if !SIMULATION_MODE
    // Configurar PWM por hardware usando el periférico LEDC de ESP32
    ledcSetup(id, 20000, 8); // Canal, Frecuencia 20 kHz, Resolución 8 bits
    ledcAttachPin(pins.pwm, id);
    
    // Configurar e inicializar hardware del contador de pulsos (PCNT)
    // El ESP32 tiene 8 unidades PCNT independientes (0 a 7), perfecto para 8 encoders.
    pcnt_config_t pcnt_config;
    pcnt_config.pulse_gpio_num = pins.encA;
    pcnt_config.ctrl_gpio_num = pins.encB;
    pcnt_config.channel = PCNT_CHANNEL_0;
    pcnt_config.unit = (pcnt_unit_t)pins.pcnt_unit;
    
    // Contar flanco de subida y bajada para decodificar cuadratura
    pcnt_config.pos_mode = PCNT_COUNT_INC;  // Cuenta arriba en flanco de subida de encA
    pcnt_config.neg_mode = PCNT_COUNT_DEC;  // Cuenta abajo en flanco de bajada de encA
    
    // Modo de control de dirección según el nivel lógico del canal B
    pcnt_config.lctrl_mode = PCNT_MODE_KEEP;    // Mantener dirección si B es LOW
    pcnt_config.hctrl_mode = PCNT_MODE_REVERSE; // Reversar dirección si B es HIGH
    
    pcnt_config.counter_h_lim = 32767;
    pcnt_config.counter_l_lim = -32768;
    
    pcnt_unit_config(&pcnt_config);
    
    // Limpiar y encender el contador hardware
    pcnt_counter_pause((pcnt_unit_t)pins.pcnt_unit);
    pcnt_counter_clear((pcnt_unit_t)pins.pcnt_unit);
    pcnt_counter_resume((pcnt_unit_t)pins.pcnt_unit);
#endif
}

void MotorController::resetEncoder() {
    current_ticks = 0;
    last_ticks = 0;
    sim_angle = 0.0f;
    sim_omega = 0.0f;
    sim_current = 0.0f;
#if !SIMULATION_MODE
    pcnt_counter_clear((pcnt_unit_t)pins.pcnt_unit);
#endif
}

void MotorController::setTargetPosition(float length_cm) {
    setTargetTicks((int32_t)(length_cm * TICKS_PER_CM));
}

void MotorController::setTargetTicks(int32_t ticks) {
    target_ticks = ticks;
}

int32_t MotorController::getCurrentTicks() {
#if SIMULATION_MODE
    return current_ticks;
#else
    int16_t raw_count = 0;
    pcnt_get_counter_value((pcnt_unit_t)pins.pcnt_unit, &raw_count);
    current_ticks = raw_count;
    return current_ticks;
#endif
}

float MotorController::getCurrentLengthCm() {
    return (float)getCurrentTicks() / TICKS_PER_CM;
}

void MotorController::update(float dt) {
#if SIMULATION_MODE
    simulateStep(dt);
#endif

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
    ledcWrite(id, pwm_val);
    
    // Actualizar registro de dirección en expansor I2C
    if (pwm_output >= 0.0f) {
        mcp_direction_register |= (1 << pins.dir_mcp);
    } else {
        mcp_direction_register &= ~(1 << pins.dir_mcp);
    }
#endif
}

void MotorController::simulateStep(float dt) {
    float v_in = (pwm_output / 255.0f) * SIM_VOLTAGE;
    
    float di = (v_in - SIM_MOTOR_R * sim_current - SIM_MOTOR_KE * sim_omega) / SIM_MOTOR_L;
    sim_current += di * dt;
    
    float torque = SIM_MOTOR_KT * sim_current;
    float tension_load = 0.005f; // Nm
    float total_torque = torque - tension_load; 
    
    float d_omega = (total_torque - SIM_MOTOR_B * sim_omega) / SIM_MOTOR_J;
    sim_omega += d_omega * dt;
    
    sim_angle += sim_omega * dt;
    current_ticks = (int32_t)(sim_angle * TICKS_PER_REV / (2.0f * PI));
}
