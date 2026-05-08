#include "cmd_router.h"

#include "fsm/state_machine.h"

namespace hs::c3 {

CommandStatus CmdRouter::route(CommandId id, const std::vector<uint8_t>& payload, StateMachine& fsm) {
    (void)payload;
    (void)fsm;
    switch (id) {
        case CommandId::Arm:
        case CommandId::Sleep:
        case CommandId::Status:
        case CommandId::SetConfig:
            ctx_.event_bus().publish(BusEvent::command(static_cast<uint8_t>(id)));
            return {id, CommandStatusCode::Stubbed};
    }
    return {id, CommandStatusCode::UnknownCommand};
}

}  // namespace hs::c3
