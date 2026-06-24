# CDPR - Robot Paralelo Accionado por Cables de 6-DOF

Este repositorio contiene la arquitectura de software completa para un Robot Paralelo Accionado por Cables (CDPR) de 6 grados de libertad (X, Y, Z, Roll, Pitch, Yaw) controlado por un ESP32 utilizando 8 motores DC independientes con control en lazo cerrado por PID.

El proyecto está diseñado para ser probado e integrado tanto en simulación por software como directamente en hardware físico.

---

## Estructura del Proyecto

*   `/firmware`: Proyecto de PlatformIO (Framework Arduino) para el microcontrolador ESP32.
    *   `/src`: Código fuente C++ (cinemática, PID, simulación interna, WiFi, WebSockets y servidor web).
    *   `/data`: Archivos estáticos de la página web de control (HTML, CSS y JS con visor 3D en Three.js) cargados en la memoria LittleFS del ESP32.
    *   `platformio.ini`: Configuración del proyecto, flags de compilación y librerías necesarias.
*   `/python-sim`: Scripts en Python para modelado físico, ajuste de PID, animación 3D de trayectorias y validación de firmware.
*   `/docs`: Documentación técnica e instrucciones de uso.

---

## 1. Configuración del Hardware y Supuestos

Para lograr un control de 8 motores con encoder de cuadratura en un único ESP32 (debido al límite de pines GPIO), se implementa el siguiente esquema eléctrico:
1.  **Reducción de Pines del TB6612FNG:** Se asume que en el hardware, los canales de entrada `IN1` e `IN2` de cada motor están conectados mediante un inversor lógico (un transistor NPN o compuerta NOT), de manera que `IN2 = !IN1`. Con esto, solo se requiere **1 pin de dirección** y **1 pin PWM** por motor en lugar de 3.
2.  **Pin de Standby (`STBY`):** Los pines de standby de los drivers de potencia TB6612FNG se conectan directamente a 3.3V (siempre activos).
3.  **Encoders:** Se asumen encoders magnéticos Hall incrementales de cuadratura (2 canales por motor). Para hardware real, los motores en pines GPI (34-39) requieren resistencias de pull-up externas físicas, ya que el ESP32 no cuenta con resistencias internas de pull-up en estos pines.

---

## 2. Firmware del ESP32 (`/firmware`)

### Uso del Modo Simulación (`SIMULATION_MODE`)
En `platformio.ini`, se incluye el flag `build_flags = -DSIMULATION_MODE=1`.
*   **Si está activo (`1`):** El firmware no requiere periféricos físicos. Al recibir señales de PWM, integra en tiempo real las ecuaciones diferenciales de un motor de corriente continua (resistencia, inductancia, inercia, fricción) y simula el giro y los pulsos de encoder. Permite probar el control del robot en lazo cerrado, la visualización 3D y la conexión serial desde una ESP32 conectada por USB a la PC.
*   **Si está desactivado (`0`):** Se asocia la lectura de encoders a interrupciones reales en los pines GPIO correspondientes y se escribe la potencia PWM directo a las salidas de los TB6612FNG.

### Instrucciones de Carga
1.  **Carga del Firmware:**
    Abre el proyecto en VS Code con la extensión PlatformIO. Haz clic en **Upload** (icono de flecha) para compilar y subir el firmware al ESP32. Alternativamente, usando la terminal:
    ```bash
    pio run --target upload
    ```
2.  **Carga de la Interfaz Web (LittleFS):**
    Los archivos de la interfaz de control (`index.html`, `style.css` y `main.js`) en la carpeta `/data` deben subirse al sistema de archivos flash del ESP32. En PlatformIO en VS Code, selecciona **Project Tasks > Platform > Upload Filesystem Image** o ejecuta en la terminal:
    ```bash
    pio run --target uploadfs
    ```

---

## 3. Página Web de Control (LittleFS)

Una vez encendido, el ESP32 crea un punto de acceso Wi-Fi local propio (sin necesidad de routers externos):
*   **SSID:** `CDPR_Robot_AP`
*   **Contraseña:** `CableRobot2026`
*   **IP por Defecto:** `192.168.4.1`

Conéctate a esta red desde una laptop o celular y navega en tu browser a: `http://192.168.4.1`.

### Características de la Web:
*   **Sliders de 6-DOF:** Permite posicionar el robot en X, Y, Z (en centímetros) y Roll, Pitch, Yaw (en grados). Los sliders se habilitan automáticamente tras completar el Homing.
*   **Visor 3D en Vivo:** Implementado mediante Three.js. Dibuja el chasis en forma de cubo de 45 cm, las poleas en las esquinas, la plataforma central y las líneas que representan los cables estirándose según la pose real.
*   **Monitoreo en Tiempo Real:** Gráficas de barra con el porcentaje de carga (PWM) y valores exactos en centímetros y ticks del encoder para cada uno de los 8 motores.
*   **Lógica de Tensionado:** Ejecuta la pre-tensión inicial activa (Homing) e implementa torque de retención activo en motores opuestos (Holding Torque) para evitar que los cables queden flojos y se salgan de sus carriles.

---

## 4. Simulador en Python (`/python-sim`)

Los scripts en Python corren en la PC para calibrar y validar las matemáticas y la respuesta dinámica del control.

### Dependencias de Python:
Asegúrate de instalar los paquetes necesarios:
```bash
pip install pyserial matplotlib
```

### Scripts Disponibles:
1.  **`motor_sim.py` (Ajuste de PID):**
    Simula una respuesta al escalón de lazo cerrado para los parámetros de PID configurados. Al ejecutarse, creará la gráfica `pid_tuning_step.png` en la raíz del script mostrando el comportamiento temporal de la longitud y el voltaje.
    ```bash
    python motor_sim.py
    ```
2.  **`visualizer.py` (Animación 3D):**
    Muestra una animación tridimensional interactiva mediante Matplotlib con el robot ejecutando una trayectoria helicoidal de prueba.
    ```bash
    python visualizer.py
    ```
3.  **`validation.py` (Validación Cruzada Serial JSON):**
    Envía coordenadas y orientaciones al ESP32 (corriendo en `SIMULATION_MODE` conectado por USB) por medio del protocolo serie JSON, recoge las longitudes calculadas por el ESP32 y las compara en tiempo real contra los resultados de Python para certificar exactitud matemática.
    *(Asegúrate de editar el puerto COM en la parte superior del script).*
    ```bash
    python validation.py
    ```
