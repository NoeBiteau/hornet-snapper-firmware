# hal/ — choices

## Purpose
Hardware abstraction layer. Headers in `include/hal/` are the contract; implementations in `impl/<target>/` provide the platform-specific code.

## Decisions
- D-1: One impl per binary, selected by CMake `HAL_TARGET`. Why: forces app code to remain HW-agnostic. Trade: requires CMake discipline.
- D-2: Functions return signed int (>=0 = success) over rich error structs. Why: minimal coupling, easy to bridge to ESP-IDF/Linux conventions. Trade: less expressive errors.

## Known issues
- I-1: `host` impl at R0 is stub-only. Real motor/encoder simulation lands in the R5 plan.

## Interfaces
Exposes: `hs_hal` static library compiled from the selected impl directory.
Depends on: nothing in `host`; ESP-IDF in `c3` (later); libv4l2 + librknn in `rv1106` (later).
