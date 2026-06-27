#include "Kinematics.h"
#include <math.h>

void calculateInverseKinematics(float x, float y, float z, float roll, float pitch, float yaw, float output_lengths[8]) {
    // Calcular cosenos y senos de los ángulos de rotación
    float cr = cos(roll);
    float sr = sin(roll);
    float cp = cos(pitch);
    float sp = sin(pitch);
    float cy = cos(yaw);
    float sy = sin(yaw);

    // Matriz de rotación R_zyx (Roll -> X, Pitch -> Y, Yaw -> Z)
    // R = Rz(yaw) * Ry(pitch) * Rx(roll)
    float r11 = cy * cp;
    float r12 = cy * sp * sr - sy * cr;
    float r13 = cy * sp * cr + sy * sr;

    float r21 = sy * cp;
    float r22 = sy * sp * sr + cy * cr;
    float r23 = sy * sp * cr - cy * sr;

    float r31 = -sp;
    float r32 = cp * sr;
    float r33 = cp * cr;

    // Calcular la longitud para cada uno de los 8 cables
    for (int i = 0; i < 8; i++) {
        // Posición del anclaje de la plataforma respecto a su centro
        float ax = ANCHOR_POSITIONS[i].x;
        float ay = ANCHOR_POSITIONS[i].y;
        float az = ANCHOR_POSITIONS[i].z;

        // Rotar y trasladar el anclaje de la plataforma al espacio del mundo
        float qx = x + (r11 * ax + r12 * ay + r13 * az);
        float qy = y + (r21 * ax + r22 * ay + r23 * az);
        float qz = z + (r31 * ax + r32 * ay + r33 * az);

        // Vector del cable: de la polea fija B_i al anclaje móvil Q_i
        float dx = POLE_POSITIONS[i].x - qx;
        float dy = POLE_POSITIONS[i].y - qy;
        float dz = POLE_POSITIONS[i].z - qz;

        // Longitud del cable (norma L2 del vector)
        output_lengths[i] = sqrt(dx * dx + dy * dy + dz * dz);
    }
}
