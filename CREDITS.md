# IRLSAFETY+ — Credits & Attribution

**Made in the USA** — built on older hardware, with care, and with **Grok Build (beta)** as a development contribution.

---

## Project

| | |
|---|---|
| **Repository** | [github.com/medicinalsheep/IRLSAFETY](https://github.com/medicinalsheep/IRLSAFETY) |
| **License** | [MIT](LICENSE) |

IRLSAFETY+ is free and 100% local. Issues and releases live on GitHub.

---

## What this project is

**IRLSAFETY+** is a **local-only** real-time privacy product family:

| Surface | Role |
|---------|------|
| **Windows OBS plugin** | Filter on live sources — stream/recording protection |
| **Android app** | Standalone camera preview + on-device detection (alpha) |
| **macOS OBS plugin** | Planned next — same `libirlsafety` core |
| **iOS app** | Planned after Android alpha — camera protection (no OBS on iOS) |

Nothing is sent to the cloud for inference. Training stays on your machine and your dataset.

---

## Current progress (2026-08)

| Platform | Shipped | Notes |
|----------|---------|-------|
| **Windows** | **v0.9.5** | OBS plugin; skip-reload YOLO; tray panel; virtual cam hooks; defaults tuned for **4–6 GB VRAM** (frame skip 8) |
| **Android** | **v0.9.6-dev** | CameraX + GLES; P16 front camera + 720p/1080p analysis; GitHub APK; primary tester: Samsung A53 |
| **Core** | `libirlsafety` | Shared static lib — pipeline, ONNX YOLO, region tracker, portable settings API |
| **macOS** | — | OBS plugin scaffold after v0.9.6 alignment |
| **iOS** | **I1 scaffold** | `ios/` CMake static lib + SwiftUI shell; I2+ next |

**Engineering milestones:** Android P10–P16 code complete. A53 device sign-off open for promoting `0.9.6-dev` → `0.9.6`. iOS I1 started.

---

## Roadmap — v0.9.6 (next aligned release)

**Goal:** one marketing version line across platforms before 1.0.0 — not necessarily feature parity, but consistent naming and bundled model.

| Target | Plan |
|--------|------|
| **v0.9.6** | Bump Windows + Android together when Android alpha checklist passes on A53 |
| **Windows** | Ship refreshed `irlsafety-detect.onnx` from the next local training pass; keep low-end defaults |
| **Android** | Promote `0.9.6-dev` → `0.9.6` (drop `-dev` when stable) |
| **macOS** | OBS plugin build + smoke test on Apple Silicon / Intel |
| **iOS** | **I2–I5** (preview, pixel bridge, ORT, overlay); TestFlight at I7 |

Nothing is **1.0.0** until a deliberate public launch (installer, Play Store, App Store). See `docs/VERSIONING.md`.

---

## Design (how it works)

```
Video / camera frame → libirlsafety pipeline
    ├─ YOLOv8n ONNX (plates, signs, mail labels, IDs) — DirectML / NNAPI / CPU
    ├─ OCR path (Windows Media OCR + optional child ONNX on Windows)
    ├─ Custom PII keywords + sensitive patterns
    ├─ Region tracker (persist + predict between detection frames)
    ├─ Censor compositor (solid box, blur, custom overlay, angled quad)
    ├─ Hybrid stream delay (OBS Windows)
    └─ Secure mode (hold overlays; optional frame drop)

Windows: OBS filter + control dock + tray panel
Android: CameraX preview + GLES censorship overlay
```

**Privacy-first choices:**
- Inference is local only
- Training is local (your disk, your labels, your GPU)
- Bundled guides in `data/models/` (OCR, platforms, training, PII strategy)

---

## Detection model — shipped & next training

### Shipped: `irlsafety_v07` (bundled `irlsafety-detect.onnx`)

| | |
|---|---|
| **Architecture** | YOLOv8n (Ultralytics) |
| **Classes** | `license_plate`, `street_sign`, `shipping_label`, `id_document` |
| **Data** | US bootstrap (plates/signs) + printed props (mail/ID) |
| **Training** | Warm-start; 100-epoch pass; local GPU |
| **Export** | ONNX ~12 MB via Ultralytics |
| **Metrics** | Plates/signs mAP50 ~0.99; full 4-class bundle in v0.9.4 Windows + Android APK |

### Next session: `irlsafety_v08` (local single-machine)

Capture, label, and train on one Windows PC. Set `IRLSAFETY_TRAINING_ROOT` if you want a custom dataset folder; otherwise the plugin uses `%APPDATA%\obs-studio\plugin_config\irlsafety-plus\training`.

| Phase | Work |
|-------|------|
| **0 — Props** | `data\scripts\build-props.ps1`; tape 4×6 labels; cardstock licenses |
| **A — Capture** | OBS Control dock → Capture Frame; angled mail, hand-held IDs, edge crops |
| **B — Label** | `scripts\label-images.ps1` — tight boxes on labels/cards only |
| **C — Gate** | `scripts\train-model.ps1 -RequireV07` after ingest (50+ mail, 30+ ID boxes) |
| **D — Train** | `scripts\train-model.ps1 -RequireV07 -WarmStart -SkipBootstrap -Device 0 -Epochs 100` |
| **E — Deploy** | Copy `best.onnx` → plugin `models/`; Reload Model; validate all four categories |

**v08 goals vs v07:**
- More real-world mail/ID diversity (angles, glare, partial labels, hands)
- Hard negatives (blank cardboard, desk without labels)
- Optional **OBB** pass after axis-aligned model is solid (`-OBB`)
- Export becomes the **v0.9.6** bundled model for Windows + Android

Full checklist: `data/models/TRAINING_SESSION.txt` · local GPU notes: `data/models/LAPTOP_TRAINING.txt`

---

## Development contributions

| Contribution | Notes |
|--------------|-------|
| **Project authors** | Architecture, `libirlsafety`, OBS plugin, Android app, training pipeline, US models |
| **Grok Build (beta)** | AI-assisted design, implementation, and iteration (v0.6–v0.9) |
| **OBS Plugin Template** | CMake/build scaffold ([obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate)) |
| **Community references** | Patterns informed by [obs-detect](https://github.com/occ-ai/obs-detect) and [obs-ocr](https://github.com/occ-ai/obs-ocr) |

Third-party libraries and licenses: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

Design docs: `docs/DESIGN-android-v1.md`, `docs/DESIGN-ios-v1.md`, `docs/ANDROID_ALPHA_REVIEW.md`.

---

## Hardware note

Developed and trained on **older US hardware** — CPU-only runs where GPUs were unavailable, **4–6 GB VRAM** Windows tuning, and a mid-range **Samsung A53** as the Android alpha device. Privacy tooling should not require a datacenter to get started.

*Made in the USA with old hardware, love, and Grok Build beta.*
