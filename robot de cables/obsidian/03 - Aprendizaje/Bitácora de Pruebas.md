---
tags: [proyecto/cdpr, tipo/bitacora]
estado: en progreso
creado: 2026-07-16
---
relacionado: [[00 - MOC CDPR]], [[Robot Paralelo de Cables 6-DOF]]
---

# 📝 Bitácora de Pruebas

Esta bitácora sirve para registrar cronológicamente cada una de las pruebas de control, cinemática y comunicación del sistema CDPR.

---

## 🪵 Registro Histórico de Ensayos

### Plantilla de Registro (Copia esto para cada nueva prueba)
```markdown
### [ID-Prueba] - [DD/MM/AAAA] - [Título de la Prueba]
- **Objetivo**: Qué se quería comprobar.
- **Cambio Realizado**: Qué se modificó en el firmware, software de simulación o hardware.
- **Resultado Obtenido**: Comportamiento observado, logs relevantes o gráficas.
- **Estado**: Exitoso / Fallido / Parcial
- **Próximos Pasos**: Tareas derivadas de este resultado.
```

---

### [PRUEBA-01] - 16/07/2026 - Validación del Lazo de Comunicación UART Binario
- **Objetivo**: Verificar que el ESP32 MAIN y el ESP32 ENCODER transmiten datos bidireccionales de manera consistente a 1 kHz (921600 bps) y sin pérdidas por corrupción de CRC8.
- **Cambio Realizado**: Configuración del bus UART hardware y parseador de tramas binarias con cálculo de Dallas/Maxim CRC-8 en ambos ESP32.
- **Resultado Obtenido**: Las tramas se reciben de forma estable. Se verificó que al interrumpir el cable de RX el Maestro entra en `STATE_ESTOP` tras 50 ms. La resincronización automática de trama funciona en menos de 5 ms ante la inyección de bytes basura inducidos manualmente.
- **Estado**: Exitoso
- **Próximos Pasos**: Verificar el lazo cerrado PID enviando lecturas simuladas desde el simulador en Python.

---

## Enlaces Relacionados
- [[00 - MOC CDPR]]
- [[Robot Paralelo de Cables 6-DOF]]
- [[Protocolo UART Binario]]
- [[Simulador Python CDPR]]
