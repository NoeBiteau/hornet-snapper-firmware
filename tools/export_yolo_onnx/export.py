# firmware/tools/export_yolo_onnx/export.py
"""One-shot YOLO .pt -> .onnx exporter for the R1 Pi5 vision pipe.

Usage:
    python export.py --weights /path/to/best.pt --out /path/to/velutina_yolo11s_640.onnx
"""
import argparse
import json
from pathlib import Path
from ultralytics import YOLO

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--weights", required=True)
    ap.add_argument("--out",     required=True)
    ap.add_argument("--imgsz",   type=int, default=640)
    args = ap.parse_args()

    m = YOLO(args.weights)
    onnx_path = m.export(format="onnx", imgsz=args.imgsz, opset=12,
                         simplify=True, dynamic=False)
    src = Path(onnx_path)
    dst = Path(args.out)
    dst.parent.mkdir(parents=True, exist_ok=True)
    src.rename(dst)

    meta = {
        "imgsz": args.imgsz,
        "names": m.names,                     # {0: 'velutina', 1: 'bee', ...}
        "input": "images",                    # name in graph
        "output_layout": "yolo11_default",    # ultralytics canonical
    }
    dst.with_suffix(".json").write_text(json.dumps(meta, indent=2))
    print(f"wrote {dst} and {dst.with_suffix('.json')}")

if __name__ == "__main__":
    main()
