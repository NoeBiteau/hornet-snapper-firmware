#ifndef HS_HAL_MOTOR_H
#define HS_HAL_MOTOR_H

typedef int hs_motor_t;

int hs_motor_open(int motor_id, hs_motor_t *out);
int hs_motor_set_pwm(hs_motor_t m, int duty_signed);
int hs_motor_brake(hs_motor_t m);
int hs_motor_coast(hs_motor_t m);

#endif
