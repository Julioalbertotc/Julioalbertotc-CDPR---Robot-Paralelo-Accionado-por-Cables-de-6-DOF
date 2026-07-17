# CDPR - Robot Paralelo Accionado por Cables de 6-DOF (Arquitectura Dual ESP32)

Este repositorio contiene la arquitectura de software completa para un Robot Paralelo Accionado por Cables (CDPR) de 6 grados de libertad (X, Y, Z, Roll, Pitch, Yaw) controlado por una arquitectura de **dos placas ESP32** interconectadas por un bus UART binario de alta velocidad (921600 bps).

La división del control optimiza la disponibilidad de pines y recursos de hardware dedicando un microcontrolador exclusivamente a la lectura de encoders (esclavo) y otro a la cinemática inversa, control PID, WiFi y servidor web (maestro).

---

## Arquitectura de Hardware y Pines

El sistema físico se compone de:
1. **ESP32 MAIN/MOTOR (Maestro)**: Se encarga de la cinemática inversa, del lazo PID de posición a 1 kHz, de la máquina de estados, del servidor web/WebSocket y de la conexión serial JSON para validación con PC.
2. **ESP32 ENCODER (Esclavo)**: Lee las 8 unidades de contadores hardware PCNT para los encoders y las envía vía UART al maestro a 1 kHz.

### Diagrama de Conexiones UART

> [!IMPORTANT]
> El cable de interconexión UART entre ambos ESP32 debe tener las líneas TX/RX cruzadas y es **estrictamente obligatorio compartir la línea GND (Tierra común)** para que el flujo de datos no se corrompa debido a diferencias de potencial.

```text
[ ESP32 MAIN ]                                  [ ESP32 ENCODER ]
  GND -------------------------------------------- GND (Común Obligatorio)
  RX2 (GPIO 16) <--------------------------------- TX2 (GPIO 17) [Telemetría Encoders]
  TX2 (GPIO 17) ---------------------------------> RX2 (GPIO 16) [Consignas PWM en Simulación]
```

### Tabla de Asignación de Pines - ESP32 MAIN/MOTOR

La dirección de cada motor se controla a través de dos señales (IN1 e IN2) utilizando el expansor I2C MCP23017 de 16 bits. La línea STBY de los 4 breakouts TB6612FNG está unificada físicamente en el GPIO 5 para permitir un apagado instantáneo (ESTOP) redundante por hardware.

| Motor | PWM Pin (ESP32) | Bit MCP23017 (IN1) | Bit MCP23017 (IN2) | Canal Físico / Breakout | Puerto y Pin del MCP23017 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Motor 0** | GPIO 2 | Bit 0 | Bit 1 | Breakout 1 - Canal A | GPA0 (Pin 21) y GPA1 (Pin 22) |
| **Motor 1** | GPIO 12 | Bit 2 | Bit 3 | Breakout 1 - Canal B | GPA2 (Pin 23) y GPA3 (Pin 24) |
| **Motor 2** | GPIO 14 | Bit 4 | Bit 5 | Breakout 2 - Canal A | GPA4 (Pin 25) y GPA5 (Pin 26) |
| **Motor 3** | GPIO 23 | Bit 6 | Bit 7 | Breakout 2 - Canal B | GPA6 (Pin 27) y GPA7 (Pin 28) |
| **Motor 4** | GPIO 27 | Bit 8 | Bit 9 | Breakout 3 - Canal A | GPB0 (Pin 1) y GPB1 (Pin 2) |
| **Motor 5** | GPIO 18 | Bit 10 | Bit 11 | Breakout 3 - Canal B | GPB2 (Pin 3) y GPB3 (Pin 4) |
| **Motor 6** | GPIO 19 | Bit 12 | Bit 13 | Breakout 4 - Canal A | GPB4 (Pin 5) y GPB5 (Pin 6) |
| **Motor 7** | GPIO 25 | Bit 14 | Bit 15 | Breakout 4 - Canal B | GPB6 (Pin 7) y GPB7 (Pin 8) |
| **STBY (Unificado)** | GPIO 5 | - | - | Desactivación global | Controla STBY de los 4 breakouts |

