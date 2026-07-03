# IRLSAFETY+

**Real-time privacy protection for OBS Studio** — local OCR, custom PII keywords, optional YOLO detection, hybrid stream delay, and a training hub for your own models. Nothing leaves your PC.

| | |
|---|---|
| **Version** | 0.6.2 |
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
2. Download **`IRLSAFETY+-v0.6.2-win64.zip`** from the latest release
3. Extract the folder
4. Right-click **`install-from-package.bat`** → **Run as administrator**
5. Restart OBS
6. **Docks → IRLSAFETY+ Control** — setup guide runs on first open

Or build locally:

```bat
scripts\build-windows.bat
scripts\package-v0.1.bat
```

Package output: `release\IRLSAFETY+-v0.6.2-win64\`

---

## Quick setup

1. Add **IRLSAFETY+** filter to **Display Capture** (top of filter list)
2. Defaults: **Screen Text** + **Custom PII** on, **Solid Box** censor
3. Add keywords under **Custom PII List** (one per line)
4. Hover any setting for a full tooltip explanation

### Verify

- **Test Effect** ON + Solid Box → center overlay appears
- Notepad with large text on captured display → Screen Text covers it
- Custom keyword visible → partial cover at **50%** by default

---

## v0.6.2 defaults

| Setting | Default |
|---------|---------|
| License Plates / Street Signs | **ON** (bundled trained model) |
| Censor Style | **Solid Box** (black) |
| Overlay Overlap | **25%** |
| Confidence | **0.35** |
| Hybrid Stream Delay | ON (**1.5s + 1.0s** auto) |
| Escalated Secure Mode | ON |

---

## Local model training (US plates + street signs)

**One command** (requires Python 3.10+):

```bat
scripts\train-model.ps1
```

Or from OBS Control dock: **Train US Model** | **Label Images** | **Capture Frame**

Pipeline downloads US bootstrap data (LISA signs + plate boxes), trains YOLOv8n locally, exports `irlsafety-detect.onnx`. Add your own GoPro/stream frames for best results. Full guide: `data/models/TRAINING.txt`

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

IRLSAFETY+ runs a **local-only** pipeline inside OBS: Windows OCR for text and keywords, YOLOv8n ONNX for US plates and signs, region tracking with hybrid stream delay, and solid-box censorship by default.

The v0.6.2 **`irlsafety-detect.onnx`** model was trained locally (40 epochs, YOLOv8n, US bootstrap data + custom labels) and exported for on-device inference — no cloud. Full write-up: [CREDITS.md](CREDITS.md).

---

## Project layout

```
src/           plugin, pipeline, OCR, GPU path, hybrid delay
src/ui/        control dock, setup walkthrough
data/locale/   UI strings + hover tooltips
scripts/       build, package, training helpers
tests/         unit tests
```