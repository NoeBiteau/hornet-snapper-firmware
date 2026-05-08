#include "app_context.h"
#include "fsm/state_machine.h"

extern "C" {
#include "hal/time.h"
#include "version.h"
}

#include <cstdio>

int main() {
    using namespace hs::c3;

    AppContext ctx;
    StateMachine fsm(ctx);
    StateMachineInput input{};
    input.boot_complete = true;
    fsm.tick(input);
    input = StateMachineInput{};
    input.init_complete = true;
    fsm.tick(input);
    input = StateMachineInput{};
    input.selftest_pass = true;
    fsm.tick(input);

    std::printf("hornet-snapper-c3 boot proto v%d.%d unix=%u state=%s\n",
                HS_PROTO_VERSION_MAJOR,
                HS_PROTO_VERSION_MINOR,
                (unsigned)hs_time_unix(),
                state_name(fsm.state()));
    return 0;
}
