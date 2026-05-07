#include <time.h>
#include "hal/time.h"
uint32_t hs_time_unix(void) { return (uint32_t)time(NULL); }
uint64_t hs_time_monotonic_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)(ts.tv_nsec / 1000000);
}
