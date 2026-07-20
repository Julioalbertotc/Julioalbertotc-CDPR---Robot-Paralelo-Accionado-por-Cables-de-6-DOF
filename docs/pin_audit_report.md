# Auditoría Completa de Asignación de Pines - CDPR

Este documento presenta una auditoría completa del hardware y la configuración de pines definida en el firmware del CDPR (Robot Paralelo Accionado por Cables) para verificar posibles colisiones y proponer una solución viable que prescinda del expansor de pines **MCP23017**, utilizando únicamente los recursos nativos de los dos **ESP32** (Maestro y Esclavo).

---

## 1. Estado Actual del Firmware y Verificación de Conflictos

### ESP32 MAIN (Maestro) - Configuración Actual
Actualmente, el ESP32 MAIN controla la generación de PWM para los 8 motores, gestiona la comunicación UART con el esclavo, se comunica por I2C con el expansor MCP23017 y controla la línea de habilitación `STBY`.

| Función | Pin GPIO | Estado / Observación | ¿Conflicto? |
| :--- | :--- | :--- | :--- |
| **UART0 TX** | GPIO 1 | Usado para depuración y carga de firmware (USB Serial). | **No** (Esperado) |
| **UART0 RX** | GPIO 3 | Usado para depuración y carga de firmware (USB Serial). | **No** (Esperado) |
| **UART2 RX** | GPIO 16 | Interconexión con ESP32 ENCODER. | **No** |
| **UART2 TX** | GPIO 17 | Interconexión con ESP32 ENCODER. | **No** |
| **I2C SDA** | GPIO 21 | Comunicación con el expansor MCP23017. | **No** |
| **I2C SCL** | GPIO 22 | Comunicación con el expansor MCP23017. | **No** |
| **STBY (Unificado)**| GPIO 5 | Desactivación física global de los drivers TB6612FNG. | **No** |
| **Motor 0 PWM** | GPIO 2 | Pin de PWM del Motor 0. (Strapping pin: LED integrado). | **No** |
| **Motor 1 PWM** | GPIO 12 | Pin de PWM del Motor 1. (Strapping pin: MTDI). | **No** |
| **Motor 2 PWM** | GPIO 14 | Pin de PWM del Motor 2. | **No** |
| **Motor 3 PWM** | GPIO 23 | Pin de PWM del Motor 3. | **No** |
| **Motor 4 PWM** | GPIO 27 | Pin de PWM del Motor 4. | **No** |
| **Motor 5 PWM** | GPIO 18 | Pin de PWM del Motor 5. | **No** |
| **Motor 6 PWM** | GPIO 19 | Pin de PWM del Motor 6. | **No** |
| **Motor 7 PWM** | GPIO 25 | Pin de PWM del Motor 7. | **No** |

> [!WARNING]
> **Pines de Arranque (Strapping Pins) en el Maestro:**
> - **GPIO 12 (MTDI):** Si durante el encendido este pin es forzado a nivel HIGH por el circuito externo, el voltaje de la memoria flash interna del ESP32 se configurará a 1.8V en lugar de 3.3V, impidiendo que el microcontrolador arranque. Se debe garantizar que la impedancia de entrada del pin PWM del puente H no lo mantenga en alto durante el reset.
> - **GPIO 2:** Debe permanecer en nivel LOW o flotante durante el inicio para entrar en modo de ejecución normal.

---

### ESP32 ENCODER (Esclavo) - Configuración Actual
El ESP32 ENCODER lee los 8 encoders ópticos de cuadratura (16 señales) usando los contadores de hardware (PCNT) y transmite la posición acumulada vía UART2.

| Motor | Canal A | Canal B | Unidad PCNT | ¿Conflicto Interno? |
| :--- | :--- | :--- | :--- | :--- |
| **Motor 0** | GPIO 34 | GPIO 35 | PCNT 0 | **No** (Pines Input-only nativos, excelente) |
| **Motor 1** | GPIO 36 | GPIO 39 | PCNT 1 | **No** (Pines Input-only nativos, excelente) |
| **Motor 2** | GPIO 32 | GPIO 33 | PCNT 2 | **No** |
| **Motor 3** | GPIO 25 | GPIO 26 | PCNT 3 | **No** |
| **Motor 4** | GPIO 27 | GPIO 13 | PCNT 4 | **No** |
| **Motor 5** | GPIO 23 | GPIO 19 | PCNT 5 | **No** |
| **Motor 6** | GPIO 4 | GPIO 15 | PCNT 6 | **No** (GPIO 15 es strapping pin) |
| **Motor 7** | GPIO 18 | GPIO 14 | PCNT 7 | **No** |
| **UART2 RX** | GPIO 16 | - | Interconexión UART. | **No** |
| **UART2 TX** | GPIO 17 | - | Interconexión UART. | **No** |

