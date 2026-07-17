---
tags: [proyecto/cdpr, tipo/concepto]
estado: validado
creado: 2026-07-16
relacionado: [[00 - MOC CDPR]]
---

# Glosario CDPR

Diccionario de términos técnicos y conceptos utilizados en el diseño, desarrollo y control de nuestro Robot Paralelo Accionado por Cables (CDPR).

---

## 📖 Definición de Términos

- **CDPR (Cable-Driven Parallel Robot)**: Robot paralelo en el cual la plataforma móvil (efector final) está suspendida y controlada mediante la tracción coordinada de múltiples cables flexibles.
- **PCNT (Pulse Counter)**: Periférico de hardware del ESP32 diseñado para contar transiciones de señal (flancos de subida y bajada) en canales de entrada de manera autónoma, consumiendo cero ciclos de CPU. Es clave para decodificar los canales de cuadratura de los encoders incrementales de los motores.
- **Anti-Windup**: Lógica de protección en controladores PID que detiene o limita la acumulación del término integral cuando la señal de salida calculada satura los límites físicos del actuador (en nuestro caso, $\pm 255$ PWM), previniendo sobreoscilaciones prolongadas al cambiar de dirección.
- **Torque de Retención Activo (Anti-Holgura)**: Valor mínimo de tensión (expresado en ciclo de trabajo PWM, en nuestro caso $25$) que se aplica a los motores de los cables que no están experimentando fuerzas de tracción principales para evitar que queden flácidos (holgura).
- **Freno Activo (Short-Brake)**: Método de frenado en puentes H (como el TB6612FNG) que cortocircuita los dos terminales del motor a tierra (`IN1 = LOW, IN2 = LOW`). Esto aprovecha la fuerza contraelectromotriz del motor DC para detener la inercia del tambor de forma inmediata.
- **CRC-8 (Cyclic Redundancy Check de 8 bits)**: Algoritmo de detección de errores de transmisión utilizado para validar la integridad de las tramas UART binarias compartidas a 1 kHz entre el ESP32 MAIN y el ESP32 ENCODER. Se utiliza el polinomio Dallas/Maxim ($0x31$).
- **Strapping Pins (Pines de Configuración)**: Pines del ESP32 (como GPIO 12, GPIO 2, GPIO 15, GPIO 0, GPIO 5) que son leídos por la ROM interna del microcontrolador al arrancar (boot) para configurar el modo de ejecución, voltaje de flash o el comportamiento de la señal de reloj. Su uso inadecuado puede impedir el encendido del microcontrolador.
- **Matriz de Rotación $R_{zyx}$**: Matriz matemática ortogonal en 3D que describe la orientación espacial del efector final mediante la secuencia de rotaciones de Tait-Bryan en los ejes Z (Yaw), Y (Pitch) e X (Roll).
- **Workspace (Espacio de Trabajo)**: El volumen en el espacio 3D que puede alcanzar físicamente el efector final del robot manteniendo todos los cables bajo una tensión mínima aceptable.

---

## Enlaces Relacionados
- [[00 - MOC CDPR]]
- [[Robot Paralelo de Cables 6-DOF]]
- [[Cinematica Inversa CDPR]]
- [[Control PID de Posicion]]
