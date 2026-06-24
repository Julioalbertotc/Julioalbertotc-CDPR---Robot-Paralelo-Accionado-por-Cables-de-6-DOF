import math

# Constantes geométricas idénticas al firmware (en cm)
POLE_POSITIONS = [
    {"x": -22.5, "y": -21.0, "z": 45.0}, # 0
    {"x": -21.0, "y": -22.5, "z": 45.0}, # 1
    {"x": 21.0,  "y": -22.5, "z": 45.0}, # 2
    {"x": 22.5,  "y": -21.0, "z": 45.0}, # 3
    {"x": 22.5,  "y": 21.0,  "z": 45.0}, # 4
    {"x": 21.0,  "y": 22.5,  "z": 45.0}, # 5
    {"x": -21.0, "y": 22.5,  "z": 45.0}, # 6
    {"x": -22.5, "y": 21.0,  "z": 45.0}  # 7
]

# Convención de orden y numeración de los 8 anclajes en la plataforma (ANCHOR_POSITIONS):
# - Índices 0, 1: Esquina delantera izquierda (Corner 1) de la plataforma
# - Índices 2, 3: Esquina delantera derecha (Corner 2) de la plataforma
# - Índices 4, 5: Esquina trasera derecha (Corner 3) de la plataforma
# - Índices 6, 7: Esquina trasera izquierda (Corner 4) de la plataforma
# Esta distribución de pares de cables evita que se crucen o interfieran geométricamente.
ANCHOR_POSITIONS = [
    {"x": -5.0, "y": -4.0, "z": 0.0}, # 0 (Corner 1, Anclaje A)
    {"x": -4.0, "y": -5.0, "z": 0.0}, # 1 (Corner 1, Anclaje B)
    {"x": 4.0,  "y": -5.0, "z": 0.0}, # 2 (Corner 2, Anclaje A)
    {"x": 5.0,  "y": -4.0, "z": 0.0}, # 3 (Corner 2, Anclaje B)
    {"x": 5.0,  "y": 4.0,  "z": 0.0}, # 4 (Corner 3, Anclaje A)
    {"x": 4.0,  "y": 5.0,  "z": 0.0}, # 5 (Corner 3, Anclaje B)
    {"x": -4.0, "y": 5.0,  "z": 0.0}, # 6 (Corner 4, Anclaje A)
    {"x": -5.0, "y": 4.0,  "z": 0.0}  # 7 (Corner 4, Anclaje B)
]

def calculate_inverse_kinematics(x, y, z, roll, pitch, yaw):
    """
    Calcula la longitud teórica de cada uno de los 8 cables.
    x, y, z en cm. roll, pitch, yaw en radianes.
    Retorna una lista de 8 floats con las longitudes en cm.
    """
    cr, sr = math.cos(roll), math.sin(roll)
    cp, sp = math.cos(pitch), math.sin(pitch)
    cy, sy = math.cos(yaw), math.sin(yaw)

    # Matriz de rotación R_zyx
    r11 = cy * cp
    r12 = cy * sp * sr - sy * cr
    r13 = cy * sp * cr + sy * sr

    r21 = sy * cp
    r22 = sy * sp * sr + cy * cr
    r23 = sy * sp * cr - cy * sr

    r31 = -sp
    r32 = cp * sr
    r33 = cp * cr

    lengths = []
    for i in range(8):
        ax = ANCHOR_POSITIONS[i]["x"]
        ay = ANCHOR_POSITIONS[i]["y"]
        az = ANCHOR_POSITIONS[i]["z"]

        # Anclaje rotado y trasladado
        qx = x + (r11 * ax + r12 * ay + r13 * az)
        qy = y + (r21 * ax + r22 * ay + r23 * az)
        qz = z + (r31 * ax + r32 * ay + r33 * az)

        # Vector del cable
        dx = POLE_POSITIONS[i]["x"] - qx
        dy = POLE_POSITIONS[i]["y"] - qy
        dz = POLE_POSITIONS[i]["z"] - qz

        lengths.append(math.sqrt(dx**2 + dy**2 + dz**2))

    return lengths
