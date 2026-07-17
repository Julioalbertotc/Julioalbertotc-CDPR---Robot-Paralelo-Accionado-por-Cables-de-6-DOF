---
tags: [proyecto/cdpr, tipo/proyecto]
estado: en progreso
creado: 2026-07-16
---
relacionado: [[00 - MOC CDPR]], [[Roadmap del Proyecto]], [[Bitácora de Pruebas]]
---

# Robot Paralelo de Cables 6-DOF

## Resumen del Proyecto
Este proyecto consiste en la implementación del firmware y software de simulación para un **Robot Paralelo Accionado por Cables (CDPR)** de 6 grados de libertad (X, Y, Z, Roll, Pitch, Yaw). La arquitectura lógica y de hardware se divide en dos placas **ESP32** para maximizar la velocidad de respuesta en tiempo real (lazo PID a 1 kHz) y evitar problemas de contención de pines.

---

## Conceptos Clave
- **[[Arquitectura Dual ESP32]]**: División de trabajo entre un controlador Maestro (Motores, PID, Servidor Web) y un Esclavo (PCNT Encoder Reader).
- **[[Protocolo UART Binario]]**: Interconexión serial binaria de alta velocidad (921600 bps) protegida por CRC8.
- **[[Cinematica Inversa CDPR]]**: Modelo geométrico 3D que determina la longitud teórica de cada cable dada la pose del efector final.
- **[[Control PID de Posicion]]**: Lazo cerrado de posición para controlar los motores con torque de retención activo anti-holgura.
- **[[Esquema de Conexiones y Pines]]**: Distribución física de los pines e integración del expansor MCP23017 y los drivers TB6612FNG.
- **[[Simulador Python CDPR]]**: Entorno de validación y simulación dinámica en PC.

---

## Aplicaciones Prácticas
1. **Posicionamiento Espacial de Cargas**: Simulación y movimiento de un efector final de 6-DOF en un espacio volumétrico acotado.
2. **Estudio Cinémico y Dinámico**: Ensayos de tensión de cables y control de trayectorias utilizando simulación interna en tiempo real.
3. **Plataformas de Movimiento**: Base para el desarrollo de simuladores de vuelo o cabinas interactivas.

---

## Estado y Siguiente Hito (Proyecto)
> [!NOTE]
> **Siguiente Hito**: Validar el control PID en bucle cerrado integrando los controladores de motor hardware y calificar la exactitud del cálculo cinemático en modo simulación física (`SIMULATION_MODE=1`). Ver la lista detallada en [[Roadmap del Proyecto]].

---

## Enlaces Relacionados
- [[00 - MOC CDPR]]
- [[Roadmap del Proyecto]]
- [[Especificaciones de Hardware]]
- [[Bitácora de Pruebas]]
