#ifndef HS_HAL_TIME_H
#define HS_HAL_TIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t hs_time_unix(void);
uint64_t hs_time_monotonic_ms(void);

#ifdef __cplusplus
}
#endif

#endif
