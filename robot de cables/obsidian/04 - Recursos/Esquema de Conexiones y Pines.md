---
tags: [proyecto/cdpr, tipo/recurso]
estado: validado
creado: 2026-07-16
---
relacionado: [[00 - MOC CDPR]], [[Arquitectura Dual ESP32]], [[Control PID de Posicion]], [[Especificaciones de Hardware]], [[Errores Conocidos y Troubleshooting]]
---

# Esquema de Conexiones y Pines

## Resumen del Hardware
El robot utiliza un expansor de pines I2C **MCP23017** de 16 bits para controlar las entradas de dirección (IN1 e IN2) de los 4 drivers de puente H de motor **TB6612FNG**. Las señales de PWM se conectan directamente a 8 GPIOs independientes del ESP32 MAIN.

---

## Conexiones I2C e Interconexión de Placas

### 1. Enlace UART Binario (Inter-ESP32)
> [!IMPORTANT]
> **Tierra Común Obligatoria**: El cable de interconexión UART entre ambos ESP32 debe tener las líneas TX/RX cruzadas y es **estrictamente obligatorio compartir la línea GND** para asegurar que el flujo de datos no se corrompa por diferencias de potencial eléctrico.

```text
[ ESP32 MAIN ]                                  [ ESP32 ENCODER ]
  GND -------------------------------------------- GND (Común Obligatorio)
  RX2 (GPIO 16) <--------------------------------- TX2 (GPIO 17) [Telemetría Encoders]
  TX2 (GPIO 17) ---------------------------------> RX2 (GPIO 16) [Consignas PWM en Simulación]
```

### 2. Canales I2C (ESP32 MAIN a MCP23017)
- **SDA**: GPIO 21 $\leftrightarrow$ Pin 13 del MCP23017
- **SCL**: GPIO 22 $\leftrightarrow$ Pin 12 del MCP23017
- **STBY (Standby Unificado de Drivers)**: GPIO 5 $\rightarrow$ Conectado a la línea `STBY` de los 4 TB6612FNG. Si se tira a `LOW`, desactiva físicamente todos los motores (función ESTOP).

---

## Asignación de Pines y Bits

### ESP32 MAIN / MOTOR (Control de Actuadores)
| Motor | Pin PWM ESP32 | Bit MCP23017 (IN1) | Bit MCP23017 (IN2) | Puerto MCP23017 |
| :--- | :--- | :--- | :--- | :--- |
| **Motor 0** | GPIO 2 | Bit 0 | Bit 1 | GPA0 (Pin 21) / GPA1 (Pin 22) |
| **Motor 1** | GPIO 12 | Bit 2 | Bit 3 | GPA2 (Pin 23) / GPA3 (Pin 24) |
| **Motor 2** | GPIO 14 | Bit 4 | Bit 5 | GPA4 (Pin 25) / GPA5 (Pin 26) |
| **Motor 3** | GPIO 23 | Bit 6 | Bit 7 | GPA6 (Pin 27) / GPA7 (Pin 28) |
| **Motor 4** | GPIO 27 | Bit 8 | Bit 9 | GPB0 (Pin 1) / GPB1 (Pin 2) |
| **Motor 5** | GPIO 18 | Bit 10 | Bit 11 | GPB2 (Pin 3) / GPB3 (Pin 4) |
| **Motor 6** | GPIO 19 | Bit 12 | Bit 13 | GPB4 (Pin 5) / GPB5 (Pin 6) |
| **Motor 7** | GPIO 25 | Bit 14 | Bit 15 | GPB6 (Pin 7) / GPB7 (Pin 8) |

> [!WARNING]
> **Riesgo de Strapping Pins**:
> - **GPIO 12 (MTDI)** es un pin de strapping. Si está a nivel `HIGH` durante el inicio del ESP32 MAIN, la tensión de la memoria flash se configurará a 1.8V en lugar de 3.3V, lo que causará un fallo de arranque en la mayoría de módulos ESP32. Asegúrese de que la entrada del TB6612FNG no mantenga un pull-up en esta línea en el encendido.
> - **GPIO 2** es otro pin de strapping (debe estar a `LOW` o flotante al bootear).

---

### ESP32 ENCODER (Lectura de Captura)
El ESP32 secundario dedica sus periféricos PCNT a la lectura rápida de los encoders:

| Motor | Canal Encoder A (GPIO) | Canal Encoder B (GPIO) | Unidad PCNT |
| :--- | :--- | :--- | :--- |
| **Motor 0** | GPIO 34 | GPIO 35 | PCNT 0 |
| **Motor 1** | GPIO 36 | GPIO 39 | PCNT 1 |
| **Motor 2** | GPIO 32 | GPIO 33 | PCNT 2 |
| **Motor 3** | GPIO 25 | GPIO 26 | PCNT 3 |
| **Motor 4** | GPIO 27 | GPIO 13 | PCNT 4 |
| **Motor 5** | GPIO 23 | GPIO 19 | PCNT 5 |
| **Motor 6** | GPIO 4 | GPIO 15 | PCNT 6 |
| **Motor 7** | GPIO 18 | GPIO 14 | PCNT 7 |

> [!IMPORTANT]
> Los pines de entrada **GPIO 34, 35, 36 y 39** en el ESP32 no disponen de resistencias de pull-up o pull-down internas por software. Si los encoders de tus motores son de colector abierto (open-drain), es estrictamente necesario soldar resistencias de pull-up externas de $10\text{ k}\Omega$ en estas líneas.

---

## Enlaces Relacionados
- [[00 - MOC CDPR]]
- [[Robot Paralelo de Cables 6-DOF]]
- [[Arquitectura Dual ESP32]]
- [[Control PID de Posicion]]
- [[Especificaciones de Hardware]]
