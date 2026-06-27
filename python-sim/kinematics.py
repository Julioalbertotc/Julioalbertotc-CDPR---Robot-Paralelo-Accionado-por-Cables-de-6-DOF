import math

POLE_POSITIONS = [
    [-22.5,-21.0, 45.0], # 0
    [-21.0,-22.5, 45.0], # 1
    [ 21.0,-22.5, 45.0], # 2
    [ 22.5,-21.0, 45.0], # 3
    [ 22.5, 21.0, 45.0], # 4
    [ 21.0, 22.5, 45.0], # 5
    [-21.0, 22.5, 45.0], # 6
    [-22.5, 21.0, 45.0]  # 7
]

ANCHOR_POSITIONS = [
    [-5.0,-4.0, 0.0], # 0 (Corner 1, Anclaje A)
    [-4.0,-5.0, 0.0], # 1 (Corner 1, Anclaje B)
    [ 4.0,-5.0, 0.0], # 2 (Corner 2, Anclaje A)
    [ 5.0,-4.0, 0.0], # 3 (Corner 2, Anclaje B)
    [ 5.0, 4.0, 0.0], # 4 (Corner 3, Anclaje A)
    [ 4.0, 5.0, 0.0], # 5 (Corner 3, Anclaje B)
    [-4.0, 5.0, 0.0], # 6 (Corner 4, Anclaje A)
    [-5.0, 4.0, 0.0]  # 7 (Corner 4, Anclaje B)
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
        ax = ANCHOR_POSITIONS[i][0]
        ay = ANCHOR_POSITIONS[i][1]
        az = ANCHOR_POSITIONS[i][2]

        qx = x + (r11 * ax + r12 * ay + r13 * az)
        qy = y + (r21 * ax + r22 * ay + r23 * az)
        qz = z + (r31 * ax + r32 * ay + r33 * az)

        dx = POLE_POSITIONS[i][0] - qx
        dy = POLE_POSITIONS[i][1] - qy
        dz = POLE_POSITIONS[i][2] - qz

        lengths.append(math.sqrt(dx**2 + dy**2 + dz**2))

    return lengths