> [!WARNING]
> **Pines de Arranque (Strapping Pins) en el Esclavo:**
> - **GPIO 15 (MTDO):** Controla la salida de logs de arranque. Si se conecta a un encoder, los pulsos durante el arranque no causarán fallos críticos, pero es buena práctica no colocar resistencias pull-down fuertes en esta línea.

---

## 2. Auditoría del Límite de Pines y el Expansor MCP23017

El diseño actual **SÍ utiliza un expansor de pines (MCP23017)** para controlar las señales de dirección `IN1` e `IN2` de los 8 motores (16 señales digitales de salida en total).

### ¿Por qué se colocó el expansor MCP23017 en el diseño inicial?
Para controlar 8 motores de manera centralizada desde un único ESP32 Maestro sin usar expansores, necesitaríamos:
- 8 señales PWM (salidas)
- 16 señales de dirección IN1/IN2 (salidas)
- 1 señal STBY (salida)
- 2 señales de comunicación UART2 (TX/RX)
- **Total = 27 pines de salida/entrada digitales.**

El ESP32 de 38 pines estándar solo tiene **21 pines GPIO configurables como salida**. Los pines 34, 35, 36 y 39 son únicamente de entrada. Los pines 6 al 11 están conectados a la memoria flash integrada y no se pueden usar. Por lo tanto, **físicamente es imposible controlar los 8 motores (PWM + Direcciones completas) y la UART usando un solo ESP32 sin expansores.**

---

## 3. Propuesta de Arquitectura Alternativa: Distribución Simétrica (Sin Expansores)

Para cumplir estrictamente con el requerimiento de **no utilizar expansores de pines** y usar solo los recursos nativos de los dos ESP32, se debe rediseñar la arquitectura lógica y física del robot:

### Solución: Reparto Simétrico de Motores (4 Motores por ESP32)
En lugar de especializar un ESP32 en control de motores y otro en encoders, **distribuimos los 8 motores de manera uniforme**:
1. **ESP32 Maestro (MAIN):** Controla directamente los **Motores 0, 1, 2 y 3** (PWM, Direcciones y Lectura de Encoders locales).
2. **ESP32 Auxiliar (SUB):** Controla directamente los **Motores 4, 5, 6 y 7** (PWM, Direcciones y Lectura de Encoders locales).

Ambos ESP32 intercambian comandos de sincronización a través de la UART2 a 1 kHz. El Maestro calcula la cinemática de los 8 cables, aplica el PID para sus 4 motores locales y envía los comandos de posición/velocidad al ESP32 Auxiliar para sus 4 motores, recibiendo de vuelta el estado de los mismos.

### Viabilidad de Pines por ESP32 en Distribución Simétrica
Para controlar y leer **4 motores** en una sola placa ESP32, se requiere:
- Encoders: 4 motores × 2 canales = 8 entradas (4 de ellas pueden usar los pines input-only 34, 35, 36, 39).
- PWM: 4 salidas.
- Direcciones (IN1, IN2): 8 salidas.
- Standby (STBY): 1 salida unificada.
- UART2: 2 pines (TX/RX).
- **Total de Pines de Salida/Comunicación necesarios = 4 (PWM) + 8 (DIR) + 1 (STBY) + 2 (UART) = 15 pines de propósito general.**
- **Total de Pines de Entrada necesarios = 4 pines de entrada general (o input-only).**

Esto encaja perfectamente dentro de los límites físicos de un ESP32 estándar (21 GPIOs bidireccionales + 4 GPIOs input-only).

---

## 4. Tabla de Asignación de Pines Propuesta (Sin Expansores)

A continuación se muestra el mapa de pines óptimo y libre de conflictos para implementar esta arquitectura distribuida sin expansores de pines.

