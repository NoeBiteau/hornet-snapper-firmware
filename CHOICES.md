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

## R1 — Vision pipe (2026-05-07)

- Vision lib in `firmware/vision/`. Same C++17 used by the RV1106 app target. Linked into `apps/rv1106` to keep one binary across host/Pi5/RV1106.
- FrameSource abstraction (`IFrameSource`): `VideoFileSource` (cv::VideoCapture) for tests + dev, `LibcameraSource` for Pi5 live. CMake option `HS_VISION_LIBCAMERA=ON` toggles the live path. Host CI builds without libcamera.
- IDetector abstraction with three concrete: `MockDetector` (canned scripts for tests), `OnnxDetector` (FP32 onnxruntime), and `mock://path.json` URI in the binary CLI to drive E2E tests with scripted detections without ONNX. RV1106 INT8 RKNN backend deferred to R5.
- Model: user-provided YOLO11s @ 640 (`best.pt` → `velutina_yolo11s_640.onnx`). Differs from spec lock (YOLOv11n @ 320 INT8). Logged TD-027 — acceptable for R1 dev work, must converge to spec before R5.
- Clip recording: H.264 mp4 via FFmpeg subprocess pipe. Avoids OpenCV codec ABI debt (libavcodec linkage, MSVC/MinGW codec build flags). FFmpeg is universally packaged on Pi OS.
- Event log: JSONL appended to `<data-dir>/events.jsonl`. nlohmann/json header-only.
- Default data dir: `/var/lib/hornet-snapper/`. Configurable via `--data-dir`.
- Strike zone: full-frame default; configurable via `--strike-zone "x,y;x,y;..."`. Editor UI is R3 (FR-11).
- Confirmer omits `cap voltage` and `bee overlap` gates from spec §6.1; both are R5 with real hardware + multi-class model.
- No fire path in R1. Confirmer outputs an event line + clip; FIRE_IRQ assertion is R2.
