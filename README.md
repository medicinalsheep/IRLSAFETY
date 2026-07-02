# IRLSAFETY+

**Real-time privacy protection for OBS Studio** — local OCR, custom PII keywords, optional YOLO detection, hybrid stream delay, and a training hub for your own models. Nothing leaves your PC.

| | |
|---|---|
| **Version** | 0.5.3 |
| **Platform** | Windows 10/11 x64 |
| **OBS** | 31.x / 32.x (64-bit) |
| **License** | MIT ([LICENSE](LICENSE)) |

---

## Private tester install (GitHub Release)

1. Open [Releases](https://github.com/medicinalsheep/IRLSAFETY/releases) on this repo
2. Download **`IRLSAFETY+-v0.5.3-win64.zip`** from the latest release
3. Extract the folder
4. Right-click **`install-from-package.bat`** → **Run as administrator**
5. Restart OBS
6. **Docks → IRLSAFETY+ Control** — setup guide runs on first open

Or build locally:

```bat
scripts\build-windows.bat
scripts\package-v0.1.bat
```

Package output: `release\IRLSAFETY+-v0.5.3-win64\`

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

## v0.5.3 defaults

| Setting | Default |
|---------|---------|
| Overlay Overlap | **0%** (tight OCR boxes) |
| Partial PII Cover | **50%** |
| Cover While Typing | OFF |
| Escalated Secure Mode | ON |
| Hybrid Stream Delay | ON (0.5s + 0.5s auto) |

---

## Local model training

1. Open **Docks → IRLSAFETY+ Control**
2. **Capture Frame for Training** → saves to local `training/images/train`
3. Label with [LabelImg](https://github.com/HumanSignal/labelImg) (local)
4. See `data/models/TRAINING.txt` or **Open Training Guide** in the dock
5. Optional: `scripts\setup-training.ps1` and `scripts\download-detection-model.ps1`

Bundled detector: `irlsafety-detect.onnx` (US plates & signs baseline). Train your own for other regions.

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
```

Upload `release\IRLSAFETY+-v0.5.3-win64.zip` to GitHub → **Releases → New release** → tag `v0.5.3`.

---

## Project layout

```
src/           plugin, pipeline, OCR, GPU path, hybrid delay
src/ui/        control dock, setup walkthrough
data/locale/   UI strings + hover tooltips
scripts/       build, package, training helpers
tests/         unit tests
```