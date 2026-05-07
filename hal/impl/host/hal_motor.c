#include "hal/motor.h"
int hs_motor_open(int id, hs_motor_t *out) { (void)id; *out = 0; return 0; }
int hs_motor_set_pwm(hs_motor_t m, int d)   { (void)m; (void)d; return 0; }
int hs_motor_brake(hs_motor_t m)            { (void)m; return 0; }
int hs_motor_coast(hs_motor_t m)            { (void)m; return 0; }
