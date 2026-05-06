# Hornet Snapper — Firmware

Two binaries (`apps/c3`, `apps/rv1106`) plus a shared protocol contract in `shared/proto/` and a layered HAL (`hal/`) with multiple implementations.

See `../docs/2026-05-06-hornet-snapper-code-architecture.md` for the full architecture.

## Build (host target — for CI and dev sim)

```bash
cmake -B build -DHAL_TARGET=host
cmake --build build
ctest --test-dir build --output-on-failure
```

## Generate golden test vectors

```bash
./build/tools/gen_vectors/gen_vectors > shared/proto/vectors/v1/uart_frame.json
```

## Build for ESP32-C3 / RV1106

Documented in `apps/c3/README.md` and `apps/rv1106/README.md` once those targets are wired (later phases).
