import os
import math
import matplotlib.pyplot as plt

class DCMotorSim:
    def __init__(self):
        # Constantes del motor (coincidentes con Config.h)
        self.R = 7.5       # Ohms
        self.L = 0.05      # Henrys
        self.Kt = 0.08     # Nm/A
        self.Ke = 0.08     # V/(rad/s)
        self.J = 0.0001    # Kg*m^2 (Inercia)
        self.b = 0.0002    # Nms (Fricción viscosa)
        
        self.gear_ratio = 45.0
        self.encoder_ppr = 11.0
        self.ticks_per_rev = self.encoder_ppr * 4.0 * self.gear_ratio  # 1980
        self.drum_radius = 1.0  # cm
        self.ticks_per_cm = self.ticks_per_rev / (2.0 * math.pi * self.drum_radius) # 315.127
        
        # Estados
        self.current = 0.0  # A
        self.omega = 0.0    # rad/s
        self.angle = 0.0    # rad
        self.ticks = 0
        
        # Carga estática simulada (tensión que jala del cable)
        self.tension_load = 0.005 # Nm

    def step(self, pwm, dt):
        # Voltaje aplicado
        v_in = (pwm / 255.0) * 12.0
        
        # Ecuaciones diferenciales (Integración de Euler)
        di = (v_in - self.R * self.current - self.Ke * self.omega) / self.L
        self.current += di * dt
        
        motor_torque = self.Kt * self.current
        total_torque = motor_torque - self.tension_load
        
        d_omega = (total_torque - self.b * self.omega) / self.J
        self.omega += d_omega * dt
        
        self.angle += self.omega * dt
        self.ticks = int(self.angle * self.ticks_per_rev / (2.0 * math.pi))
        
        return self.ticks

class PIDController:
    def __init__(self, kp, ki, kd):
        self.kp = kp
        self.ki = ki
        self.kd = kd
        
        self.integral = 0.0
        self.last_error = 0.0
        self.integral_limit = 50.0

    def compute(self, target, current, dt):
        error = target - current
        
        # Integral con anti-windup
        self.integral += error * dt
        self.integral = max(min(self.integral, self.integral_limit), -self.integral_limit)
        
        # Derivativa
        derivative = (error - self.last_error) / dt
        self.last_error = error
        
        # Control bruto
        signal = (self.kp * error) + (self.ki * self.integral) + (self.kd * derivative)
        
        # Limitador
        pwm = max(min(signal, 255.0), -255.0)
        return pwm

def run_step_response_test():
    """
    Simula una respuesta al escalón del motor para pre-sintonizar el PID.
    """
    motor = DCMotorSim()
    # Usar ganancias por defecto de Config.h
    pid = PIDController(kp=8.5, ki=0.1, kd=0.4)
    
    dt = 0.001  # 1 ms
    duration = 1.0  # 1 segundo
    steps = int(duration / dt)
    
    # Consigna: mover el cable a una longitud de 5.0 cm (equivalente a aprox 1575 ticks)
    target_cm = 5.0
    target_ticks = int(target_cm * motor.ticks_per_cm)
    
    time_history = []
    ticks_history = []
    target_history = []
    pwm_history = []
    
    for step in range(steps):
        t = step * dt
        current_ticks = motor.ticks
        
        # Lazo cerrado
        pwm = pid.compute(target_ticks, current_ticks, dt)
        
        # Aplicar holding torque en simulación (mantener tensión mínima activa si el error es pequeño)
        if abs(target_ticks - current_ticks) < 50:
            if pwm > 0 and pwm < 25:
                pwm = 25
        
        # Avance del motor
        motor.step(pwm, dt)
        
        time_history.append(t)
        ticks_history.append(current_ticks / motor.ticks_per_cm) # Convertir a cm para visualización
        target_history.append(target_cm)
        pwm_history.append(pwm)
        
    # Graficar resultados
    plt.figure(figsize=(10, 6))
    
    plt.subplot(2, 1, 1)
    plt.plot(time_history, target_history, 'r--', label='Consigna (cm)')
    plt.plot(time_history, ticks_history, 'b-', label='Respuesta Real (cm)')
    plt.title('Simulación de Lazo Cerrado CDPR - PID Posición del Motor')
    plt.ylabel('Longitud del Cable (cm)')
    plt.grid(True)
    plt.legend()
    
    plt.subplot(2, 1, 2)
    plt.plot(time_history, pwm_history, 'g-', label='Señal de Control (PWM)')
    plt.xlabel('Tiempo (s)')
    plt.ylabel('PWM (-255 a 255)')
    plt.grid(True)
    plt.legend()
    
    plt.tight_layout()
    
    output_filename = "pid_tuning_step.png"
    plt.savefig(output_filename)
    print(f"Simulación de sintonización completada. Gráfica guardada como: {os.path.abspath(output_filename)}")

if __name__ == "__main__":
    run_step_response_test()
