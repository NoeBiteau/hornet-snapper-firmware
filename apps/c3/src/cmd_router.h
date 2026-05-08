#ifndef HS_C3_CMD_ROUTER_H
#define HS_C3_CMD_ROUTER_H

#include "app_context.h"

#include <cstdint>
#include <vector>

namespace hs::c3 {

class StateMachine;

enum class CommandId : uint8_t {
    Arm = 0x20,
    Sleep = 0x21,
    Status = 0x22,
    SetConfig = 0x23,
};

enum class CommandStatusCode : uint8_t {
    Ok,
    Stubbed,
    UnknownCommand,
};

struct CommandStatus {
    CommandId command = CommandId::Status;
    CommandStatusCode code = CommandStatusCode::UnknownCommand;
};

class CmdRouter {
public:
    explicit CmdRouter(AppContext& ctx) : ctx_(ctx) {}

    CommandStatus route(CommandId id, const std::vector<uint8_t>& payload, StateMachine& fsm);

private:
    AppContext& ctx_;
};

}  // namespace hs::c3

#endif
