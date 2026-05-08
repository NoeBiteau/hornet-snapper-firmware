#include "app_context.h"
#include "cmd_router.h"
#include "fsm/state_machine.h"

#include <cassert>

int main() {
    using namespace hs::c3;

    AppContext ctx(AppContext::Options::memory_only());
    StateMachine fsm(ctx);
    CmdRouter router(ctx);

    auto status = router.route(CommandId::Arm, {}, fsm);
    assert(status.code == CommandStatusCode::Stubbed);
    assert(status.command == CommandId::Arm);
    assert(ctx.event_bus().size() == 1);
    assert(fsm.state() == C3State::Boot);

    status = router.route(static_cast<CommandId>(0xFE), {}, fsm);
    assert(status.code == CommandStatusCode::UnknownCommand);
    assert(fsm.state() == C3State::Boot);
    assert(ctx.event_bus().size() == 1);
    return 0;
}
