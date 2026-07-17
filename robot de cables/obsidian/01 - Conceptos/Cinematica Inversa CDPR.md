---
tags: [proyecto/cdpr, tipo/concepto]
estado: validado
creado: 2026-07-16
---
relacionado: [[00 - MOC CDPR]], [[Control PID de Posicion]], [[Simulador Python CDPR]], [[Plan de Pruebas de Cinemática]]
---

# Cinemática Inversa CDPR

## Definición
La cinemática inversa determina la longitud requerida para cada uno de los 8 cables del Robot Paralelo Accionado por Cables (CDPR) a partir de una pose deseada de la plataforma móvil definida en 6 grados de libertad ($X, Y, Z, Roll, Pitch, Yaw$).

---

## Modelo Matemático y Geométrico

Dada la pose del efector final definida por su traslación $\mathbf{p} = [x, y, z]^T$ y su orientación en ángulos de Euler de Tait-Bryan $\boldsymbol{\theta} = [\phi, \theta, \psi]^T$ (Roll, Pitch, Yaw):

1. **Matriz de Rotación $\mathbf{R}$ ($R_{zyx}$)**:
   $$
   \mathbf{R} = \mathbf{R}_z(\psi) \mathbf{R}_y(\theta) \mathbf{R}_x(\phi)
   $$
   
2. **Posición de los Anclajes en el Espacio del Mundo ($\mathbf{q}_i$)**:
   Si $\mathbf{a}_i$ es la posición local del punto de anclaje del cable $i$ en la plataforma móvil:
   $$
   \mathbf{q}_i = \mathbf{p} + \mathbf{R} \mathbf{a}_i \quad \text{para } i = 0, \dots, 7
   $$

3. **Cálculo de la Longitud del Cable ($L_i$)**:
   Si $\mathbf{b}_i$ representa la posición de la polea fija de salida en el poste del marco estructural del robot:
   $$
   L_i = \|\mathbf{b}_i - \mathbf{q}_i\|_2 = \sqrt{(b_{ix} - q_{ix})^2 + (b_{iy} - q_{iy})^2 + (b_{iz} - q_{iz})^2}
   $$

---

## Geometría del Robot (Configuración Física en cm)

### Coordenadas de las Poleas Fijadas al Marco ($\mathbf{b}_i$)
```text
POLE_POSITIONS = [
    [-22.5, -21.0, 45.0], // Cable 0
    [-21.0, -22.5, 45.0], // Cable 1
    [ 21.0, -22.5, 45.0], // Cable 2
    [ 22.5, -21.0, 45.0], // Cable 3
    [ 22.5,  21.0, 45.0], // Cable 4
    [ 21.0,  22.5, 45.0], // Cable 5
    [-21.0,  22.5, 45.0], // Cable 6
    [-22.5,  21.0, 45.0]  // Cable 7
]
```

### Coordenadas de los Anclajes en la Plataforma Móvil ($\mathbf{a}_i$)
```text
ANCHOR_POSITIONS = [
    [-5.0, -4.0, 0.0], // Cable 0
    [-4.0, -5.0, 0.0], // Cable 1
    [ 4.0, -5.0, 0.0], // Cable 2
    [ 5.0, -4.0, 0.0], // Cable 3
    [ 5.0,  4.0, 0.0], // Cable 4
    [ 4.0,  5.0, 0.0], // Cable 5
    [-4.0,  5.0, 0.0], // Cable 6
    [-5.0,  4.0, 0.0]  // Cable 7
]
```

---

## Implementación en Python
La función calcula la longitud teórica de cada cable de la siguiente manera:
```python
def calculate_inverse_kinematics(x, y, z, roll, pitch, yaw):
    cr, sr = math.cos(roll), math.sin(roll)
    cp, sp = math.cos(pitch), math.sin(pitch)
    cy, sy = math.cos(yaw), math.sin(yaw)

    # Coeficientes de la matriz de rotación R
    r11, r12, r13 = cy*cp, cy*sp*sr - sy*cr, cy*sp*cr + sy*sr
    r21, r22, r23 = sy*cp, sy*sp*sr + cy*cr, sy*sp*cr - cy*sr
    r31, r32, r33 = -sp,   cp*sr,            cp*cr

    lengths = []
    for i in range(8):
        ax, ay, az = ANCHOR_POSITIONS[i]
        qx = x + (r11 * ax + r12 * ay + r13 * az)
        qy = y + (r21 * ax + r22 * ay + r23 * az)
        qz = z + (r31 * ax + r32 * ay + r33 * az)

        dx = POLE_POSITIONS[i][0] - qx
        dy = POLE_POSITIONS[i][1] - qy
        dz = POLE_POSITIONS[i][2] - qz
        lengths.append(math.sqrt(dx**2 + dy**2 + dz**2))
    return lengths
```

---

## Enlaces Relacionados
- [[00 - MOC CDPR]]
- [[Robot Paralelo de Cables 6-DOF]]
- [[Control PID de Posicion]]
- [[Simulador Python CDPR]]
- [[Plan de Pruebas de Cinemática]]
