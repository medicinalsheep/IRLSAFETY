# IRLSAFETY+

**Local-only real-time privacy** — OBS plugin on Windows, standalone camera app on Android, with **macOS** and **iOS** next. YOLO detection (plates, signs, mail labels, IDs), optional OCR, custom PII keywords, hybrid stream delay, and an on-machine training hub. Nothing leaves your devices.

| | |
|---|---|
| **Windows** | **v0.9.5** (OBS plugin) |
| **Android** | **v0.9.6-dev** (alpha APK) |
| **iOS** | **I1 scaffold** (`ios/`) |
| **Next aligned** | **v0.9.6** — A53 sign-off + refreshed model |
| **License** | MIT ([LICENSE](LICENSE)) |

**Made in the USA** — built on older hardware, with care, and with **Grok Build (beta)** as a development contribution.  
Attribution and training history: **[CREDITS.md](CREDITS.md)** · Third-party: **[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)**

---

## Downloads

https://github.com/medicinalsheep/IRLSAFETY/releases

CI overview: **[docs/GITHUB.md](docs/GITHUB.md)**

| Platform | File | Install |
|----------|------|---------|
| **Windows** | `IRLSAFETY+-v0.9.5-win64.zip` | Extract → **install-from-package.bat** (admin) → restart OBS |
| **Android** | `IRLSAFETY+-0.9.6-dev-android.apk` | Sideload — **[android/TESTER.md](android/TESTER.md)** (Samsung A53) |

Local builds:

```bat
scripts\build-windows.bat
scripts\package-windows.bat
scripts\package-android-apk.bat
```

---

## Current progress

| Area | Status |
|------|--------|
| **Windows v0.9.5** | Package rename + model skip-reload; low-end defaults (frame skip 8, 4–6 GB VRAM) |
| **Android v0.9.6-dev** | P16: front camera, 720p/1080p analysis, frame skip 8, settings hot-swap |
| **`libirlsafety`** | Shared core — Windows OBS + Android NDK; iOS CMake I1 |
| **Training** | `irlsafety_v07` model bundled; **v08** is a local-only retrain (mail/ID variety) |
| **macOS** | OBS plugin — after v0.9.6 version alignment |
| **iOS** | **I1** scaffold in `ios/` — [design](docs/DESIGN-ios-v1.md) |

---

## Roadmap — v0.9.6 and beyond

**v0.9.6** is the next milestone where versions line up across shipped platforms (still pre-1.0):

1. **Android** — A53 alpha checklist complete → promote `0.9.6-dev` to **0.9.6**
2. **Windows** — ship **v0.9.6** with the new `irlsafety_v08` ONNX from a local training pass
3. **macOS** — OBS plugin build from the same `libirlsafety` + bundled model
4. **iOS** — **I2–I5** (preview, bridge, ORT, overlay) in parallel; TestFlight is I7

**1.0.0** remains a deliberate launch (installer / stores), not an automatic bump. Policy: `docs/VERSIONING.md`.

---

## Quick setup (Windows OBS)

1. Add **IRLSAFETY+** filter to **Display Capture** (top of filter list)
2. Defaults: plates, signs, mail, IDs **ON**; **Screen Text OFF**; frame skip **8**
3. Tray panel → **Reload Model** after install
4. Custom keywords under **Custom PII List** (one per line)

**Verify:** Test Effect ON → red center box · hold a shipping label or ID prop → black boxes appear.

**Existing filters:** remove and re-add the filter to pick up current defaults.

**Limits:** Detection is best-effort — missed mail/IDs can still leak until v08 data improves coverage. See `release/INSTALL-windows.txt`.

---

## Quick setup (Android alpha)

1. Install APK from Releases (`v0.9.6-dev`)
2. Grant camera · Battery → **Unrestricted** (Samsung One UI)
3. Toggle detection categories · try **Front camera** and **720p/1080p** analysis
4. See **[android/TESTER.md](android/TESTER.md)** and **[docs/ANDROID_ALPHA_REVIEW.md](docs/ANDROID_ALPHA_REVIEW.md)**

