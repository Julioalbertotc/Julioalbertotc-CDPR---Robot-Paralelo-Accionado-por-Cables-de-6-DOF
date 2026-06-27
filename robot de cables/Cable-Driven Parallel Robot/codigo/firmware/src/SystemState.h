#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

enum SystemState {
    STATE_UNHOMED = 0,
    STATE_HOMING = 1,
    STATE_READY = 2,
    STATE_ESTOP = 3
};

#endif // SYSTEM_STATE_H
