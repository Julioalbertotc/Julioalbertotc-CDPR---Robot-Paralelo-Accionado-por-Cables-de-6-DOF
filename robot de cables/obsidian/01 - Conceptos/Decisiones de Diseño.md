---
tags: [proyecto/cdpr, tipo/concepto]
estado: validado
creado: 2026-07-16
---
relacionado: [[00 - MOC CDPR]], [[Arquitectura Dual ESP32]], [[Esquema de Conexiones y Pines]]
---

# Decisiones de Diseño

En este documento se registran y justifican técnicamente las decisiones de diseño tomadas para la arquitectura y el hardware del Robot Paralelo Accionado por Cables (CDPR) de 6-DOF.

---

## ⚖️ Análisis de Decisiones Clave

### 1. ¿Por qué 8 cables para 6 grados de libertad?
- **Problema**: Los cables son elementos unidireccionales (solo pueden ejercer fuerza en tensión, no en compresión).
- **Decisión**: Se optó por una configuración **redundante en cables** de $m = 8$ cables para un sistema de $n = 6$ grados de libertad ($m = n + 2$).
- **Justificación**:
  - En un robot de cables con la misma cantidad de cables que de grados de libertad ($m = n = 6$), es imposible mantener la tensión en todos los cables en todo el espacio de trabajo. Habrá direcciones en las que el robot se volverá inestable o perderá rigidez.
  - Al añadir redundancia ($8$ cables), siempre es posible encontrar una distribución de tensiones estrictamente positiva (todos los cables tensos) para cualquier pose dentro del espacio de trabajo estático útil, eliminando singularidades internas y mejorando la rigidez dinámica.

### 2. ¿Por qué una Arquitectura Dual ESP32 en lugar de un único ESP32?
- **Problema**: La lectura de 8 encoders incrementales con canales A/B en cuadratura requiere 16 pines rápidos y la gestión de 8 periféricos PCNT. Al mismo tiempo, el control PID de 8 motores requiere 8 canales PWM, 16 señales de dirección de puente H (por I2C), el cálculo de la cinemática inversa flotante, y la gestión del servidor Web/WebSockets por WiFi a 1 kHz.
- **Decisión**: Separar el sistema en un **ESP32 MAIN (Maestro)** y un **ESP32 ENCODER (Esclavo)**.
- **Justificación**:
  - **Contención de Pines**: El ESP32 no cuenta con suficientes GPIOs accesibles simultáneamente sin interferir con las memorias flash internas (pines de strapping y SDIO) y la conectividad WiFi.
  - **Latencia de Interrupciones y Context Switching**: La tarea de lectura y acumulación de encoders por software afectaría la constancia del lazo PID a 1 kHz. Delegar los contadores PCNT de hardware a una placa secundaria libera al Maestro para realizar operaciones cinemáticas complejas en punto flotante sin jitter.

### 3. ¿Por qué usar el expansor MCP23017 en vez de GPIOs directos para la dirección?
- **Problema**: Controlar la dirección de 8 motores mediante puentes H convencionales requiere 2 pines discretos por motor ($16$ pines en total).
- **Decisión**: Utilizar un expansor de puertos GPIO de 16 bits **MCP23017** controlado por I2C a alta velocidad ($400\text{ kHz}$).
- **Justificación**:
  - Al utilizar I2C, solo se consumen 2 pines del ESP32 MAIN (SDA/SCL) en lugar de 16 pines discretos.
  - Las señales de dirección (`IN1`/`IN2`) de los TB6612FNG no requieren conmutación rápida de microsegundos (a diferencia del PWM), por lo que la velocidad del bus I2C a 400 kHz es más que suficiente para actualizarlas dentro del ciclo de control de 1 ms (1 kHz).

---

## Enlaces Relacionados
- [[00 - MOC CDPR]]
- [[Robot Paralelo de Cables 6-DOF]]
- [[Arquitectura Dual ESP32]]
- [[Control PID de Posicion]]