- **I2C**: SDA = GPIO 21, SCL = GPIO 22 (Siempre activo para el control del MCP23017).
- **UART2**: RX2 = GPIO 16, TX2 = GPIO 17.

### Diagrama de Conexión de Controladores TB6612FNG y MCP23017

```text
       [ ESP32 MAIN ]                       [ MCP23017 ]
        GPIO 21 (SDA) <====================> Pin 13 (SDA)
        GPIO 22 (SCL) <====================> Pin 12 (SCL)
        GPIO 5  (STBY) ---------------------+
                                            |
       [ ALIMENTACIÓN ]                     |
        VM (12V Motor)                      |
        VCC (5V/3.3V Logic)                 |
        GND --------------------+           |
                                |           |
       +------------------------V-----------V-----------------------+
       |   Breakouts Físicos TB6612FNG (1 al 4)                     |
       |                                                            |
       |   +------------------+         +------------------+        |
       |   |   Breakout #1    |         |   Breakout #2    |        |
       |   | VM, VCC, GND     |         | VM, VCC, GND     |        |
       |   | STBY <-----------|---------| STBY <-----------|--------| (Desde GPIO 5)
       |   |                  |         |                  |        |
       |   | PWMA <--- GPIO 2 |         | PWMA <--- GPIO 14|        |
       |   | AIN1 <--- GPA0   |         | AIN1 <--- GPA4   |        |
       |   | AIN2 <--- GPA1   |         | AIN2 <--- GPA5   |        |
       |   | AO1/AO2 -> Mot 0 |         | AO1/AO2 -> Mot 2 |        |
       |   |                  |         |                  |        |
       |   | PWMB <--- GPIO 12|         | PWMB <--- GPIO 23|        |
       |   | BIN1 <--- GPA2   |         | BIN1 <--- GPA6   |        |
       |   | BIN2 <--- GPA3   |         | BIN2 <--- GPA7   |        |
       |   | BO1/BO2 -> Mot 1 |         | BO1/BO2 -> Mot 3 |        |
       |   +------------------+         +------------------+        |
       |                                                            |
       |   +------------------+         +------------------+        |
       |   |   Breakout #3    |         |   Breakout #4    |        |
       |   | VM, VCC, GND     |         | VM, VCC, GND     |        |
       |   | STBY <-----------|---------| STBY <-----------|--------| (Desde GPIO 5)
       |   |                  |         |                  |        |
       |   | PWMA <--- GPIO 27|         | PWMA <--- GPIO 19|        |
       |   | AIN1 <--- GPB0   |         | AIN1 <--- GPB4   |        |
       |   | AIN2 <--- GPB1   |         | AIN2 <--- GPB5   |        |
       |   | AO1/AO2 -> Mot 4 |         | AO1/AO2 -> Mot 6 |        |
       |   |                  |         |                  |        |
       |   | PWMB <--- GPIO 18|         | PWMB <--- GPIO 25|        |
       |   | BIN1 <--- GPB2   |         | BIN1 <--- GPB6   |        |
       |   | BIN2 <--- GPB3   |         | BIN2 <--- GPB7   |        |
       |   | BO1/BO2 -> Mot 5 |         | BO1/BO2 -> Mot 7 |        |
       |   +------------------+         +------------------+        |
       +------------------------------------------------------------+
```

---

### Tabla de Asignación de Pines - ESP32 ENCODER

| Motor | Canal Encoder A (PCNT Input) | Canal Encoder B (PCNT Input) | Unidad PCNT |
| :--- | :--- | :--- | :--- |
| **Motor 0** | GPIO 34 | GPIO 35 | PCNT 0 |
| **Motor 1** | GPIO 36 | GPIO 39 | PCNT 1 |
| **Motor 2** | GPIO 32 | GPIO 33 | PCNT 2 |
| **Motor 3** | GPIO 25 | GPIO 26 | PCNT 3 |
| **Motor 4** | GPIO 27 | GPIO 13 | PCNT 4 |
| **Motor 5** | GPIO 23 | GPIO 19 | PCNT 5 |
| **Motor 6** | GPIO 4 | GPIO 15 | PCNT 6 |
| **Motor 7** | GPIO 18 | GPIO 14 | PCNT 7 |

