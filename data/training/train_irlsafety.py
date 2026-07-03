#!/usr/bin/env python3
"""
IRLSAFETY+ — train YOLOv8n for plates, signs, mail labels, and IDs; export ONNX for OBS.
"""

from __future__ import annotations

import argparse
import shutil
from pathlib import Path


def resolve_paths(training_dir: Path, models_dir: Path) -> tuple[Path, Path, Path]:
    training_dir = training_dir.resolve()
    models_dir = models_dir.resolve()
    onnx_out = models_dir / "irlsafety-detect.onnx"
    return training_dir, models_dir, onnx_out


def find_warm_start_weights(training_dir: Path, obb: bool) -> Path | None:
    """Prefer the latest successful run checkpoint for transfer learning."""
    candidates = []
    if obb:
        candidates.append(training_dir / "runs" / "irlsafety_obb" / "weights" / "best.pt")
    for name in ("irlsafety_v07", "irlsafety_us"):
        candidates.append(training_dir / "runs" / name / "weights" / "best.pt")
    candidates.append(training_dir / "runs" / "irlsafety_us" / "weights" / "last.pt")

    for path in candidates:
        if path.is_file():
            return path
    return None


def validate_onnx_class_count(onnx_path: Path, expected_classes: int = 4) -> None:
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
            if num_classes > 8:
                raise RuntimeError(
                    f"Exported model has {num_classes} classes (looks like COCO). "
                    "IRLSAFETY+ needs a custom model with <= 8 classes."
                )
            print(f"ONNX validated: {num_classes} detection class(es)")
            if num_classes < expected_classes:
                print(
                    f"  Warning: expected {expected_classes} classes for v0.7 — "
                    "add mail/ID labels and retrain if categories are missing in OBS."
                )
            return


def main() -> int:
    parser = argparse.ArgumentParser(description="Train IRLSAFETY+ US detection model")
    parser.add_argument("--training-dir", type=Path, required=True)
    parser.add_argument("--models-dir", type=Path, required=True)
    parser.add_argument("--epochs", type=int, default=100)
    parser.add_argument("--imgsz", type=int, default=640)
    parser.add_argument("--batch", type=int, default=8)
    parser.add_argument("--device", default="", help="cuda device id, cpu, or 0")
    parser.add_argument("--run-name", default="", help="Ultralytics run folder name")
    parser.add_argument("--runs-dir", type=Path, default=None,
                        help="Local runs folder (e.g. SSD on JWCOM2); default: training-dir/runs")
    parser.add_argument("--resume", type=Path, default=None,
                        help="Resume from a specific .pt checkpoint")
    parser.add_argument("--warm-start", action="store_true",
                        help="Start from previous best.pt instead of COCO-pretrained yolov8n")
    parser.add_argument(
        "--obb",
        action="store_true",
        help="Train YOLOv8n-OBB for angled labels/packages (labels must be OBB format)",
    )
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
    run_name = args.run_name or ("irlsafety_obb" if args.obb else "irlsafety_v07")
    runs_root = args.runs_dir.resolve() if args.runs_dir else training_dir / "runs"
    runs_root.mkdir(parents=True, exist_ok=True)

    weights_path: Path | None = args.resume
    if weights_path and not weights_path.is_file():
        print(f"Resume checkpoint not found: {weights_path}")
        return 1

    if not weights_path and args.warm_start:
        weights_path = find_warm_start_weights(training_dir, args.obb)

    if weights_path:
        print(f"Warm-start weights: {weights_path}")
        model = YOLO(str(weights_path))
    else:
        base_weights = "yolov8n-obb.pt" if args.obb else "yolov8n.pt"
        print(f"Base weights: {base_weights}")
        model = YOLO(base_weights)

    print(f"Training dir: {training_dir}")
    print(f"Dataset:      {dataset_yaml}")
    print(f"Runs:         {runs_root / run_name}")
    print(f"Epochs:       {args.epochs}")

    results = model.train(
        data=str(dataset_yaml),
        epochs=args.epochs,
        imgsz=args.imgsz,
        batch=args.batch,
        device=args.device if args.device else None,
        project=str(runs_root),
        name=run_name,
        exist_ok=True,
        pretrained=weights_path is None,
        verbose=True,
        cos_lr=True,
        close_mosaic=10,
        patience=25,
        degrees=12.0,
        translate=0.1,
        scale=0.5,
        mosaic=1.0,
        mixup=0.05,
        hsv_h=0.015,
        hsv_s=0.7,
        hsv_v=0.4,
    )

    best_pt = Path(results.save_dir) / "weights" / "best.pt"
    if not best_pt.is_file():
        best_pt = runs_root / run_name / "weights" / "best.pt"
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
    weights_onnx = best_pt.with_suffix(".onnx")
    shutil.copy2(export_path, weights_onnx)
    validate_onnx_class_count(onnx_out)

    print()
    print(f"Model ready: {onnx_out}")
    print("OBS install:")
    print(r'  copy "' + str(onnx_out) + r'" "%ProgramFiles%\obs-studio\data\obs-plugins\irlsafety-plus\models\"')
    print("Then reload model in IRLSAFETY+ Control dock.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())