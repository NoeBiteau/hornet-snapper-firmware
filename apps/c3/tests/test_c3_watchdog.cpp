#include "watchdog.h"

#include <cassert>

int main() {
    using namespace hs::c3;

    WatchdogSupervisor wdt;
    wdt.register_task("fsm", 5000, 100);
    assert(wdt.check(200).empty());
    auto actions = wdt.check(6200);
    assert(actions.size() == 1);
    assert(actions[0] == WatchdogAction::ResetC3);

    wdt.record_task_heartbeat("fsm", 7000);
    assert(wdt.check(8000).empty());

    assert(wdt.note_rv_alive_missed().empty());
    assert(wdt.note_rv_alive_missed().empty());
    actions = wdt.note_rv_alive_missed();
    assert(actions.size() == 1);
    assert(actions[0] == WatchdogAction::PowerCycleRv);

    actions = wdt.note_brownout();
    assert(actions.size() == 1);
    assert(actions[0] == WatchdogAction::ColdBoot);
    return 0;
}
