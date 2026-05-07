#include <cstdio>
#include "hal/time.h"
#include "version.h"

int main() {
    std::printf("hornet-snapper-rv1106 boot proto v%d.%d unix=%u\n",
                HS_PROTO_VERSION_MAJOR, HS_PROTO_VERSION_MINOR,
                (unsigned)hs_time_unix());
    return 0;
}
