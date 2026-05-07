# apps/c3 — choices

## Purpose
ESP32-C3 system-controller binary. Always-on, owns BLE/LoRa/fault-tree/fire-control. At R0 this is a stub `main.c` that proves linkage against `hs_proto` and `hs_hal`.

## Decisions
- D-1: ESP-IDF native target (later). Host build for CI.
- D-2: Module map per code-architecture section 3.2 (later phases populate `app/`, `svc/`, `proto_glue/`).

## Known issues
- I-1: ESP-IDF target not wired at R0. Host build only.

## Interfaces
Produces: `app_c3` executable.
Depends on: `hs_proto`, `hs_hal`.
