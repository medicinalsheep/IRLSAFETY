#!/usr/bin/env python3
"""
IRLSAFETY+ — train YOLOv8n for US license plates + street signs, export ONNX for OBS.
"""

from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path


def resolve_paths(training_dir: Path, models_dir: Path) -> tuple[Path, Path, Path]:
    training_dir = training_dir.resolve()
    models_dir = models_dir.resolve()
    onnx_out = models_dir / "irlsafety-detect.onnx"
    return training_dir, models_dir, onnx_out


def validate_onnx_class_count(onnx_path: Path, max_classes: int = 8) -> None:
    try:
        import onnx
    except ImportError:
        print("Optional: pip install onnx  (for export validation)")
        return

    model = onnx.load(str(onnx_path))
    for out in model.graph.output:
        shape = [d.dim_value for d in out.type.tensor_type.shape.dim]
        if len(shape) == 3 and shape[1] > 4:
            num_classes = shape[1] - 4
            if num_classes > max_classes:
                raise RuntimeError(
                    f"Exported model has {num_classes} classes (looks like COCO). "
                    f"IRLSAFETY+ needs a custom plate/sign model with <= {max_classes} classes."
                )
            print(f"ONNX validated: {num_classes} detection class(es)")
            return


def main() -> int:
    parser = argparse.ArgumentParser(description="Train IRLSAFETY+ US detection model")
    parser.add_argument("--training-dir", type=Path, required=True)
    parser.add_argument("--models-dir", type=Path, required=True)
    parser.add_argument("--epochs", type=int, default=80)
    parser.add_argument("--imgsz", type=int, default=640)
    parser.add_argument("--batch", type=int, default=8)
    parser.add_argument("--device", default="", help="cuda device id, cpu, or 0")
    args = parser.parse_args()

    training_dir, models_dir, onnx_out = resolve_paths(args.training_dir, args.models_dir)
    dataset_yaml = training_dir / "dataset.yaml"
    if not dataset_yaml.is_file():
        print(f"Missing {dataset_yaml} — run prepare_dataset.py first")
        return 1

    try:
        from ultralytics import YOLO
    except ImportError:
        print("Install training deps:  pip install ultralytics onnx pillow huggingface_hub")
        return 1

    models_dir.mkdir(parents=True, exist_ok=True)
    print(f"Training dir: {training_dir}")
    print(f"Dataset:      {dataset_yaml}")
    print(f"Epochs:       {args.epochs}")

    model = YOLO("yolov8n.pt")
    results = model.train(
        data=str(dataset_yaml),
        epochs=args.epochs,
        imgsz=args.imgsz,
        batch=args.batch,
        device=args.device if args.device else None,
        project=str(training_dir / "runs"),
        name="irlsafety_us",
        exist_ok=True,
        pretrained=True,
        verbose=True,
    )

    best_pt = Path(results.save_dir) / "weights" / "best.pt"
    if not best_pt.is_file():
        best_pt = training_dir / "runs" / "irlsafety_us" / "weights" / "best.pt"
    if not best_pt.is_file():
        print("Training finished but best.pt not found")
        return 1

    print(f"Exporting ONNX from {best_pt}...")
    export_model = YOLO(str(best_pt))
    export_path = export_model.export(format="onnx", imgsz=args.imgsz, simplify=True)
    export_path = Path(export_path)
    if not export_path.is_file():
        print("ONNX export failed")
        return 1

    shutil.copy2(export_path, onnx_out)
    validate_onnx_class_count(onnx_out)

    print()
    print(f"Model ready: {onnx_out}")
    print("OBS install:")
    print(r'  copy "' + str(onnx_out) + r'" "%ProgramFiles%\obs-studio\data\obs-plugins\irlsafety-plus\models\"')
    print("Then reload model in IRLSAFETY+ Control dock.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())