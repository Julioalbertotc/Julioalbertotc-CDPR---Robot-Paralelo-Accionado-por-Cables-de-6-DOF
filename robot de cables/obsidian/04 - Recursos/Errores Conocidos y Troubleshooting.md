---
tags: [proyecto/cdpr, tipo/recurso]
estado: validado
creado: 2026-07-16
---
relacionado: [[00 - MOC CDPR]], [[Esquema de Conexiones y Pines]], [[Control PID de Posicion]]
---

# Errores Conocidos y Troubleshooting

Guía rápida de diagnóstico para resolver fallos comunes en el sistema físico y lógico del Robot Paralelo Accionado por Cables (CDPR).

---

## 🛠️ Tabla de Síntomas, Causas y Soluciones

| Síntoma | Causa Probable | Solución |
| :--- | :--- | :--- |
| **El ESP32 MAIN entra constantemente en `STATE_ESTOP` por UART Timeout.** | 1. Pérdida física de conexión serial inter-placas.<br>2. Ausencia de tierra común (GND).<br>3. Desajuste de velocidad baudrate. | 1. Revise la continuidad del cable serial.<br>2. **Crucial**: Asegúrese de que el pin GND de ambos ESP32 esté conectado directamente entre sí.<br>3. Verifique que `INTERCONNECT_BAUD` sea `921600` en ambos firmwares. |
| **Cables flácidos (sin tensión) durante movimientos o parada.** | 1. Salida PID no alcanza a vencer fricción/gravedad.<br>2. Ausencia de torque de retención activo o umbral muy bajo. | 1. Compruebe la lógica anti-holgura en [[Control PID de Posicion]].<br>2. Si es necesario, incremente el valor de `HOLDING_TORQUE_PWM` (actualmente en 25) o sintonice la constante integral $K_i$. |
| **El ESP32 MAIN no arranca (LED indicador de encendido encendido pero sin logs en puerto serie).** | 1. Conflicto con pines de strapping en el arranque (`GPIO 12` o `GPIO 2`). | 1. Revise el [[Esquema de Conexiones y Pines]]. Compruebe que la línea `MTDI` (GPIO 12) no esté siendo forzada a HIGH por la circuitería externa al momento de energizar.<br>2. Añada una resistencia de pull-down externa si es necesario. |
| **Drift acumulativo de posición (el robot cree estar en una pose pero físicamente está desviado).** | 1. Ruido eléctrico en las señales A/B del encoder.<br>2. Falta de pull-ups en pines GPI (34, 35, 36, 39).<br>3. Variación del radio del tambor por superposición del cable. | 1. Soldar condensadores de $0.1\,\mu\text{F}$ en paralelo con las entradas de encoder para filtrar ruido de alta frecuencia.<br>2. Instalar resistencias de pull-up externas de $10\text{ k}\Omega$ en los canales del ESP32 ENCODER que usan GPIOs 34, 35, 36 y 39.<br>3. Implementar corrección matemática no lineal para el radio del tambor si el cable se enrolla sobre sí mismo. |
| **El ESP32 ENCODER se reinicia inesperadamente y causa `STATE_ESTOP` en el MAIN.** | 1. Ruido electromagnético inducido por los motores.<br>2. Caída momentánea de tensión en la línea de 5V/3.3V debido al consumo de los motores. | 1. Separe físicamente las rutas de los cables de potencia de los motores de los cables de señal de los encoders.<br>2. Instale un condensador electrolítico grande ($470\,\mu\text{F}$ o superior) cerca de la alimentación de los drivers TB6612FNG. |

---

## Enlaces Relacionados
- [[00 - MOC CDPR]]
- [[Esquema de Conexiones y Pines]]
- [[Control PID de Posicion]]
- [[Arquitectura Dual ESP32]]
