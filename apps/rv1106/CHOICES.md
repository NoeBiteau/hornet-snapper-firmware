# apps/rv1106 — choices

## Purpose
RV1106 vision-coprocessor binary. Linux daemon. At R0 this is a stub `main.cpp`.

## Decisions
- D-1: C++17, CMake host build at R0. Cross-compile target lands in R1.

## Known issues
- I-1: No vision pipeline at R0; just linkage validation.

## Interfaces
Produces: `app_rv1106` executable.
Depends on: `hs_proto`, `hs_hal`.
