# CDPR — Robot Paralelo Accionado por Cables (6-DOF, 8 cables)

![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Estado](https://img.shields.io/badge/estado-en%20desarrollo-yellow)
![Licencia](https://img.shields.io/badge/licencia-MIT-green)

Un **Robot Paralelo Accionado por Cables (CDPR)** de escala de prototipo con **6 grados de libertad** (X, Y, Z, Roll, Pitch, Yaw), controlado mediante 8 cables independientes. Un ESP32 calcula la cinemática inversa y controla 8 motores DC en lazo cerrado PID, todo gestionado desde una interfaz web propia con visualización 3D en vivo.

---

## ¿Qué es esto?

En lugar de brazos rígidos, este robot mueve una plataforma móvil suspendida dentro de un cubo de acero usando 8 cables de acero inoxidable de 0.3 mm de diámetro. Cambiando la longitud de cada cable de forma coordinada, la plataforma puede trasladarse y rotar libremente en los 6 grados de libertad dentro del volumen del cubo.

El proyecto incluye todo lo necesario para probarlo de principio a fin:
- Firmware embebido para el ESP32 (cinemática + control PID).
- Una página web de control con visualización 3D, servida directamente desde el propio robot.
- Un modelo de simulación en Python para validar la lógica antes de mover motores reales.

---

## Características

- **Cinemática inversa en tiempo real** calculada a bordo del ESP32 (sin depender de una PC).
- **Control PID independiente** para cada uno de los 8 motores, con torque de retención activo para mantener los cables siempre tensos.
- **Interfaz web de control** con sliders de pose (X, Y, Z, Roll, Pitch, Yaw), telemetría en vivo y visualización 3D del robot moviéndose en tiempo real (Three.js).
- **Modo de simulación integrado** en el propio firmware: puedes probar todo el sistema (cinemática, control, web) sin tener los motores conectados.
- **Modelo de simulación en Python** para pre-sintonizar el PID y validar la cinemática antes de tocar hardware.
- **Punto de acceso WiFi propio**: el robot crea su propia red, no necesita router ni conexión a internet.

---

## Hardware utilizado

| Componente | Especificación | Cantidad |
|---|---|---|
| Controlador | ESP32 (dual-core, 240 MHz) | 1 |
| Motores | DC JGA25-370, 12V, 150 RPM | 8 |
| Encoders | Hall, cuadratura, integrados en el eje del motor | 8 |
| Drivers | Puente H TB6612FNG (doble canal, 1.2A/canal) | 4 |
| Cable de izaje | Acero inoxidable, 0.3 mm de diámetro | 8 hilos |
| Estructura | Cubo de acero, 45 x 45 cm | 1 |

Cada motor usa 1 pin de PWM y 1 pin de dirección (gracias a un inversor de hardware entre `IN1`/`IN2` del driver), para reducir el uso de pines del ESP32. El pinout exacto está definido en `firmware/src/Config.h`.

---

## Arquitectura del proyecto

```
/firmware
  /src             → Cinemática, control PID, WiFi, servidor web
  /data            → Página web de control (servida vía LittleFS)
  platformio.ini
/python-sim        → Simulación, sintonización de PID, validación
README.md
LICENSE
```

El ESP32 opera de forma **autónoma**: la cinemática y el control PID corren completamente a bordo. El enlace serial con la PC es opcional, y se usa solo para validación cruzada y pruebas (no es necesario para que el robot funcione).

---

## Cómo empezar

### Requisitos
- [PlatformIO](https://platformio.org/) (extensión de VS Code o CLI)
- Python 3 con `pyserial` y `matplotlib` (solo si vas a usar los scripts de `/python-sim`)

### 1. Clonar el repositorio
```bash
git clone https://github.com/tu-usuario/tu-repositorio.git
cd tu-repositorio
```

### 2. Configurar tus credenciales WiFi
Copia el archivo de ejemplo y pon tu propio SSID/contraseña para el punto de acceso del robot:
```bash
cp firmware/src/secrets.h.example firmware/src/secrets.h
```
Edita `secrets.h` con tus propios valores antes de continuar.

### 3. Cargar el firmware al ESP32
```bash
pio run --target upload
```

### 4. Cargar la interfaz web (sistema de archivos)
```bash
pio run --target uploadfs
```

### 5. Conectarte al robot
Una vez encendido, el ESP32 crea su propia red WiFi. Conéctate desde tu celular o laptop y abre en el navegador:
```
http://192.168.4.1
```

---

## Uso

1. **Homing:** al conectarte, presiona "Inicializar Homing" con la plataforma centrada manualmente en la base del cubo. Esto calibra los encoders y aplica la pre-tensión inicial en los 8 cables.
2. **Mover la plataforma:** una vez listo, usa los sliders de posición (X, Y, Z) y rotación (Roll, Pitch, Yaw) para mover la plataforma en tiempo real.
3. **Telemetría:** el panel derecho muestra la longitud objetivo y real de cada cable, junto con su nivel de carga (PWM).
4. **Paro de emergencia:** el botón "PARO DE EMERGENCIA" detiene todos los motores de inmediato.

---

## Simulación y validación (sin hardware)

Si quieres probar la lógica del robot sin tener los motores conectados:

```bash
pip install pyserial matplotlib
```

| Script | Qué hace |
|---|---|
| `motor_sim.py` | Simula la respuesta del motor + PID para ajustar las ganancias antes de probar en hardware real |
| `visualizer.py` | Anima en 3D una trayectoria de prueba usando la misma cinemática del firmware |
| `validation.py` | Compara las longitudes de cable calculadas por el ESP32 (en modo simulación) contra el modelo en Python, para verificar que ambos coincidan |

---

## Limitaciones conocidas / Roadmap

- El hardware físico está en proceso de ensamblaje; el firmware se ha validado principalmente en modo simulación.
- El modelo actual asume que el cable se enrolla en una sola capa sobre el tambor del motor; esto se revisará una vez confirmadas las dimensiones finales del tambor.
- Próximas mejoras evaluadas: lectura de encoders por hardware (PCNT) y expansión de pines de dirección vía I2C, para liberar pines del ESP32.

---

## Licencia

Este proyecto se distribuye bajo licencia [MIT](./LICENSE).

---

## Autor

**Julio Alberto Trejo Cruz** — Ingeniería Mecatrónica, UTSJR.
Portafolio: [ingjulioalberto.qzz.io](https://ingjulioalberto.qzz.io)

¿Preguntas o sugerencias? Abre un [Issue](../../issues) en este repositorio.
