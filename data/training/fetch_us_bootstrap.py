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
import json
import random
import shutil
import sys
import zipfile
from pathlib import Path, PurePosixPath
from urllib.request import urlretrieve

LISA_ZIP_URLS = [
    "http://cvrr.ucsd.edu/LISA_traffic_signs/signDatabasePublicFramesOnly.zip",
    "https://git-disl.github.io/GTDLBench/datasets/lisa_traffic_sign_dataset/signDatabasePublicFramesOnly.zip",
]
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


def coco_bbox_to_yolo(bbox: list[float], img_w: int, img_h: int) -> tuple[float, float, float, float] | None:
    if len(bbox) < 4 or img_w <= 0 or img_h <= 0:
        return None
    x, y, w, h = bbox[:4]
    if w <= 1 or h <= 1:
        return None
    xc = (x + w * 0.5) / img_w
    yc = (y + h * 0.5) / img_h
    nw = w / img_w
    nh = h / img_h
    return xc, yc, nw, nh


def import_coco_zip(zip_path: Path, images_dir: Path, labels_dir: Path, class_id: int, max_images: int,
                    prefix: str, imported_so_far: int) -> int:
    """Import Roboflow-style COCO zip (image + _annotations.coco.json per archive)."""
    imported = 0
    if not zip_path.is_file():
        return 0

    print(f"Importing COCO zip: {zip_path.name}")
    with zipfile.ZipFile(zip_path, "r") as zf:
        if "_annotations.coco.json" not in zf.namelist():
            print(f"  Skipped {zip_path.name} — no _annotations.coco.json")
            return 0

        coco = json.loads(zf.read("_annotations.coco.json").decode("utf-8"))
        images = {item["id"]: item for item in coco.get("images", [])}
        anns_by_image: dict[int, list[dict]] = {}
        for ann in coco.get("annotations", []):
            anns_by_image.setdefault(ann["image_id"], []).append(ann)

        for image_id, meta in images.items():
            if imported_so_far + imported >= max_images:
                break

            file_name = meta.get("file_name")
            if not file_name or file_name not in zf.namelist():
                continue

            img_w = int(meta.get("width") or 0)
            img_h = int(meta.get("height") or 0)
            lines: list[str] = []
            for ann in anns_by_image.get(image_id, []):
                yolo = coco_bbox_to_yolo(ann.get("bbox", []), img_w, img_h)
                if yolo:
                    lines.append(yolo_line(class_id, *yolo))

            if not lines:
                continue

            stem = f"{prefix}_{Path(file_name).stem}"
            suffix = Path(file_name).suffix.lower() or ".jpg"
            out_img = images_dir / f"{stem}{suffix}"
            out_lbl = labels_dir / f"{stem}.txt"
            with zf.open(file_name) as src, out_img.open("wb") as dst:
                shutil.copyfileobj(src, dst)
            write_label(out_lbl, lines)
            imported += 1
            if imported % 200 == 0:
                print(f"  {prefix} imported: {imported}")

    print(f"  {prefix} imported: {imported}")
    return imported


def import_hf_plates(training_dir: Path, images_dir: Path, labels_dir: Path, max_images: int) -> int:
    """Import keremberke/license-plate-object-detection COCO zips (local cache)."""
    try:
        from huggingface_hub import hf_hub_download
    except ImportError:
        print("Optional: pip install huggingface_hub  (for plate bootstrap download)")
        return 0

    cache = training_dir / "raw" / "hf_plates" / "data"
    cache.mkdir(parents=True, exist_ok=True)
    repo_id = "keremberke/license-plate-object-detection"

    imported = 0
    for split, remote_name, local_name in (
        ("train", "data/train.zip", "train.zip"),
        ("valid", "data/valid.zip", "valid.zip"),
    ):
        zip_path = cache / local_name
        if not zip_path.is_file():
            print(f"Downloading HuggingFace {remote_name}...")
            try:
                downloaded = hf_hub_download(repo_id=repo_id, repo_type="dataset", filename=remote_name,
                                             local_dir=cache.parent)
                zip_path = Path(downloaded)
            except Exception as exc:  # noqa: BLE001
                print(f"Plate zip download failed ({remote_name}): {exc}")
                continue

        imported += import_coco_zip(zip_path, images_dir, labels_dir, CLASS_LICENSE_PLATE,
                                    max_images, "plate", imported)

    return imported


