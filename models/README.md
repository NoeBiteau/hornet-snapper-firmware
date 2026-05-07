# models/

R1 Pi5 vision pipe uses `velutina_yolo11s_640.onnx` (FP32, 640×640). Generated from
the user's local `best.pt` with `tools/export_yolo_onnx/export.py`.

Class IDs are recorded in the side-car `velutina_yolo11s_640.json` (key `names`).
Mapping to `vision::ClassId` is done in `src/onnx_detector.cpp`.

## Why not committed to git
The .onnx is ~38 MB. Track via GitHub Releases instead. The `release-vectors` CI
workflow uploads the file alongside the proto vectors zip on tag push:

* Asset name: `velutina_yolo11s_640.onnx`
* Asset URL: `https://github.com/NoeBiteau/hornet-snapper-firmware/releases/download/<tag>/velutina_yolo11s_640.onnx`

Local builds: place the .onnx into `firmware/models/` (gitignored). CI fetches the
release asset before running E2E tests.

```bash
gh release upload v1.0.0 firmware/models/velutina_yolo11s_640.onnx --clobber
```
