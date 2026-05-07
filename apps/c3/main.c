#include <stdio.h>
#include "hal/time.h"
#include "version.h"

int main(void) {
    printf("hornet-snapper-c3 boot proto v%d.%d unix=%u\n",
           HS_PROTO_VERSION_MAJOR, HS_PROTO_VERSION_MINOR,
           (unsigned)hs_time_unix());
    return 0;
}
