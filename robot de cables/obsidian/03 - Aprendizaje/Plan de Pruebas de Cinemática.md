---
tags: [proyecto/cdpr, tipo/aprendizaje]
estado: borrador
creado: 2026-07-16
---
relacionado: [[00 - MOC CDPR]], [[Cinematica Inversa CDPR]], [[Simulador Python CDPR]]
---

# Plan de Pruebas de Cinemática

Este documento define la estrategia y los casos de prueba para validar que el modelo cinemático del robot paralelo accionado por cables (CDPR) implementado en el ESP32 coincide exactamente con el modelo matemático y la simulación interactiva de Python.

---

## 🎯 Estrategia de Validación

La validación se realiza comparando los resultados del cálculo de longitudes de cable de la función `calculateInverseKinematics` ejecutada en C++ en el ESP32 contra la función `calculate_inverse_kinematics` ejecutada en Python en el PC, para un conjunto de poses seleccionadas.

### Criterio de Aceptación
Para cada caso de prueba, la diferencia máxima absoluta entre la longitud de cada uno de los 8 cables calculada en Python y en C++ debe cumplir:
$$
\Delta L_i = |L_{\text{python}, i} - L_{\text{cpp}, i}| \le 0.001\text{ cm} \quad (10\,\mu\text{m})
$$

---

## 📋 Casos de Prueba Críticos

| ID Caso | Pose de Entrada $(x, y, z, Roll, Pitch, Yaw)$ | Propósito de la Prueba | Longitudes Teóricas Esperadas |
| :--- | :--- | :--- | :--- |
| **TC-01** | $(0.0, 0.0, 22.5, 0.0, 0.0, 0.0)$ | **Pose Central (Reposo)**: Verifica el estado inicial simétrico y alineado. | Los 8 cables deben medir exactamente la misma longitud por simetría geométrica: $31.819\text{ cm}$. |
| **TC-02** | $(0.0, 0.0, 45.0, 0.0, 0.0, 0.0)$ | **Límite Superior**: Plataforma móvil llevada al plano superior donde se encuentran las poleas fijas. | Cables muy cortos. Caso extremo para comprobar singularidades físicas del modelo matemático. |
| **TC-03** | $(10.0, -10.0, 20.0, 0.0, 0.0, 0.0)$ | **Traslación Pura Descentrada**: Movimiento lineal diagonal fuera del origen. | Longitudes asimétricas. Verifica la correcta proyección de las coordenadas de las poleas. |
| **TC-04** | $(0.0, 0.0, 22.5, 0.174, 0.0, 0.0)$ | **Rotación Pura de Roll ($10^\circ$)**: La plataforma rota alrededor del eje X. | Evalúa los términos sen/cos del Roll en la matriz de rotación $R_{zyx}$. |
| **TC-05** | $(0.0, 0.0, 22.5, 0.174, 0.174, 0.174)$ | **Rotación y Traslación Combinada**: Pose arbitraria de 6 grados de libertad. | Acoplamiento total de todas las variables trigonométricas en la matriz de rotación. |

---

## 💻 Procedimiento de Ejecución
1. Configurar los ESP32 en `SIMULATION_MODE=1`.
2. Conectar el ESP32 MAIN a la PC y abrir la consola serie para recibir las tramas de telemetría de depuración.
3. Utilizar el script `validation.py` para inyectar secuencialmente los comandos de pose correspondientes a los casos de prueba `TC-01` a `TC-05`.
4. Registrar en la [[Bitácora de Pruebas]] la salida de longitudes calculadas por el ESP32 y comparar con la simulación en Python.

---

## Enlaces Relacionados
- [[00 - MOC CDPR]]
- [[Cinematica Inversa CDPR]]
- [[Simulador Python CDPR]]
- [[Bitácora de Pruebas]]
