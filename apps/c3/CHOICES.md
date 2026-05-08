# apps/c3 — choices

## Purpose
ESP32-C3 system-controller binary. Always-on controller for BLE/LoRa/fault-tree/fire-control. At R2 this has a C++17 host-runnable control core that exercises the C3 FSM and FIRE event path through the existing C HAL/proto ABI.

## Decisions
- D-1: ESP-IDF native target later; host build is the R2 executable and CI target.
- D-2: Hand-rolled FSM instead of Boost.SML or a generated state machine. Why: the R2 FSM is small enough to keep explicit, testable, and friendly to ESP32-C3 porting. Trade: manual discipline as states grow.
- D-3: C++17 for C3 internals, C ABI at the HAL/proto boundary. Why: requested `.cpp` state classes and simpler host test ergonomics. Trade: R4 must validate ESP-IDF C++ runtime/binary-size assumptions.
- D-4: `hs_event_t` and `HS_UART_MSG_EVENT` remain the only fire event wire truth. R2 adds no schema generator and no parallel event type.
- D-5: R2 service modules are deterministic host stubs: event bus, storage ring, command router, power governor, watchdog. Real BLE, LoRa, ESP-IDF WDT, ISR GPIO, and flash behavior land in later hardware stages.

## Known issues
- TD-028: R2 C3 FSM host core is not yet validated on ESP-IDF/C3.
- TD-029: R2 fault tree dispositions lack persistent counters and full recovery orchestration.
- TD-030: R2 storage ring is a host binary-file implementation, not flash-safe storage.
- TD-031: R2 event bus is a bounded queue, not true pub/sub or ISR-safe dispatch.
- TD-032: R2 watchdog is a mock policy, not hardware WDT integration.
- TD-033: R2 plan artifact was written after the first implementation pass; review before merge is mandatory.
- TD-034: R2 fault actions are partially side-effected; several recovery actions emit/log only.

## Interfaces
Produces: `app_c3` executable.
Depends on: `app_c3_core`, `hs_proto`, `hs_hal`.
