#ifndef KINEMATICS_H
#define KINEMATICS_H

#include "Config.h"

// Calcula la longitud de cada uno de los 8 cables para una pose dada (X, Y, Z en cm, R, P, Y en radianes).
// lengths es un array de salida de tamaño 8.
void calculateInverseKinematics(float x, float y, float z, float roll, float pitch, float yaw, float output_lengths[8]);

#endif // KINEMATICS_H
