---
tags: [proyecto/cdpr, tipo/concepto]
estado: validado
creado: 2026-07-16
---
relacionado: [[00 - MOC CDPR]], [[Arquitectura Dual ESP32]], [[Esquema de Conexiones y Pines]]
---

# Protocolo UART Binario

## Definición
Es el protocolo de comunicación serie punto a punto a 921600 bps que conecta el ESP32 MAIN y el ESP32 ENCODER de manera compacta y de baja latencia para garantizar el control en tiempo real a 1 kHz.

---

## Estructura de las Tramas

### 1. Trama de Telemetría (ENCODER $\rightarrow$ MAIN)
Tiene una longitud fija de **38 bytes** y se transmite de forma síncrona a 1 kHz.
- **SOF 1 & 2** (2 bytes): `0xAA 0xBB` (Start of Frame)
- **LENGTH** (1 byte): `0x20` (32 bytes de payload)
- **SEQ** (1 byte): Contador circular de secuencia (0-255)
- **BOOT_ID** (1 byte): Identificador único de arranque generado aleatoriamente.
- **PAYLOAD** (32 bytes): `int32_t encoder_ticks[8]` (Ticks acumulados de encoders de los 8 motores).
- **CRC8** (1 byte): Código de redundancia cíclica calculado sobre los bytes 3 a 36 utilizando el polinomio **Dallas/Maxim (0x31)** ($x^8 + x^5 + x^4 + 1$).

### 2. Trama de Simulación PWM (MAIN $\rightarrow$ ENCODER)
Se utiliza únicamente en `SIMULATION_MODE=1` para transmitir las consignas del lazo PID del Maestro hacia el simulador dinámico del Esclavo. Su longitud es de **37 bytes**.
- **SOF 1 & 2** (2 bytes): `0xCC 0xDD`
- **LENGTH** (1 byte): `0x20` (32 bytes de payload)
- **SEQ** (1 byte): Contador de secuencia circular.
- **PAYLOAD** (32 bytes): `float motor_pwm[8]` (Consignas PWM en rango $[-255.0, 255.0]$).
- **CRC8** (1 byte): Dallas/Maxim CRC-8 calculado sobre los bytes 3 a 35.

---

## Algoritmo CRC-8 (Implementación en C++)
```cpp
uint8_t calculateCRC8(const uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31; // Polinomio Dallas/Maxim
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}
```

---

## Enlaces Relacionados
- [[00 - MOC CDPR]]
- [[Robot Paralelo de Cables 6-DOF]]
- [[Arquitectura Dual ESP32]]
- [[Esquema de Conexiones y Pines]]
