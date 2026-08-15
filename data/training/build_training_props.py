#!/usr/bin/env python3
"""
IRLSAFETY+ — build training props from real-world reference data:
  - Official US state DMV sample/specimen images (public sources)
  - Shipping labels matching standard 4x6\" thermal carrier layouts
"""

from __future__ import annotations

import argparse
import json
import urllib.request
from io import BytesIO
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

# 4x6 inches @ 300 DPI — industry standard thermal shipping label
LABEL_W = 1200
LABEL_H = 1800
DL_PRINT_W = 1050  # ~3.5\" wide for wallet card prints


def load_font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    names = (
        ["C:/Windows/Fonts/arialbd.ttf", "C:/Windows/Fonts/segoeuib.ttf"]
        if bold
        else ["C:/Windows/Fonts/arial.ttf", "C:/Windows/Fonts/segoeui.ttf"]
    )
    for path in names:
        if Path(path).is_file():
            return ImageFont.truetype(path, size=size)
    return ImageFont.load_default()


def download_image(url: str) -> Image.Image:
    req = urllib.request.Request(url, headers={"User-Agent": "IRLSAFETY+ props builder"})
    with urllib.request.urlopen(req, timeout=60) as resp:
        data = resp.read()
    return Image.open(BytesIO(data)).convert("RGB")


def save_print_dl(img: Image.Image, out_path: Path, source_note: str) -> None:
    ratio = img.width / img.height
    height = int(DL_PRINT_W / ratio)
    resized = img.resize((DL_PRINT_W, height), Image.Resampling.LANCZOS)
    resized.save(out_path, "PNG", dpi=(300, 300))
    meta = out_path.with_suffix(".json")
    meta.write_text(json.dumps({"source": source_note, "print_width_px": DL_PRINT_W}, indent=2), encoding="utf-8")


def draw_maxicode(draw: ImageDraw.ImageDraw, cx: int, cy: int, radius: int) -> None:
    colors = [(0, 0, 0), (255, 255, 255), (0, 0, 0), (255, 255, 255)]
    r = radius
    for i, color in enumerate(colors):
        draw.ellipse((cx - r, cy - r, cx + r, cy + r), fill=color)
        r = int(r * 0.72)
    for angle in range(0, 360, 30):
        import math

        rad = math.radians(angle)
        x = cx + int(math.cos(rad) * radius * 0.85)
        y = cy + int(math.sin(rad) * radius * 0.85)
        draw.rectangle((x - 8, y - 8, x + 8, y + 8), fill=0)


def draw_code128_band(draw: ImageDraw.ImageDraw, x: int, y: int, w: int, h: int) -> None:
    bar = 4
    toggle = True
    pos = x
    while pos < x + w:
        if toggle:
            draw.rectangle((pos, y, min(pos + bar, x + w), y + h), fill=0)
        toggle = not toggle
        pos += bar + (1 if (pos % 17) < 9 else 2)


def make_ups_ground_label(out_path: Path) -> None:
    img = Image.new("RGB", (LABEL_W, LABEL_H), (255, 255, 255))
    draw = ImageDraw.Draw(img)

    draw.rectangle((0, 0, LABEL_W, 110), fill=(53, 28, 21))  # UPS brown #351C15
    draw.text((36, 28), "UPS", fill=(255, 200, 0), font=load_font(64, bold=True))
    draw.text((200, 42), "GROUND", fill=(255, 255, 255), font=load_font(40, bold=True))
    draw.text((36, 130), "SHIP TO:", fill=0, font=load_font(28, bold=True))
    ship_to = [
        "JANE SAMPLE",
        "456 COMMERCE PARK DR STE 200",
        "LOS ANGELES CA 90058-1234",
        "US",
    ]
    y = 170
    for line in ship_to:
        draw.text((36, y), line, fill=0, font=load_font(34, bold=(line == ship_to[0])))
        y += 46

    draw.text((36, 400), "TRACKING #:", fill=0, font=load_font(24, bold=True))
    draw.text((36, 440), "1Z 999 AA1 01 2345 6784", fill=0, font=load_font(52, bold=True))

    draw.text((36, 540), "BILLING: P/P", fill=0, font=load_font(22))
    draw.text((36, 580), "DESC: TRAINING SAMPLE PKG", fill=0, font=load_font(22))
    draw.text((36, 620), "REF: TRAIN-UPS-001  WT: 2.4 LBS", fill=0, font=load_font(22))

    draw.rectangle((36, 700, 420, 760), fill=(0, 0, 0))
    draw.text((52, 712), "422  J5  948", fill=(255, 255, 255), font=load_font(40, bold=True))

    draw_maxicode(draw, LABEL_W - 200, 260, 120)
    draw_code128_band(draw, 60, LABEL_H - 220, LABEL_W - 120, 120)
    draw.text((60, LABEL_H - 80), "1Z999AA10123456784", fill=0, font=load_font(28))

    img.save(out_path, "PNG", dpi=(300, 300))