- **UART2**: RX2 = GPIO 16, TX2 = GPIO 17.

---

## Protocolo UART Binario y Medidas de Seguridad

La comunicación se realiza mediante tramas binarias compactas para evitar latencias. El cálculo de CRC8 se hace utilizando el polinomio **Dallas/Maxim CRC-8 (0x31)** ($x^8 + x^5 + x^4 + 1$).

### 1. Frame de Telemetría (ENCODER -> MAIN, 38 bytes, 1 kHz)
Calculado sobre los bytes 3 a 36:
- `SOF 1 & 2`: `0xAA 0xBB`
- `LENGTH`: `0x20` (32 bytes)
- `SEQ`: Contador circular de secuencia (0-255)
- `BOOT_ID`: Generado pseudoaleatoriamente (`esp_random() % 256`) al arrancar el esclavo.
- `PAYLOAD`: `int32_t[8]` con ticks crudos de encoders.
- `CRC8`: Byte final.

### 2. Frame de Simulación PWM (MAIN -> ENCODER, 37 bytes, 1 kHz, solo en `SIMULATION_MODE=1`)
Calculado sobre los bytes 3 a 35:
- `SOF 1 & 2`: `0xCC 0xDD`
- `LENGTH`: `0x20` (32 bytes)
- `SEQ`: Contador circular de secuencia (0-255)
- `PAYLOAD`: `float[8]` con salidas PWM calculadas por el PID.
- `CRC8`: Byte final.

### Medidas de Seguridad del Sistema
1. **Protección de Enlace UART (Timeout de 50 ms)**: Si el MAIN no recibe tramas válidas del ENCODER por más de 50 ms, transiciona inmediatamente a `STATE_ESTOP` por seguridad.
2. **Protección contra Reinicio del Esclavo (BOOT_ID)**: El MAIN almacena el `BOOT_ID` inicial durante el homing. Si el ENCODER se reinicia (el `BOOT_ID` cambia), el MAIN fuerza `STATE_ESTOP` para evitar saltos violentos en la estimación de posición.
3. **Resincronización de Trama por Timeout (5 ms)**: Si se pierde la sincronización de bytes dentro de una trama, el parseador se restablece a los 5 ms de inactividad para no bloquear el control.
4. **Timeout en Esclavo (50 ms)**: En modo simulación, si el esclavo deja de recibir tramas del maestro por 50 ms, apaga automáticamente todas las entradas PWM a `0.0f` para prevenir runaways inerciales.

---

## Compilación y Carga de Firmwares

El repositorio contiene dos proyectos PlatformIO independientes:

### Carga del Firmware MAIN (Maestro)
1. Conecta el ESP32 MAIN a tu computadora.
2. Entra al directorio `firmware-main/` o ábrelo en VS Code.
3. Compila y carga el firmware:
   ```bash
   pio run --target upload
   ```
4. Sube la interfaz web a la memoria flash LittleFS:
   ```bash
   pio run --target uploadfs
   ```

### Carga del Firmware ENCODER (Esclavo)
1. Conecta el ESP32 ENCODER a tu computadora.
2. Entra al directorio `firmware-encoder/` o ábrelo en VS Code.
3. Compila y carga el firmware:
   ```bash
   pio run --target upload
   ```

---

## Simulación y Validación con Python

El contrato del protocolo JSON serie del maestro para telemetría no ha sido alterado, garantizando plena compatibilidad con los scripts de python actuales en `python-sim/` (como `validation.py`, `visualizer.py` y `motor_sim.py`).

1. Habilita `-DSIMULATION_MODE=1` en los archivos `platformio.ini` de ambos proyectos.
2. Conéctalos físicamente cruzando los pines RX2 y TX2 con GND común.
3. Abre el script de validación con Python indicando el puerto de depuración USB del ESP32 MAIN:
   ```bash
   python python-sim/validation.py
   ```
