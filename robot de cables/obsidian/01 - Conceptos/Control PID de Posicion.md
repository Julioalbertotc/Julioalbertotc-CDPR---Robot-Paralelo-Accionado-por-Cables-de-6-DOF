---
tags: [proyecto/cdpr, tipo/concepto]
estado: validado
creado: 2026-07-16
---
relacionado: [[00 - MOC CDPR]], [[Cinematica Inversa CDPR]], [[Esquema de Conexiones y Pines]], [[Decisiones de Diseño]]
---

# Control PID de Posición

## Definición
Algoritmo de control realimentado que calcula de manera continua la señal PWM de control aplicada a cada uno de los 8 motores DC reductores basándose en el error de posición en ticks de encoder.

---

## Formulación del Controlador

Para cada motor, la salida de control $u(t)$ en el instante $t$ se define como:
$$
u(t) = K_p \, e(t) + K_i \int_{0}^{t} e(\tau) d\tau + K_d \, \frac{de(t)}{dt}
$$
Donde el error de posición $e(t)$ es:
$$
e(t) = \text{target\_ticks} - \text{current\_ticks}
$$

### Constantes de Sintonización
- **$K_p$ (Proporcional)** = $8.5$
- **$K_i$ (Integral)** = $0.1$
- **$K_d$ (Derivativo)** = $0.4$

---

## Características de Diseño Especiales

1. **Anti-Windup**: 
   La acumulación del término integral se restringe a un límite estricto utilizando la función `constrain` en el rango de $[-50.0, 50.0]$ para prevenir saturación prolongada.
   
   > [!WARNING]
   > **Análisis de Límites de Integral**:
   > Con un límite de integral de $\pm 50.0$ y un coeficiente $K_i = 0.1$, la contribución máxima del término integral a la salida de control PWM es de apenas $\pm 5.0$ (sobre un rango de $\pm 255$). En presencia de cargas gravitatorias constantes u oposición friccional alta, esta saturación podría ser demasiado baja, induciendo un error persistente en estado estacionario.
   
2. **Torque de Retención Activo (Anti-Holgura)**:
   Puesto que los cables no pueden empujar (sólo jalar), se requiere mantener una tensión constante mínima en todo momento.
   - Si la señal calculada por el PID $u(t) > 0$ pero es menor que `HOLDING_TORQUE_PWM` ($25$), se fuerza a $25$.
   - Si la plataforma está muy cerca del objetivo (error $< 50$ ticks, aprox $1.5\text{ mm}$), se aplica una consigna de retención de $25$ PWM para mantener el cable tenso y evitar la flacidez del cable.
3. **Freno Activo (Short-Brake)**:
   Cuando la consigna $u(t)$ es exactamente $0$, los pines del expansor MCP23017 se configuran a `IN1 = LOW, IN2 = LOW` provocando el cortocircuito del puente H de los TB6612FNG para bloquear dinámicamente el motor.

---

## Conversión Ticks a Centímetros
El sistema utiliza las siguientes constantes físicas para convertir de distancias espaciales (cinemática) a ticks de encoder:
- Relación de Reducción: $45:1$
- Pulsos Por Revolución (PPR) de Encoder: $11.0$ (Cuadratura: $\times 4$)
- Radio del Tambor de Cable: $1.0\text{ cm}$
- Ticks por Revolución: $11 \times 4 \times 45 = 1980\text{ ticks}$
- **Factor de Conversión (`TICKS_PER_CM`)**:
  $$
  \text{TICKS\_PER\_CM} = \frac{1980}{2 \pi \cdot 1.0} \approx 315.127 \text{ ticks/cm}
  $$

---

## Enlaces Relacionados
- [[00 - MOC CDPR]]
- [[Robot Paralelo de Cables 6-DOF]]
- [[Cinematica Inversa CDPR]]
- [[Esquema de Conexiones y Pines]]
- [[Decisiones de Diseño]]
