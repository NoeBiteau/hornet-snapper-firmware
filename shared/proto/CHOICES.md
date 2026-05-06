# shared/proto/ — choices

## Purpose
Canonical wire-format spec. Linked by every firmware binary. Mirrored by `android/core/proto/`. Drift here causes field bricks.

## Decisions
- D-1: Hand-mirror enforced by C-emitted golden test vectors (Approach A+ per code-architecture §4). Why: idiomatic on each platform, mechanical drift detection. Trade: requires both repos' CI to round-trip the same vectors.
- D-2: Bit-packed structs over protobuf/flatbuffers. Why: LoRa airtime budget at SF7 is 200 B per frame; framing tax unaffordable. Trade: hand-rolled serializers.
- D-3: PROTO_VERSION_MAJOR / MINOR in `version.h`. Wire change → bump → regenerate vectors → vectors/v(N+1)/.

## Known issues
None at R0.

## Interfaces
Exposes: `hs_proto` static library with `uart_frame.h`, `tilt_target.h`, `event.h`, `crc16.h`, `version.h`.
Depends on: nothing — pure C99, no HAL.
