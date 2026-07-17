---
tags: [proyecto/cdpr, tipo/aprendizaje]
estado: validado
creado: 2026-07-16
---
relacionado: [[00 - MOC CDPR]], [[Cinematica Inversa CDPR]], [[Plan de Pruebas de Cinemática]]
---

# Simulador Python CDPR

## Definición
Conjunto de herramientas en Python diseñadas para validar de manera interactiva la cinemática inversa, visualizar en 3D el estado del robot paralelo accionado por cables y simular dinámicamente el comportamiento de los motores en PC antes del despliegue en hardware real.

---

## Estructura de Módulos del Simulador

El directorio `python-sim/` contiene las siguientes utilidades:
1. `kinematics.py`: Módulo de cinemática inversa que calcula la longitud teórica de los 8 cables dada una pose de 6-DOF.
2. `motor_sim.py`: Simulación física del motor DC basada en ecuaciones diferenciales del sistema electromecánico:
   - Resistencia ($R = 7.5\,\Omega$), Inductancia ($L = 0.05\text{ H}$), Inercia ($J = 0.0001$), Fricción viscosa ($B = 0.0002$), Constante de torque/fuerza contraelectromotriz ($K_t = K_e = 0.08$).
3. `validation.py`: Script para interactuar por comunicación serie serializada JSON con el ESP32 MAIN y registrar datos de comportamiento.
4. `visualizer.py`: Visualización 3D del volumen del robot, postes, plataforma y tensiones relativas de los cables usando matplotlib/3D.

---

## Ejecución del Sistema de Validación y Simulación
Para correr el simulador dinámico y validar la interconexión con el firmware:

1. **Habilitar modo Simulación**:
   Define `-DSIMULATION_MODE=1` en los archivos `platformio.ini` tanto del ESP32 MAIN como de la placa ENCODER.
2. **Conexión física**:
   Une los dos ESP32 cruzando sus líneas UART y compartiendo GND (ver [[Esquema de Conexiones y Pines]]).
3. **Lanzar Script en PC**:
   Conecta el puerto USB del ESP32 MAIN a tu PC y ejecuta:
   ```bash
   python python-sim/validation.py
   ```

---

## Enlaces Relacionados
- [[00 - MOC CDPR]]
- [[Robot Paralelo de Cables 6-DOF]]
- [[Cinematica Inversa CDPR]]
- [[Esquema de Conexiones y Pines]]
- [[Plan de Pruebas de Cinemática]]
