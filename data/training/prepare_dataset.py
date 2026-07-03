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


def read_class_names(training_dir: Path) -> dict[int, str]:
    classes_file = training_dir / "classes.txt"
    names: dict[int, str] = {}
    if classes_file.is_file():
        for i, line in enumerate(classes_file.read_text(encoding="utf-8").splitlines()):
            name = line.strip()
            if name and not name.startswith("#"):
                names[i] = name
    if not names:
        names = {
            0: "license_plate",
            1: "street_sign",
            2: "shipping_label",
            3: "id_document",
        }
    return names


def detect_label_format(labels_dirs: list[Path]) -> str:
    """Return 'obb' when labels use 5-value YOLO-OBB lines, else 'detect'."""
    for labels_dir in labels_dirs:
        if not labels_dir.is_dir():
            continue
        for lbl in sorted(labels_dir.glob("*.txt"))[:40]:
            for line in lbl.read_text(encoding="utf-8", errors="ignore").splitlines():
                parts = line.strip().split()
                if len(parts) >= 6:
                    return "obb"
                if len(parts) >= 5:
                    return "detect"
    return "unknown"


def count_class_instances(labels_dirs: list[Path]) -> dict[int, int]:
    counts: dict[int, int] = {}
    for labels_dir in labels_dirs:
        if not labels_dir.is_dir():
            continue
        for lbl in labels_dir.glob("*.txt"):
            for line in lbl.read_text(encoding="utf-8", errors="ignore").splitlines():
                parts = line.strip().split()
                if not parts:
                    continue
                try:
                    cls_id = int(parts[0])
                except ValueError:
                    continue
                counts[cls_id] = counts.get(cls_id, 0) + 1
    return counts


def write_dataset_yaml(training_dir: Path) -> Path:
    names = read_class_names(training_dir)
    class_count = max(names.keys()) + 1 if names else 4

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


def ingest_staging(training_dir: Path) -> int:
    """Move new captures from images/staging into images/train for labeling."""
    src_images = training_dir / "images" / "staging"
    dst_images = training_dir / "images" / "train"
    src_labels = training_dir / "labels" / "staging"
    dst_labels = training_dir / "labels" / "train"

    if not src_images.is_dir():
        return 0

    dst_images.mkdir(parents=True, exist_ok=True)
    dst_labels.mkdir(parents=True, exist_ok=True)

    moved = 0
    for img in list_images(src_images):
        dest_img = dst_images / img.name
        if dest_img.exists():
            stem = img.stem
            suffix = img.suffix
            n = 1
            while dest_img.exists():
                dest_img = dst_images / f"{stem}_ingest{n}{suffix}"
                n += 1
        shutil.move(str(img), str(dest_img))
        lbl = label_for_image(img, src_labels)
        if lbl.is_file():
            dest_lbl = dst_labels / dest_img.with_suffix(".txt").name
            shutil.move(str(lbl), str(dest_lbl))
        moved += 1

    return moved


def check_v07_readiness(
    class_counts: dict[int, int],
    names: dict[int, str],
    min_shipping: int,
    min_id: int,
) -> tuple[bool, list[str]]:
    issues: list[str] = []
    shipping_id = next((k for k, v in names.items() if v == "shipping_label"), 2)
    id_doc_id = next((k for k, v in names.items() if v == "id_document"), 3)

    shipping = class_counts.get(shipping_id, 0)
    id_doc = class_counts.get(id_doc_id, 0)

    if shipping < min_shipping:
        issues.append(f"shipping_label: {shipping}/{min_shipping} boxes")
    if id_doc < min_id:
        issues.append(f"id_document: {id_doc}/{min_id} boxes")

    return len(issues) == 0, issues


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
    parser.add_argument("--ingest-staging", action="store_true",
                        help="Move images/staging captures into images/train")
    parser.add_argument("--require-v07", action="store_true",
                        help="Require minimum mail + ID box counts before train")
    parser.add_argument("--min-shipping", type=int, default=50)
    parser.add_argument("--min-id", type=int, default=30)
    args = parser.parse_args()

    training_dir = args.training_dir.resolve()
    for sub in ("images/train", "images/val", "images/staging",
                "labels/train", "labels/val", "labels/staging"):
        (training_dir / sub).mkdir(parents=True, exist_ok=True)

    if args.ingest_staging:
        ingested = ingest_staging(training_dir)
        if ingested:
            print(f"Ingested {ingested} staging capture(s) into images/train")

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
    names = read_class_names(training_dir)
    label_dirs = [training_dir / "labels" / "train", training_dir / "labels" / "val"]
    class_counts = count_class_instances(label_dirs)
    label_format = detect_label_format(label_dirs)

    staging_total = len(list_images(training_dir / "images" / "staging"))

    print(f"Training dir: {training_dir}")
    print(f"Staging:      {staging_total} unlabeled capture(s) in images/staging")
    print(f"Train images: {train_total} ({train_labeled} labeled, {train_missing} unlabeled)")
    print(f"Val images:   {val_total} ({val_labeled} labeled, {val_missing} unlabeled)")
    print(f"Dataset yaml: {yaml_path}")
    print(f"Label format: {label_format} ({'use train-model.ps1 -OBB' if label_format == 'obb' else 'standard YOLO detect'})")
    print("Per-class box counts:")
    for cls_id in sorted(class_counts.keys()):
        label = names.get(cls_id, f"class_{cls_id}")
        count = class_counts[cls_id]
        print(f"  {cls_id} {label}: {count}")
        if label == "shipping_label" and count < 50:
            print("    -> aim for 50+ shipping_label boxes before training mail detection")
        if label == "id_document" and count < 30:
            print("    -> aim for 30+ id_document boxes before training ID detection")

    if total_labeled < args.min_labeled:
        print()
        print(f"Need at least {args.min_labeled} labeled pairs for a useful US model.")
        print("Run:  scripts\\fetch-us-bootstrap.ps1")
        print("Then label your OBS captures with:  scripts\\label-images.ps1")
        return 1

    if args.require_v07:
        ready, issues = check_v07_readiness(class_counts, names, args.min_shipping, args.min_id)
        if not ready:
            print()
            print("v0.7 mail/ID requirements not met:")
            for issue in issues:
                print(f"  - {issue}")
            print("Capture + label more frames (see models/TRAINING_SESSION.txt), then re-run.")
            return 1
        print()
        print("v0.7 readiness: OK (mail + ID minimums met)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())