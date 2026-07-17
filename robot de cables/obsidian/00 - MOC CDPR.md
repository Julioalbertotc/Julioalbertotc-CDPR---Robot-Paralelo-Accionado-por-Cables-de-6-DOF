---
tags: [proyecto/cdpr, tipo/moc]
estado: validado
creado: 2026-07-16
relacionado: []
---

# 🗺️ MOC - Robot Paralelo de Cables 6-DOF (CDPR)

Bienvenido al panel central de navegación de tu segundo cerebro para el **Robot Paralelo Accionado por Cables (CDPR)**. Este mapa de contenido organiza los archivos de la bóveda para facilitar la consulta y es totalmente compatible con consultas de Dataview.

---

## 📌 Gestión de Proyecto
- [[Robot Paralelo de Cables 6-DOF]] — Nota principal del proyecto y resumen.
- [[Roadmap del Proyecto]] — Hitos del proyecto, objetivos y estado de tareas.
- [[Bitácora de Pruebas]] — Registro cronológico de experimentos y resultados.

## 🧠 Conceptos y Teoría
- [[Decisiones de Diseño]] — Justificación de hardware, la arquitectura dual y cantidad de cables.
- [[Cinematica Inversa CDPR]] — Ecuaciones geométricas y transformaciones de pose a longitudes de cable.
- [[Control PID de Posicion]] — Lazo de control, torque de retención activo y anti-windup.
- [[Arquitectura Dual ESP32]] — Distribución Maestro-Esclavo y comunicación en tiempo real.
- [[Protocolo UART Binario]] — Estructura de tramas seriales, CRC8 y sincronización.
- [[Glosario CDPR]] — Diccionario técnico de términos y siglas del sistema.

## 🛠️ Hardware y Recursos
- [[Especificaciones de Hardware]] — Ficha técnica consolidada de motores, poleas y controladores.
- [[Esquema de Conexiones y Pines]] — Pinout detallado de ambos ESP32, MCP23017 y TB6612FNG.
- [[Errores Conocidos y Troubleshooting]] — Guía de resolución de problemas físicos y lógicos.

## 💻 Simulación y Pruebas
- [[Simulador Python CDPR]] — Módulos en Python para visualización 3D y simulación dinámica.
- [[Plan de Pruebas de Cinemática]] — Casos de prueba para verificar el modelo matemático.

---

> [!TIP]
> Si tienes instalado el plugin **Dataview**, puedes listar todas las notas de este proyecto usando la siguiente consulta en una nota nueva:
> ```dataview
> LIST FROM #proyecto/cdpr SORT file.name ASC
> ```
