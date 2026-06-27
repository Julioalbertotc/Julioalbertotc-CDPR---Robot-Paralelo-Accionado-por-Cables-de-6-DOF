# CDPR — Robot Paralelo Accionado por Cables (6-DOF, 8 cables)

![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Estado](https://img.shields.io/badge/estado-en%20desarrollo-yellow)
![Licencia](https://img.shields.io/badge/licencia-MIT-green)

Sistema de control para un **Robot Paralelo Accionado por Cables (CDPR)** de 6 grados de libertad (X, Y, Z, Roll, Pitch, Yaw), controlado por un ESP32 mediante 8 motores DC independientes en lazo cerrado PID, con interfaz web propia y modelo de simulación en Python para validar todo antes de tocar hardware real.

> ⚠️ **Estado actual: en desarrollo, hardware aún no armado.** Este README documenta lo que está confirmado en el código de este repositorio. Hay puntos marcados explícitamente como **pendientes de verificar** — no asumas que están resueltos solo porque se mencionaron en una conversación o auditoría anterior.

---

## Tabla de contenido
1. [Especificaciones de hardware (BOM)](#1-especificaciones-de-hardware-bom)
2. [Arquitectura del proyecto](#2-arquitectura-del-proyecto)
3. [Firmware ESP32](#3-firmware-esp32-firmware)
4. [Página web de control](#4-página-web-de-control-firmwaredata)
5. [Modelo de simulación en Python](#5-modelo-de-simulación-en-python-python-sim)
6. [Pendientes y riesgos conocidos](#6-pendientes-y-riesgos-conocidos)
7. [Licencia](#7-licencia)
8. [Autor](#8-autor)

---

## 1. Especificaciones de Hardware (BOM)

| Componente | Especificación | Cantidad |
|---|---|---|
| Controlador | ESP32 (dual-core, 240 MHz) | 1 |
| Motores | DC JGA25-370, 12V, 150 RPM | 8 |
| Encoders | Hall, cuadratura, integrados en el eje del motor | 8 |
| Drivers | Puente H TB6612FNG (doble canal, 1.2A/canal) | 4 |
| Cable de izaje | Acero inoxidable, **0.3 mm de diámetro** | 8 hilos |
| Estructura | Cubo de acero, 45 x 45 cm | 1 |

### Esquema eléctrico asumido
- **Reducción de pines del TB6612FNG:** `IN1`/`IN2` de cada motor están invertidos por hardware (transistor NPN o compuerta NOT, `IN2 = !IN1`), reduciendo cada motor a **1 pin de dirección + 1 pin PWM** en vez de 3.
  - ⚠️ Con este esquema se pierde el modo de freno corto del TB6612FNG (solo quedan CW/CCW + PWM=0 como aproximación de stop, no es un freno activo). El torque de retención por motores opuestos compensa esto, pero es bueno tenerlo presente.
- **Encoders:** Hall en cuadratura. Los pines GPI 34-39 del ESP32 no tienen pull-up interno — requieren resistencia de pull-up externa si se usan ahí.

> ⚠️ **PENDIENTE DE VERIFICAR contra el código real de `firmware/src/`:** en una revisión anterior se discutió mover el pin `STBY` de los TB6612FNG a un GPIO controlable (para E-stop de hardware) en vez de dejarlo fijo a 3.3V, usar el periférico **PCNT** del ESP32 para los 8 encoders en vez de interrupciones por software, y sacar las 8 líneas de dirección a un **expansor I2C (MCP23017)**. Estos cambios **no están confirmados en los archivos disponibles en este repo** (no se incluyó `Config.h` ni `MotorController.cpp` en esta revisión). No documentes ni asumas ese esquema como vigente hasta confirmar con el código fuente real cuál de las dos versiones (STBY fijo a 3.3V / STBY por GPIO+expansor I2C) está efectivamente implementada.

---

## 2. Arquitectura del Proyecto

```
/firmware          → Proyecto PlatformIO (ESP32, framework Arduino)
  /src             → Cinemática, PID, WiFi/WebSockets, servidor web
  /data            → Página web de control (servida vía LittleFS)
  platformio.ini
/python-sim        → Simulación, sintonización de PID, validación cruzada
README.md
LICENSE
.gitignore
```

El ESP32 opera de forma **autónoma**: la cinemática inversa y el lazo PID corren completamente a bordo (no dependen de la PC). El enlace serial con la PC (usado por `validation.py`) es un canal de apoyo para pruebas, telemetría y validación — no para cómputo en tiempo real.

---

## 3. Firmware ESP32 (`/firmware`)

### Modo de simulación (`SIMULATION_MODE`)
El firmware soporta un modo de simulación interno controlado por flag de compilación:
- **Activo:** no requiere hardware físico. Las señales PWM alimentan un modelo de motor DC simulado en software (resistencia, inductancia, inercia, fricción) que genera pulsos de encoder sintéticos. Permite probar cinemática, PID, web y comunicación serial con solo un ESP32 conectado por USB.
- **Desactivo:** lee encoders e impulsa los TB6612FNG con hardware real.

> ⚠️ **`platformio.ini` actualmente define un solo entorno (`esp32dev`) con `SIMULATION_MODE=1` fijo en `build_flags`.** Esto significa que para pasar a hardware real hay que editar manualmente el `.ini` y recompilar — fácil de olvidar, y con motores no autoblocantes eso importa. **Recomendación:** dividir en dos entornos nombrados, por ejemplo:
> ```ini
> [env:esp32dev_sim]
> build_flags = -DSIMULATION_MODE=1
>
> [env:esp32dev_hw]
> build_flags = -DSIMULATION_MODE=0
> ```
> Así seleccionas el modo explícitamente con `pio run -e esp32dev_hw --target upload`, sin tocar flags a mano cada vez.

### Carga del firmware
```bash
pio run --target upload          # Firmware
pio run --target uploadfs        # Página web (LittleFS) — index.html, style.css, main.js
```

---

## 4. Página Web de Control (`firmware/data/`)

El ESP32 crea su propio punto de acceso WiFi (sin router ni internet):

| Parámetro | Valor por defecto |
|---|---|
| SSID | `CDPR_Robot_AP` |
| Password | `CableRobot2026` |
| IP | `192.168.4.1` |

Conéctate a esa red y abre `http://192.168.4.1` desde cualquier celular o laptop.

### ⚠️ Corrección pendiente: Three.js debe cargarse en local, no desde CDN
`index.html` actualmente carga Three.js y OrbitControls desde CDN (`cdnjs.cloudflare.com`, `cdn.jsdelivr.net`). **Esto no va a funcionar en el AP standalone del ESP32**, porque ningún dispositivo conectado a esa red tiene salida a internet. Ya existen copias locales de ambas librerías (`three_min.js`, `OrbitControls.js`) — deben copiarse a `firmware/data/` y referenciarse en `index.html` con rutas relativas:
```html
<script src="three_min.js"></script>
<script src="OrbitControls.js"></script>
```
en vez de las URLs de CDN. Sin este cambio, la visualización 3D (y probablemente el resto de la página) no carga al usar el AP del robot sin internet.

### Características
- **Control de pose (6-DOF):** sliders de X/Y/Z (cm) y Roll/Pitch/Yaw (°), habilitados solo después de completar el Homing.
  - Rango actual configurado en la UI: X/Y ±10 cm, Z entre 10 y 35 cm, rotaciones ±30°. Estos límites son "blandos" (definidos en el HTML) — confírmalos contra el espacio de trabajo real alcanzable por los 8 cables antes de confiar en ellos como límite de seguridad.
- **Visualización 3D en vivo (Three.js):** cubo, poleas, plataforma y los 8 cables, actualizados en tiempo real vía WebSocket.
- **Telemetría:** longitud objetivo/real y carga PWM de cada uno de los 8 cables.
- **Botones de control:** Homing, Ir a Centro, Paro de Emergencia.
- **Reconexión automática** del WebSocket si se pierde la conexión.

### ⚠️ Seguridad de credenciales
El SSID/password del AP están hardcodeados en `main.js` (y probablemente también en el firmware). El `.gitignore` actual **no excluye ningún archivo de credenciales** — si subes este repo a GitHub (incluso privado), quedan expuestas. Antes de tu primer push, agrega esto a `.gitignore`:
```
# Credenciales locales (crear secrets.h / wifi_config.json a partir de un .example)
firmware/src/secrets.h
firmware/data/wifi_config.json
```
y mueve el SSID/password reales a uno de esos archivos (deja un `secrets.h.example` con valores ficticios versionado en su lugar). Si ya hiciste push con la contraseña real, cámbiala — quitarla del `.gitignore` después no la borra del historial de git.

---

## 5. Modelo de Simulación en Python (`/python-sim`)

```bash
pip install pyserial matplotlib
```

| Script | Función |
|---|---|
| `kinematics.py` | Cinemática inversa (misma fórmula que el firmware), geometría de poleas/anclajes |
| `motor_sim.py` | Simulación de motor DC + PID para pre-sintonizar Kp/Ki/Kd (`python motor_sim.py` → genera `pid_tuning_step.png`) |
| `visualizer.py` | Animación 3D (Matplotlib) de una trayectoria helicoidal de prueba, con longitudes de cable impresas en consola en cada frame |
| `validation.py` | Validación cruzada: envía poses por serial al ESP32 en `SIMULATION_MODE` y compara las longitudes calculadas en ambos lados (editar `PORT` antes de ejecutar) |

---

## 6. Pendientes y Riesgos Conocidos

- **Esquema de STBY/encoders sin confirmar:** ver advertencia en la sección 1. No dar por hecho que PCNT/expansor I2C/E-stop por GPIO están implementados sin revisar `Config.h` y `MotorController.cpp` directamente.
- **Carga de Three.js vía CDN:** rompe el control web en el AP standalone (sección 4) — corregir antes de cualquier prueba con hardware.
- **Modelo de tambor/winch asume diámetro de enrollado constante** (`drum_radius = 1.0 cm` en `motor_sim.py`). Con cable de 0.3 mm, si el recorrido necesario no cabe en una sola capa sobre el tambor, el cable se enrolla en capas superpuestas y la relación longitud-por-pulso deja de ser lineal — esto puede introducir error de posición acumulado justo en el rango de mayor recorrido. Falta confirmar si el ancho del tambor real garantiza una sola capa, o si se necesita una corrección no lineal en el firmware.
- **Sin separación de entornos `SIMULATION_MODE`** en `platformio.ini` — riesgo de compilar para el modo equivocado sin darse cuenta.

---

## 7. Licencia

Este proyecto se distribuye bajo licencia **MIT** (ver archivo [`LICENSE`](./LICENSE)). Es un placeholder razonable para un proyecto académico/portafolio — cámbiala si prefieres otra licencia antes de hacer público el repositorio.

---

## 8. Autor

**Julio** — Ingeniería Mecatrónica, UTSJR.
Portafolio: [ingjulioalberto.qzz.io](https://ingjulioalberto.qzz.io)

¿Preguntas o sugerencias? Abre un [Issue](../../issues) en este repositorio.