def import_hf_signs(training_dir: Path, images_dir: Path, labels_dir: Path, max_images: int) -> int:
    """Import ayoubsa/Sign_Road_Detection_Dataset (Roboflow YOLO zips, remapped to street_sign)."""
    try:
        from huggingface_hub import hf_hub_download
    except ImportError:
        print("Optional: pip install huggingface_hub  (for sign bootstrap download)")
        return 0

    cache = training_dir / "raw" / "hf_signs"
    cache.mkdir(parents=True, exist_ok=True)
    repo_id = "ayoubsa/Sign_Road_Detection_Dataset"

    imported = 0
    for remote_name in ("train.zip", "valid.zip"):
        zip_path = cache / remote_name
        if not zip_path.is_file():
            print(f"Downloading HuggingFace {remote_name}...")
            try:
                downloaded = hf_hub_download(repo_id=repo_id, repo_type="dataset", filename=remote_name,
                                             local_dir=cache)
                zip_path = Path(downloaded)
            except Exception as exc:  # noqa: BLE001
                print(f"Sign zip download failed ({remote_name}): {exc}")
                continue

        print(f"Importing sign zip: {zip_path.name}")
        with zipfile.ZipFile(zip_path, "r") as zf:
            img_names = [
                n for n in zf.namelist()
                if "/images/" in n and n.lower().endswith((".jpg", ".jpeg", ".png"))
            ]
            for img_name in img_names:
                if imported >= max_images:
                    break

                lbl_name = str(PurePosixPath(img_name).with_suffix(".txt")).replace("/images/", "/labels/")
                if lbl_name not in zf.namelist():
                    continue

                raw_lines = zf.read(lbl_name).decode("utf-8").strip().splitlines()
                remapped: list[str] = []
                for line in raw_lines:
                    parts = line.strip().split()
                    if len(parts) < 5:
                        continue
                    remapped.append(f"{CLASS_STREET_SIGN} {' '.join(parts[1:5])}")

                if not remapped:
                    continue

                stem = f"sign_{PurePosixPath(img_name).stem}"
                suffix = PurePosixPath(img_name).suffix.lower()
                out_img = images_dir / f"{stem}{suffix}"
                out_lbl = labels_dir / f"{stem}.txt"
                with zf.open(img_name) as src, out_img.open("wb") as dst:
                    shutil.copyfileobj(src, dst)
                write_label(out_lbl, remapped)
                imported += 1
                if imported % 200 == 0:
                    print(f"  sign imported: {imported}")

        if imported >= max_images:
            break

    print(f"  sign imported: {imported}")
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
                last_err: Exception | None = None
                for url in LISA_ZIP_URLS:
                    try:
                        download(url, lisa_zip)
                        last_err = None
                        break
                    except Exception as exc:  # noqa: BLE001
                        last_err = exc
                if last_err is not None:
                    raise last_err
            if not lisa_extract.is_dir() or not any(lisa_extract.rglob("annotations.csv")):
                print(f"Extracting {lisa_zip.name}...")
                with zipfile.ZipFile(lisa_zip, "r") as zf:
                    zf.extractall(lisa_extract)
            sign_count = import_lisa_signs(lisa_extract, images_dir, labels_dir, args.max_signs)
        except Exception as exc:  # noqa: BLE001
            print(f"LISA download failed: {exc}")
            print("Manual fallback: download signDatabasePublicFramesOnly.zip from")
            print("  http://cvrr.ucsd.edu/LISA/lisa-traffic-sign-dataset.html")
            print(f"  extract to {lisa_extract}")

    if sign_count == 0:
        print("Falling back to HuggingFace street sign bootstrap...")
        sign_count = import_hf_signs(training_dir, images_dir, labels_dir, args.max_signs)

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