---

## v0.9.x highlights (Windows + core)

| Area | What's in today |
|------|-----------------|
| **Detection** | 4 classes — plates, signs, mail labels, IDs (`irlsafety-detect.onnx`) |
| **Performance** | Screen Text OFF by default; frame skip 8 on fresh installs (4–6 GB VRAM) |
| **Censor** | Solid box default; angled quad cover for tilted packages |
| **OBS** | Hybrid delay, secure mode, control dock, dark tray panel |
| **Portable core** | `libirlsafety` — same pipeline on Windows and Android NDK |
| **Android** | On-device YOLO + GLES overlay; no network permission |
| **Reload** | YOLO model reload skipped when path + GPU preference unchanged |

---

## Train your own model (local only)

Nothing uploads. Capture, label, and train on the same Windows machine.

| Step | Command / action |
|------|------------------|
| Setup (once) | `scripts\setup-training.ps1` |
| Optional starter data | `scripts\fetch-us-bootstrap.ps1` |
| Print props | `data\scripts\build-props.ps1` then print from `data\training\props\` |
| Capture frames | OBS Control dock → **Capture Frame** (mail angles, IDs in hand, edge crops) |
| Label | `scripts\label-images.ps1` |
| Ingest + gate | `data\scripts\ingest-captures.ps1` then `scripts\train-model.ps1 -RequireV07` (dry audit) |
| Train + export | `scripts\train-model.ps1 -RequireV07 -WarmStart -SkipBootstrap -Device 0 -Epochs 100` |
| Deploy | Copy `data\models\irlsafety-detect.onnx` into the OBS plugin `models\` folder → **Reload Model** |

**Targets for a v08-quality pass:** ≥50 `shipping_label` boxes · ≥30 `id_document` boxes · more angle/glare variety than v07.

**Optional phase 2:** OBB labels + `scripts\train-model.ps1 -OBB` for angled cover accuracy.

Shot list: **`data/models/TRAINING_SESSION.txt`**  
Local GPU notes: **`data/models/LAPTOP_TRAINING.txt`**

Default dataset folder (override with `IRLSAFETY_TRAINING_ROOT`):

`%APPDATA%\obs-studio\plugin_config\irlsafety-plus\training`

---

## Build from source (Windows)

**Requirements:** Visual Studio 2022 (C++), CMake 3.28+, OBS 31.x

```bat
scripts\build-windows.bat
scripts\install-to-obs.bat
```

---

## Releasing

**Windows:**

```bat
scripts\package-windows.bat
scripts\zip-release.ps1
scripts\publish-release.ps1
```

**Android:** Actions → **Android APK Release** → Run workflow (reads `versionName` from Gradle)

---

## Project layout

```
src/              libirlsafety core, OBS plugin, pipeline, ONNX, OCR
android/          Kotlin app + JNI (CameraX, GLES overlay)
ios/              I1 scaffold — CMake lib + SwiftUI shell
cmake/            libirlsafety + platform backends (Windows, Android, iOS, …)
data/models/      bundled ONNX, training guides, TRAINING_SESSION.txt
data/training/    YOLO scripts, props, class lists
docs/             Android/iOS design, versioning, alpha review
scripts/          build, package, and local training helpers
tests/            unit tests
```

---

## Design & docs

| Doc | Contents |
|-----|----------|
| [CREDITS.md](CREDITS.md) | Attribution, model history, roadmap |
| [docs/DESIGN-android-v1.md](docs/DESIGN-android-v1.md) | Android phases P10–P16 |
| [docs/DESIGN-ios-v1.md](docs/DESIGN-ios-v1.md) | iOS plan (I1–I7) |
| [docs/VERSIONING.md](docs/VERSIONING.md) | Pre-1.0 version policy |
| [docs/ANDROID_ALPHA_REVIEW.md](docs/ANDROID_ALPHA_REVIEW.md) | A53 checklist for 0.9.6 |
