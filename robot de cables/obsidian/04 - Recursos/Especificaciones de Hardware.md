---
tags: [proyecto/cdpr, tipo/recurso]
estado: borrador
creado: 2026-07-16
---
relacionado: [[00 - MOC CDPR]], [[Esquema de Conexiones y Pines]]
---

# Especificaciones de Hardware

Ficha técnica y características consolidadas del hardware utilizado para la construcción e integración del Robot Paralelo Accionado por Cables (CDPR).

---

## ⚡ Especificación Eléctrica y Electrónica

### 1. Controladores Principales
- **ESP32 MAIN (Maestro)**: Microcontrolador Tensilica dual-core a 240 MHz con conectividad WiFi/Bluetooth. Encargado de cinemática inversa, lazo PID a 1 kHz y servidor web interactivo.
- **ESP32 ENCODER (Esclavo)**: Microcontrolador secundario dedicado. Utiliza las 8 unidades de contadores de pulsos físicos por hardware (**PCNT**) para decodificar encoders en cuadratura sin sobrecargar el procesador.

### 2. Controladores de Motores (Puentes H)
- **Chips**: 4x Módulos Breakout **TB6612FNG**.
- **Canales**: 2 puentes H por circuito integrado, permitiendo controlar un total de 8 motores DC.
- **Corriente Nominal**: $1.2\text{ A}$ continuos por canal ($3.2\text{ A}$ de pico).
- **Control**: Señales PWM conectadas a GPIOs dedicados del ESP32 MAIN y señales de dirección (`IN1`/`IN2`) controladas por el expansor de E/S MCP23017.
- **Línea STBY**: Todos los pines de standby de los drivers están unificados físicamente y conectados al GPIO 5 del ESP32 MAIN para permitir un apagado por hardware instantáneo.

### 3. Expansor de Puertos I2C
- **Chip**: **MCP23017** de 16 bits.
- **Dirección I2C**: $0x20$ (Configurable por hardware mediante pines A0, A1, A2).
- **Frecuencia de Operación**: Modo rápido de I2C ($400\text{ kHz}$).

---

## ⚙️ Especificación Mecánica y Actuadores

### 1. Motores y Encoders
- **Tipo de Motor**: Motor DC reductor (modelo específico pendiente de confirmación, ej. *Pololu 25D mm*).
- **Voltaje de Alimentación**: $12\text{ V}$ nominales (máximo del TB6612FNG: $15\text{ V}$).
- **Relación de Reducción (`GEAR_RATIO`)**: $45:1$.
- **Encoder**: Incremental de cuadratura (2 canales A/B) acoplado al eje del motor antes de la reducción.
- **Resolución (`ENCODER_PPR`)**: $11.0$ pulsos por revolución del eje del motor (PPR).
- **Tambor de Bobinado**: Tambor cilíndrico de radio $r = 1.0\text{ cm}$ acoplado al eje de salida.

### 2. Estructura Física y Cables
- **Estructura del Robot**: Marco cuadrangular de postes verticales.
  - Dimensiones del volumen de trabajo útil aproximado: $45\text{ cm} \times 45\text{ cm} \times 45\text{ cm}$ (según coordenadas de poleas).
- **Cables**: Hilo de pescar trenzado de polietileno de ultra alto peso molecular (Dyneema/Spectra) o hilo de nylon de alta resistencia a la tracción.
- **Poleas**: Poleas fijas con rodamientos situadas en las 4 esquinas superiores de la estructura.

---

> [!WARNING]
> **Datos Pendientes por Validar en Campo**:
> 1. **Modelo exacto de los motores**: Confirmar la corriente máxima en rotor bloqueado (stall current) para evitar sobrepasar el límite de los TB6612FNG ($1.2\text{ A}$ continuo).
> 2. **Tensión del cable máxima recomendada**: Límite de carga de rotura del cable utilizado.
> 3. **Fuente de poder**: Capacidad en Amperios de la fuente de 12V necesaria para alimentar los 8 motores simultáneamente en condiciones de alta carga.

---

## Enlaces Relacionados
- [[00 - MOC CDPR]]
- [[Esquema de Conexiones y Pines]]
- [[Robot Paralelo de Cables 6-DOF]]
