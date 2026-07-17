---
tags: [proyecto/cdpr, tipo/roadmap]
estado: en progreso
creado: 2026-07-16
---
relacionado: [[00 - MOC CDPR]], [[Robot Paralelo de Cables 6-DOF]]
---

# 🗺️ Roadmap del Proyecto - CDPR

Este documento registra los hitos alcanzados y planificados para el desarrollo, simulación y puesta en marcha del Robot Paralelo Accionado por Cables (CDPR).

---

## 🏁 Estado General del Proyecto
- [x] **Fase 1: Diseño Mecánico y Definición Geométrica**
- [x] **Fase 2: Arquitectura del Firmware Dual (MAIN + ENCODER)**
- [/] **Fase 3: Integración de Controladores Físicos y Lazo Cerrado**
- [ ] **Fase 4: Puesta en Marcha Física y Calibración**
- [ ] **Fase 5: Trayectorias Avanzadas e Interfaz Web Final**

---

## 🎯 Plan Detallado de Hitos

### Hito 1: Modelo Geométrico y Simulación Dinámica (Completado)
- [x] Definir coordenadas estáticas de las poleas (`POLE_POSITIONS`) y los anclajes de la plataforma móvil (`ANCHOR_POSITIONS`).
- [x] Desarrollar el script de simulación cinemática inversa en Python (`kinematics.py`).
- [x] Implementar simulación del comportamiento transitorio del motor DC basado en variables electromecánicas en Python (`motor_sim.py`).
- [x] Diseñar visor 3D de cables y efector en tiempo real (`visualizer.py`).

### Hito 2: Comunicación y Lecturas de Hardware (Completado)
- [x] Desarrollar firmware de lectura de encoder de 8 canales usando contadores PCNT de hardware en el ESP32 ENCODER.
- [x] Definir protocolo serial binario de comunicación bidireccional a 921600 bps.
- [x] Implementar validación de redundancia de tramas por CRC-8 Dallas/Maxim.
- [x] Crear máquina de estados lógica del robot (`STATE_INIT`, `STATE_HOMING`, `STATE_READY`, `STATE_ESTOP`, etc.).
- [x] Probar el lazo de seguridad de pérdida de comunicación UART ($50\text{ ms}$ timeout para apagar motores).

### Hito 3: Control en Lazo Cerrado e Integración de Motores (En Progreso)
- [/] Resolver conflictos de pines de arranque (*strapping pins*) en el ESP32 MAIN para evitar bloqueos del procesador al encender.
- [x] Implementar regulador PID de posición a 1 kHz en el ESP32 MAIN.
- [x] Diseñar algoritmo de torque de retención activo (`anti-holgura`) para mantener cables tensionados en el firmware.
- [ ] Implementar la interfaz I2C a 400 kHz con el expansor MCP23017 para conmutar las direcciones físicas de los puentes H TB6612FNG.
- [ ] Validar la cinemática inversa en bucle cerrado usando simulación UART por hardware (`SIMULATION_MODE=1`).

### Hito 4: Calibración y Movimiento Real (Planificado)
- [ ] Soldar pull-ups de $10\text{ k}\Omega$ en los canales de entrada analógica/digital GPI del ESP32 ENCODER para lecturas limpias de encoder.
- [ ] Calibrar el punto cero (*Homing*) tensando manualmente cada cable y reseteando los contadores acumulados de encoders.
- [ ] Ejecutar movimientos simples de traducción pura (ejes X, Y, Z de forma independiente) a velocidad controlada.
- [ ] Caracterizar la holgura del cable y la variación no lineal del radio del tambor al enrollarse.

---

## Enlaces Relacionados
- [[00 - MOC CDPR]]
- [[Robot Paralelo de Cables 6-DOF]]
- [[Especificaciones de Hardware]]
- [[Plan de Pruebas de Cinemática]]
