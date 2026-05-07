#include "hal/encoder.h"
int hs_encoder_open(int id, hs_encoder_t *out) { (void)id; *out = 0; return 0; }
int hs_encoder_read(hs_encoder_t e, uint16_t *out) { (void)e; *out = 0; return 0; }
