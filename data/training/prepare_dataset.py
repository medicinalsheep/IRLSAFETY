#!/usr/bin/env python3
"""
IRLSAFETY+ — validate YOLO labels, sync image/label pairs, train/val split.
"""

from __future__ import annotations

import argparse
import random
import shutil
from pathlib import Path

IMAGE_EXTS = {".jpg", ".jpeg", ".png", ".bmp", ".webp"}


def list_images(folder: Path) -> list[Path]:
    if not folder.is_dir():
        return []
    return sorted(p for p in folder.iterdir() if p.suffix.lower() in IMAGE_EXTS)


def label_for_image(image_path: Path, labels_dir: Path) -> Path:
    return labels_dir / f"{image_path.stem}.txt"


def count_pairs(images_dir: Path, labels_dir: Path) -> tuple[int, int, int]:
    images = list_images(images_dir)
    labeled = 0
    missing = 0
    for img in images:
        if label_for_image(img, labels_dir).is_file():
            labeled += 1
        else:
            missing += 1
    return len(images), labeled, missing


def write_dataset_yaml(training_dir: Path, class_count: int = 4) -> Path:
    classes_file = training_dir / "classes.txt"
    names: dict[int, str] = {}
    if classes_file.is_file():
        for i, line in enumerate(classes_file.read_text(encoding="utf-8").splitlines()):
            name = line.strip()
            if name:
                names[i] = name
    if not names:
        names = {0: "license_plate", 1: "street_sign", 2: "document", 3: "face"}

    yaml_path = training_dir / "dataset.yaml"
    lines = [
        f"path: {training_dir.resolve().as_posix()}",
        "train: images/train",
        "val: images/val",
        "names:",
    ]
    for i in range(class_count):
        lines.append(f"  {i}: {names.get(i, f'class_{i}')}")
    yaml_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return yaml_path


def split_unassigned(training_dir: Path, val_ratio: float, seed: int) -> int:
    """Move labeled pairs from images/train only into train/val split folders."""
    src_images = training_dir / "images" / "train"
    src_labels = training_dir / "labels" / "train"
    if not src_images.is_dir():
        return 0

    pairs: list[tuple[Path, Path]] = []
    for img in list_images(src_images):
        lbl = label_for_image(img, src_labels)
        if lbl.is_file():
            pairs.append((img, lbl))

    if len(pairs) < 4:
        return len(pairs)

    random.seed(seed)
    random.shuffle(pairs)
    val_count = max(1, int(len(pairs) * val_ratio))
    val_pairs = pairs[:val_count]
    train_pairs = pairs[val_count:]

    dst_train_img = training_dir / "images" / "train"
    dst_train_lbl = training_dir / "labels" / "train"
    dst_val_img = training_dir / "images" / "val"
    dst_val_lbl = training_dir / "labels" / "val"
    for d in (dst_train_img, dst_train_lbl, dst_val_img, dst_val_lbl):
        d.mkdir(parents=True, exist_ok=True)

    # Clear val folders first (re-split).
    for folder in (dst_val_img, dst_val_lbl):
        for f in folder.iterdir():
            if f.is_file():
                f.unlink()

    for img, lbl in val_pairs:
        shutil.move(str(img), str(dst_val_img / img.name))
        shutil.move(str(lbl), str(dst_val_lbl / lbl.name))

    return len(train_pairs) + len(val_pairs)


def main() -> int:
    parser = argparse.ArgumentParser(description="Prepare IRLSAFETY+ YOLO dataset")
    parser.add_argument("--training-dir", type=Path, required=True)
    parser.add_argument("--val-ratio", type=float, default=0.2)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--min-labeled", type=int, default=20,
                        help="Minimum labeled image pairs required")
    args = parser.parse_args()

    training_dir = args.training_dir.resolve()
    for sub in ("images/train", "images/val", "labels/train", "labels/val"):
        (training_dir / sub).mkdir(parents=True, exist_ok=True)

    train_total, train_labeled, train_missing = count_pairs(
        training_dir / "images" / "train", training_dir / "labels" / "train"
    )
    val_total, val_labeled, val_missing = count_pairs(
        training_dir / "images" / "val", training_dir / "labels" / "val"
    )

    total_labeled = train_labeled + val_labeled
    if val_labeled == 0 and train_labeled >= 4:
        total_labeled = split_unassigned(training_dir, args.val_ratio, args.seed)
        train_total, train_labeled, train_missing = count_pairs(
            training_dir / "images" / "train", training_dir / "labels" / "train"
        )
        val_total, val_labeled, val_missing = count_pairs(
            training_dir / "images" / "val", training_dir / "labels" / "val"
        )
        total_labeled = train_labeled + val_labeled

    yaml_path = write_dataset_yaml(training_dir)

    print(f"Training dir: {training_dir}")
    print(f"Train images: {train_total} ({train_labeled} labeled, {train_missing} unlabeled)")
    print(f"Val images:   {val_total} ({val_labeled} labeled, {val_missing} unlabeled)")
    print(f"Dataset yaml: {yaml_path}")

    if total_labeled < args.min_labeled:
        print()
        print(f"Need at least {args.min_labeled} labeled pairs for a useful US model.")
        print("Run:  scripts\\fetch-us-bootstrap.ps1")
        print("Then label your OBS captures with:  scripts\\label-images.ps1")
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())