# IRLSAFETY+

**Real-time privacy protection for OBS Studio** — local OCR, custom PII keywords, YOLO detection (plates, signs, mail labels, IDs), hybrid stream delay, and a training hub. Nothing leaves your PC.

| | |
|---|---|
| **Version** | 0.7.0 |
| **Platform** | Windows 10/11 x64 |
| **OBS** | 31.x / 32.x (64-bit) |
| **Author** | [medicinalsheep](https://github.com/medicinalsheep) |
| **Contact** | jfkyt@icloud.com |
| **Support** | [GitHub Sponsors](https://github.com/sponsors/medicinalsheep?frequency=one-time&sponsor=medicinalsheep) (optional — keeps it free & local) |
| **License** | MIT ([LICENSE](LICENSE)) |

**Made in the USA** — built on older hardware, with care, and with **Grok Build (beta)** as a development contribution.  
Design, training summary, and full attribution: **[CREDITS.md](CREDITS.md)** · Third-party licenses: **[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)**

---

## Private tester install (GitHub Release)

1. Open [Releases](https://github.com/medicinalsheep/IRLSAFETY/releases) on this repo
2. Download **`IRLSAFETY+-v0.7.0-win64.zip`** from the latest release (when published)
3. Extract the folder
4. Right-click **`install-from-package.bat`** → **Run as administrator**
5. Restart OBS
6. **Docks → IRLSAFETY+ Control** — setup guide runs on first open

Or build locally:

```bat
scripts\build-windows.bat
scripts\package-v0.1.bat
```

Package output: `release\IRLSAFETY+-v0.7.0-win64\`

---

## Quick setup

1. Add **IRLSAFETY+** filter to **Display Capture** (top of filter list)
2. Defaults: **Solid Box** censor, **Mail & Labels** + **IDs** + plates/signs ON (train model for mail/ID classes)
3. Add keywords under **Custom PII List** (one per line)
4. Hover any setting for a full tooltip explanation

### Verify

- **Test Effect** ON + Solid Box → center overlay appears
- Notepad with large text on captured display → Screen Text covers it
- Custom keyword visible → partial cover at **50%** by default (Advanced)

---

## v0.7.0 highlights

| Area | What's new |
|------|------------|
| **Categories** | Mail & Shipping Labels, IDs & Licenses (4-class training path) |
| **Angled cover** | Low-poly quad censor for tilted packages (OBB-trained models) |
| **Simpler UI** | Core settings in Protection; overlap padding removed |
| **Training** | `shipping_label` + `id_document` classes, `--obb` / `-OBB` path |
| **Dock** | Session Plan + laptop/RAM-disk training guides |

### v0.7 defaults

| Setting | Default |
|---------|---------|
| License Plates / Signs / Mail / IDs | **ON** |
| Censor Style | **Solid Box** |
| Angled Cover | **ON** |
| Sensitive Numbers | **OFF** (Advanced) |
| Confidence | **0.35** |
| Scan Every N Frames | **3** |
| Stream Delay | **1.5s** baseline |
| Escalated Secure Mode | ON |

Bundled `irlsafety-detect.onnx` may still be **2-class** (plates + signs) until you retrain. See `data/models/TRAINING_SESSION.txt`.

---

## Local model training

**One command** (Python 3.10+, NVIDIA GPU recommended):

```bat
scripts\train-model.ps1 -Device 0
```

Angled labels (OBB):

```bat
scripts\train-model.ps1 -Device 0 -OBB
```

Control dock: **Train Detection Model** · **Label Images** · **Capture Frame** · **Session Plan**

Classes: `license_plate`, `street_sign`, `shipping_label`, `id_document`

Network RAM disk (optional):

```bat
set IRLSAFETY_TRAINING_ROOT=\\YOUR-PC\share\irlsafety-training
```

Guides: `data/models/TRAINING.txt`, `TRAINING_SESSION.txt`, `LAPTOP_TRAINING.txt`  
JWCOM2 GPU kit: `X:\irlsafety-training-kit` — see `JWCOM2-RUN.txt` or `START_HERE.txt`

---

## Build from source

**Requirements:** Visual Studio 2022 (C++), CMake 3.28+

```bat
scripts\build-windows.bat
scripts\install-to-obs.bat
```

---

## Maintainer: publish a release

```bat
scripts\package-v0.1.bat
scripts\zip-release.ps1
scripts\publish-release.ps1
```

---

## About the design

IRLSAFETY+ runs a **local-only** pipeline inside OBS: Windows OCR for text and keywords, YOLOv8n ONNX for objects, region tracking with hybrid stream delay, and solid-box censorship by default.

Full write-up: [CREDITS.md](CREDITS.md)

---

## Project layout

```
src/           plugin, pipeline, OCR, GPU path, geometry, hybrid delay
src/ui/        control dock, setup walkthrough
data/locale/   UI strings + hover tooltips
data/training/ YOLO scripts + class lists
scripts/       build, package, training helpers
tests/         unit tests
```