def make_fedex_label(out_path: Path) -> None:
    img = Image.new("RGB", (LABEL_W, LABEL_H), (255, 255, 255))
    draw = ImageDraw.Draw(img)

    draw.rectangle((0, 0, LABEL_W, 100), fill=(77, 20, 140))
    draw.text((36, 24), "FedEx.", fill=(255, 255, 255), font=load_font(56, bold=True))
    draw.text((300, 34), "Home Delivery", fill=(255, 102, 0), font=load_font(36, bold=True))

    draw.text((36, 130), "TO", fill=0, font=load_font(30, bold=True))
    lines = ["ALEX SAMPLE", "789 PARCEL PKWY", "DALLAS TX 75201", "UNITED STATES"]
    y = 175
    for line in lines:
        draw.text((36, y), line, fill=0, font=load_font(34))
        y += 44

    draw.text((36, 380), "TRK#", fill=0, font=load_font(26, bold=True))
    draw.text((36, 420), "7489 1234 5678", fill=0, font=load_font(48, bold=True))
    draw.text((36, 500), "INV: TRAIN-FDX-002", fill=0, font=load_font(22))

    draw_code128_band(draw, 80, LABEL_H - 260, LABEL_W - 160, 140)
    draw.text((80, LABEL_H - 90), "748912345678", fill=0, font=load_font(30))
    img.save(out_path, "PNG", dpi=(300, 300))


def make_usps_priority_label(out_path: Path) -> None:
    img = Image.new("RGB", (LABEL_W, LABEL_H), (255, 255, 255))
    draw = ImageDraw.Draw(img)

    draw.rectangle((0, 0, LABEL_W, 90), fill=(200, 16, 46))
    draw.text((36, 22), "USPS PRIORITY MAIL 2-DAY", fill=(255, 255, 255), font=load_font(40, bold=True))
    draw.text((36, 120), "SHIP TO:", fill=0, font=load_font(28, bold=True))
    lines = ["RECIPIENT SAMPLE", "321 MAIL STREAM RD", "PHOENIX AZ 85001"]
    y = 165
    for line in lines:
        draw.text((36, y), line, fill=0, font=load_font(34))
        y += 44

    draw.text((36, 360), "Tracking #", fill=0, font=load_font(24, bold=True))
    draw.text((36, 400), "9400 1000 0000 0000 0000 00", fill=0, font=load_font(40, bold=True))

    draw.rectangle((36, 520, 200, 600), fill=(0, 48, 135))
    draw.text((52, 548), "PRIORITY", fill=(255, 255, 255), font=load_font(28, bold=True))

    draw_code128_band(draw, 60, LABEL_H - 240, LABEL_W - 120, 130)
    img.save(out_path, "PNG", dpi=(300, 300))


def make_amazon_label(out_path: Path) -> None:
    img = Image.new("RGB", (LABEL_W, LABEL_H), (255, 255, 255))
    draw = ImageDraw.Draw(img)

    draw.rectangle((0, 0, LABEL_W, 90), fill=(35, 47, 62))
    draw.text((36, 22), "amazon", fill=(255, 153, 0), font=load_font(52, bold=True))
    draw.text((36, 120), "Deliver to:", fill=0, font=load_font(30, bold=True))
    lines = ["Jordan Sample", "123 Training Blvd", "Seattle WA 98101"]
    y = 165
    for line in lines:
        draw.text((36, y), line, fill=0, font=load_font(34))
        y += 44

    draw.text((36, 340), "Order # TRAIN-AMZ-88421", fill=0, font=load_font(26))
    draw.text((36, 390), "Routing: DSEA / SORT CENTER B", fill=0, font=load_font(26))

    draw.rectangle((36, 470, 500, 560), fill=(35, 47, 62))
    draw.arc((70, 490, 200, 540), 200, 340, fill=(255, 153, 0), width=16)
    draw.arc((250, 490, 380, 540), 200, 340, fill=(255, 153, 0), width=16)

    draw_code128_band(draw, 60, LABEL_H - 220, LABEL_W - 120, 110)
    img.save(out_path, "PNG", dpi=(300, 300))


def make_dhl_express_label(out_path: Path) -> None:
    img = Image.new("RGB", (LABEL_W, LABEL_H), (255, 255, 255))
    draw = ImageDraw.Draw(img)

    draw.rectangle((0, 0, LABEL_W, 100), fill=(255, 204, 0))
    draw.text((36, 28), "DHL EXPRESS", fill=(200, 16, 46), font=load_font(48, bold=True))
    draw.text((36, 130), "TO:", fill=0, font=load_font(30, bold=True))
    lines = ["SAMPLE RECIPIENT", "88 COURIER WAY", "MIAMI FL 33101"]
    y = 175
    for line in lines:
        draw.text((36, y), line, fill=0, font=load_font(34))
        y += 44
    draw.text((36, 360), "Waybill: TRAIN-DHL-449201", fill=0, font=load_font(32, bold=True))
    draw_code128_band(draw, 60, LABEL_H - 230, LABEL_W - 120, 120)
    img.save(out_path, "PNG", dpi=(300, 300))


