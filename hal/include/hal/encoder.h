#ifndef HS_HAL_ENCODER_H
#define HS_HAL_ENCODER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int hs_encoder_t;

int hs_encoder_open(int encoder_id, hs_encoder_t *out);
int hs_encoder_read(hs_encoder_t e, uint16_t *counts_out);

#ifdef __cplusplus
}
#endif

#endif