### Placa 1: ESP32 Maestro (MAIN) - Motores 0 a 3
| Dispositivo / Función | Señal | Pin GPIO | Tipo | Notas / Strapping |
| :--- | :--- | :--- | :--- | :--- |
| **Interconexión UART** | UART2 RX2 | **GPIO 16** | Input | Interconexión con Placa 2 |
| **Interconexión UART** | UART2 TX2 | **GPIO 17** | Output | Interconexión con Placa 2 |
| **Seguridad Driver** | STBY | **GPIO 5** | Output | Habilitación global motores 0-3. Strapping normal. |
| **Motor 0** | PWM | **GPIO 2** | Output | Strapping (mantener flotante/bajo en boot) |
| **Motor 0** | IN1 | **GPIO 4** | Output | Dirección 1 |
| **Motor 0** | IN2 | **GPIO 15** | Output | Dirección 2. Strapping. |
| **Motor 0** | Enc A | **GPIO 34** | Input | Input-only |
| **Motor 0** | Enc B | **GPIO 35** | Input | Input-only |
| **Motor 1** | PWM | **GPIO 12** | Output | Strapping (MTDI: evitar pull-up fuerte) |
| **Motor 1** | IN1 | **GPIO 13** | Output | Dirección 1 |
| **Motor 1** | IN2 | **GPIO 14** | Output | Dirección 2 |
| **Motor 1** | Enc A | **GPIO 36** | Input | Input-only |
| **Motor 1** | Enc B | **GPIO 39** | Input | Input-only |
| **Motor 2** | PWM | **GPIO 25** | Output | Control PWM |
| **Motor 2** | IN1 | **GPIO 26** | Output | Dirección 1 |
| **Motor 2** | IN2 | **GPIO 27** | Output | Dirección 2 |
| **Motor 2** | Enc A | **GPIO 32** | Input | Entrada Encoder (Soporta ADC/Touch) |
| **Motor 2** | Enc B | **GPIO 33** | Input | Entrada Encoder (Soporta ADC/Touch) |
| **Motor 3** | PWM | **GPIO 18** | Output | Control PWM |
| **Motor 3** | IN1 | **GPIO 19** | Output | Dirección 1 |
| **Motor 3** | IN2 | **GPIO 23** | Output | Dirección 2 |
| **Motor 3** | Enc A | **GPIO 21** | Input | Entrada Encoder (SDA si se requiere I2C) |
| **Motor 3** | Enc B | **GPIO 22** | Input | Entrada Encoder (SCL si se requiere I2C) |

---

### Placa 2: ESP32 Auxiliar (SUB/ENCODER) - Motores 4 a 7
| Dispositivo / Función | Señal | Pin GPIO | Tipo | Notas / Strapping |
| :--- | :--- | :--- | :--- | :--- |
| **Interconexión UART** | UART2 RX2 | **GPIO 16** | Input | Conexión cruzada a TX2 de Placa 1 |
| **Interconexión UART** | UART2 TX2 | **GPIO 17** | Output | Conexión cruzada a RX2 de Placa 1 |
| **Seguridad Driver** | STBY | **GPIO 5** | Output | Habilitación global motores 4-7. |
| **Motor 4** | PWM | **GPIO 2** | Output | Strapping (mantener flotante/bajo en boot) |
| **Motor 4** | IN1 | **GPIO 4** | Output | Dirección 1 |
| **Motor 4** | IN2 | **GPIO 15** | Output | Dirección 2. Strapping. |
| **Motor 4** | Enc A | **GPIO 34** | Input | Input-only |
| **Motor 4** | Enc B | **GPIO 35** | Input | Input-only |
| **Motor 5** | PWM | **GPIO 12** | Output | Strapping (MTDI: evitar pull-up fuerte) |
| **Motor 5** | IN1 | **GPIO 13** | Output | Dirección 1 |
| **Motor 5** | IN2 | **GPIO 14** | Output | Dirección 2 |
| **Motor 5** | Enc A | **GPIO 36** | Input | Input-only |
| **Motor 5** | Enc B | **GPIO 39** | Input | Input-only |
| **Motor 6** | PWM | **GPIO 25** | Output | Control PWM |
| **Motor 6** | IN1 | **GPIO 26** | Output | Dirección 1 |
| **Motor 6** | IN2 | **GPIO 27** | Output | Dirección 2 |
| **Motor 6** | Enc A | **GPIO 32** | Input | Entrada Encoder |
| **Motor 6** | Enc B | **GPIO 33** | Input | Entrada Encoder |
| **Motor 7** | PWM | **GPIO 18** | Output | Control PWM |
| **Motor 7** | IN1 | **GPIO 19** | Output | Dirección 1 |
| **Motor 7** | IN2 | **GPIO 23** | Output | Dirección 2 |
| **Motor 7** | Enc A | **GPIO 21** | Input | Entrada Encoder |
| **Motor 7** | Enc B | **GPIO 22** | Input | Entrada Encoder |

---

## 5. Conclusiones y Recomendaciones de la Auditoría

1. **Estado Actual:** No existen colisiones de pines en la configuración cargada actualmente, pero **sí se está usando un expansor de pines MCP23017** por hardware e I2C en el maestro para controlar las direcciones de los motores.
2. **Uso de Extensor:** Para poder eliminar el MCP23017 y cumplir con el requerimiento de usar únicamente los dos ESP32, es **mandatorio abandonar el esquema Maestro-Motor / Esclavo-Encoder puro** y adoptar el **Esquema de Distribución Simétrica (4 motores por placa)** detallado en la sección 4.
3. **Pines Críticos:** Se debe prestar especial atención a los pines de arranque **GPIO 12, 15 y 2** en ambas placas al realizar el cableado físico para evitar problemas de booteo o de flasheo.
