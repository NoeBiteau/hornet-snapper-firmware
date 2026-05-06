# firmware/ — choices

## Purpose
Top-level firmware monorepo. Hosts three application binaries and the shared protocol contract.

## Decisions
- D-1: Monorepo over polyrepo. Why: shared protocol headers and HAL headers must stay in lockstep across C3, RV1106, and bridge binaries. Trade: larger checkout.
- D-2: CMake as the top-level build system. Why: native to ESP-IDF and rknn-toolkit; supports cross-compile + host targets in one tree. Trade: verbose vs single-purpose tools.
- D-3: HAL_TARGET CMake variable selects one HAL implementation per build (host | pi5 | c3 | rv1106). Why: no `#ifdef` jungle in app code. Trade: requires CMake discipline.

## Known issues
None at R0.

## Interfaces
Exposes: nothing externally. Internal subdirs export their own libraries via add_subdirectory().
Depends on: gcc 11+ for host build, ESP-IDF 5.1+ for c3 (later), Buildroot+RKNN for rv1106 (later).
