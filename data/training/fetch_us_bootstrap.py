#!/usr/bin/env python3
"""
IRLSAFETY+ — download US-focused bootstrap data (local only) for plate + sign training.

Sources (public research datasets, downloaded to YOUR disk only):
  - LISA Traffic Signs (UCSD) — US road signs
  - HuggingFace license-plate-object-detection (optional) — plate boxes
"""

from __future__ import annotations

import argparse
import csv
import random
import shutil
import sys
import zipfile
from pathlib import Path
from urllib.request import urlretrieve

LISA_ZIP_URL = "http://cvrr.ucsd.edu/LISA_traffic_signs/signDatabasePublicFramesOnly.zip"
CLASS_LICENSE_PLATE = 0
CLASS_STREET_SIGN = 1
IMAGE_EXTS = {".jpg", ".jpeg", ".png", ".bmp"}


def download(url: str, dest: Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    print(f"Downloading: {url}")
    urlretrieve(url, dest)  # noqa: S310 — user-initiated training bootstrap


def yolo_line(class_id: int, x_center: float, y_center: float, w: float, h: float) -> str:
    return f"{class_id} {x_center:.6f} {y_center:.6f} {w:.6f} {h:.6f}"


def write_label(label_path: Path, lines: list[str]) -> None:
    label_path.parent.mkdir(parents=True, exist_ok=True)
    label_path.write_text("\n".join(lines) + ("\n" if lines else ""), encoding="utf-8")


def import_lisa_signs(raw_dir: Path, images_dir: Path, labels_dir: Path, max_images: int) -> int:
    """Convert LISA signDatabasePublicFramesOnly annotations to class 1 (street_sign)."""
    imported = 0
    for ann_csv in sorted(raw_dir.rglob("annotations.csv")):
        seq_dir = ann_csv.parent
        rows: list[dict[str, str]] = []
        with ann_csv.open(newline="", encoding="utf-8", errors="replace") as f:
            reader = csv.DictReader(f)
            for row in reader:
                rows.append(row)

        by_frame: dict[str, list[dict[str, str]]] = {}
        for row in rows:
            frame = row.get("Filename") or row.get("filename") or row.get("Frame") or row.get("frame")
            if not frame:
                continue
            by_frame.setdefault(str(frame), []).append(row)

        for frame_name, frame_rows in by_frame.items():
            if imported >= max_images:
                return imported

            img_path = None
            for ext in (".jpg", ".jpeg", ".png", ".JPG", ".JPEG", ".PNG"):
                candidate = seq_dir / f"{frame_name}{ext}" if not Path(frame_name).suffix else seq_dir / frame_name
                if candidate.is_file():
                    img_path = candidate
                    break
                candidate = seq_dir / frame_name
                if candidate.is_file():
                    img_path = candidate
                    break

            if not img_path:
                continue

            try:
                from PIL import Image
            except ImportError:
                print("Install pillow:  pip install pillow")
                return imported

            with Image.open(img_path) as im:
                iw, ih = im.size

            lines: list[str] = []
            for row in frame_rows:
                try:
                    x = float(row.get("xst") or row.get("X") or row.get("x") or 0)
                    y = float(row.get("yst") or row.get("Y") or row.get("y") or 0)
                    w = float(row.get("width") or row.get("Width") or row.get("w") or 0)
                    h = float(row.get("height") or row.get("Height") or row.get("h") or 0)
                except ValueError:
                    continue
                if w <= 1 or h <= 1:
                    continue
                xc = (x + w * 0.5) / iw
                yc = (y + h * 0.5) / ih
                nw = w / iw
                nh = h / ih
                if nw <= 0 or nh <= 0:
                    continue
                lines.append(yolo_line(CLASS_STREET_SIGN, xc, yc, nw, nh))

            if not lines:
                continue

            stem = f"lisa_{seq_dir.name}_{Path(frame_name).stem}"
            out_img = images_dir / f"{stem}.jpg"
            out_lbl = labels_dir / f"{stem}.txt"
            shutil.copy2(img_path, out_img)
            write_label(out_lbl, lines)
            imported += 1
            if imported % 100 == 0:
                print(f"  LISA signs imported: {imported}")

    return imported


def import_hf_plates(training_dir: Path, images_dir: Path, labels_dir: Path, max_images: int) -> int:
    """Import keremberke/license-plate-object-detection if huggingface_hub is available."""
    try:
        from huggingface_hub import snapshot_download
    except ImportError:
        print("Optional: pip install huggingface_hub  (for plate bootstrap download)")
        return 0

    cache = training_dir / "raw" / "hf_plates"
    print("Downloading HuggingFace license-plate-object-detection (local cache)...")
    try:
        repo_path = Path(
            snapshot_download(
                repo_id="keremberke/license-plate-object-detection",
                repo_type="dataset",
                local_dir=cache,
            )
        )
    except Exception as exc:  # noqa: BLE001
        print(f"Plate bootstrap skipped: {exc}")
        return 0

    imported = 0
    for split in ("train", "valid", "validation", "val", "test"):
        split_img = repo_path / split / "images"
        split_lbl = repo_path / split / "labels"
        if not split_img.is_dir():
            split_img = repo_path / "images" / split
            split_lbl = repo_path / "labels" / split
        if not split_img.is_dir():
            continue

        for img in sorted(split_img.iterdir()):
            if imported >= max_images:
                return imported
            if img.suffix.lower() not in IMAGE_EXTS:
                continue
            src_lbl = split_lbl / f"{img.stem}.txt"
            if not src_lbl.is_file():
                continue

            raw_lines = src_lbl.read_text(encoding="utf-8").splitlines()
            remapped: list[str] = []
            for line in raw_lines:
                parts = line.strip().split()
                if len(parts) < 5:
                    continue
                remapped.append(f"{CLASS_LICENSE_PLATE} {' '.join(parts[1:5])}")

            if not remapped:
                continue

            stem = f"plate_{img.stem}"
            shutil.copy2(img, images_dir / f"{stem}{img.suffix.lower()}")
            write_label(labels_dir / f"{stem}.txt", remapped)
            imported += 1

    print(f"  License plates imported: {imported}")
    return imported


def main() -> int:
    parser = argparse.ArgumentParser(description="Fetch US bootstrap training data")
    parser.add_argument("--training-dir", type=Path, required=True)
    parser.add_argument("--max-signs", type=int, default=800)
    parser.add_argument("--max-plates", type=int, default=400)
    parser.add_argument("--skip-lisa", action="store_true")
    parser.add_argument("--skip-plates", action="store_true")
    args = parser.parse_args()

    training_dir = args.training_dir.resolve()
    raw_dir = training_dir / "raw"
    images_dir = training_dir / "images" / "train"
    labels_dir = training_dir / "labels" / "train"
    images_dir.mkdir(parents=True, exist_ok=True)
    labels_dir.mkdir(parents=True, exist_ok=True)

    sign_count = 0
    plate_count = 0

    if not args.skip_lisa:
        lisa_zip = raw_dir / "lisa_signs.zip"
        lisa_extract = raw_dir / "lisa_signs"
        try:
            if not lisa_zip.is_file():
                download(LISA_ZIP_URL, lisa_zip)
            if not lisa_extract.is_dir():
                print(f"Extracting {lisa_zip.name}...")
                with zipfile.ZipFile(lisa_zip, "r") as zf:
                    zf.extractall(lisa_extract)
            sign_count = import_lisa_signs(lisa_extract, images_dir, labels_dir, args.max_signs)
        except Exception as exc:  # noqa: BLE001
            print(f"LISA download failed: {exc}")
            print("Manual fallback: download signDatabasePublicFramesOnly.zip from")
            print("  http://cvrr.ucsd.edu/LISA_traffic_signs/")
            print(f"  extract to {lisa_extract}")

    if not args.skip_plates:
        plate_count = import_hf_plates(training_dir, images_dir, labels_dir, args.max_plates)

    print()
    print(f"Bootstrap complete — signs: {sign_count}, plates: {plate_count}")
    print(f"Images: {images_dir}")
    print(f"Labels: {labels_dir}")
    if sign_count + plate_count == 0:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())