OFFICIAL_SPECIMENS = [
    (
        "dl_california_2025_sample.jpg",
        "https://upload.wikimedia.org/wikipedia/commons/1/13/Californian_sample_driver%27s_license%2C_2025.jpg",
        "California DMV official sample (2025) — Wikimedia Commons, public domain (CA public record)",
    ),
    (
        "dl_california_2019_sample.jpg",
        "https://upload.wikimedia.org/wikipedia/commons/7/79/Californian_sample_driver%27s_license%2C_c._2019.jpg",
        "California DMV official sample (c. 2019) — Wikimedia Commons, public domain",
    ),
    (
        "dl_florida_2019_sample.png",
        "https://upload.wikimedia.org/wikipedia/commons/f/f5/Florida_Driver_License.png",
        "Florida HSMV official example DL (May 2019) — Wikimedia Commons, public domain",
    ),
    (
        "dl_new_york_realid_sample.png",
        "https://dmv.ny.gov/sites/default/files/styles/wysiwyg/public/images/2022-08/realid-dl-front-875x397.png",
        "New York DMV sample REAL ID document — dmv.ny.gov official sample",
    ),
    (
        "dl_new_york_enhanced_sample.png",
        "https://dmv.ny.gov/sites/default/files/styles/wysiwyg/public/images/2022-08/enhanced-dl-front-862x392.png",
        "New York DMV sample Enhanced DL — dmv.ny.gov official sample",
    ),
    (
        "dl_washington_edl_sample.jpg",
        "https://upload.wikimedia.org/wikipedia/commons/c/cd/Washington_State_Enhanced_Driver%27s_License_-_Sample.webp",
        "Washington EDL sample — Wikimedia Commons, CC-BY-SA 4.0 (attribution required)",
    ),
]


def fetch_specimens(ids_dir: Path) -> list[dict]:
    manifest: list[dict] = []
    for filename, url, note in OFFICIAL_SPECIMENS:
        stem = Path(filename).stem
        out = ids_dir / f"{stem}_print.png"
        print(f"Downloading {filename}...")
        try:
            img = download_image(url)
            save_print_dl(img, out, note)
            manifest.append({"file": out.name, "source": note, "url": url})
            print(f"  -> {out.name}")
        except Exception as exc:
            print(f"  FAILED: {exc}")
    return manifest


def main() -> int:
    parser = argparse.ArgumentParser(description="Build IRLSAFETY+ props from real-world references")
    parser.add_argument("--out-dir", type=Path, default=Path("props"))
    args = parser.parse_args()

    out = args.out_dir.resolve()
    ids_dir = out / "driver_licenses"
    labels_dir = out / "shipping_labels"
    ids_dir.mkdir(parents=True, exist_ok=True)
    labels_dir.mkdir(parents=True, exist_ok=True)

    print("=== Official DMV specimen downloads ===")
    manifest = fetch_specimens(ids_dir)

    print("\n=== Real-format 4x6 shipping labels ===")
    labels = [
        ("label_ups_ground_4x6.png", make_ups_ground_label),
        ("label_fedex_home_4x6.png", make_fedex_label),
        ("label_usps_priority_4x6.png", make_usps_priority_label),
        ("label_amazon_4x6.png", make_amazon_label),
        ("label_dhl_express_4x6.png", make_dhl_express_label),
    ]
    for name, fn in labels:
        path = labels_dir / name
        fn(path)
        print(f"  {name}")

    (out / "SOURCES.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    (out / "PROPS_GUIDE.txt").write_text(
        """IRLSAFETY+ Training Props (real-world references)
=================================================

DRIVER LICENSES — official government SAMPLES
  Sources in SOURCES.json. These are DMV/HSMV specimen images, not synthetic art.
  Print at 100% scale on cardstock for physical capture.

  id_document class — draw tight box around the card in LabelImg.

SHIPPING LABELS — standard 4x6\" thermal layout
  Based on real UPS/FedEx/USPS/Amazon/DHL field structure (tracking, ship-to,
  routing, barcodes). Use fake tracking data only.

  shipping_label class — tight box on the printed label.

PRINT
  Labels: 4x6\" sticker paper or tape to boxes/mailers
  Licenses: cardstock, color, 100% scale

CAPTURE
  Optional: setx IRLSAFETY_TRAINING_ROOT <local-folder>
  Restart OBS → Capture Frame → images/staging

LABEL
  scripts\\label-images.ps1
  data\\scripts\\ingest-captures.ps1
  scripts\\train-model.ps1 -RequireV07
""",
        encoding="utf-8",
    )

    print(f"\nDone: {out}")
    print(f"  {len(manifest)} official DL specimens")
    print(f"  {len(labels)} shipping labels (4x6 real layout)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())