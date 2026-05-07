#ifndef HS_HAL_TIME_H
#define HS_HAL_TIME_H

#include <stdint.h>

uint32_t hs_time_unix(void);
uint64_t hs_time_monotonic_ms(void);

#endif
