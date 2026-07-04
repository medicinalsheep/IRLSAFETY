# IRLSAFETY+

**Local-only real-time privacy** — OBS plugin on Windows, standalone camera app on Android, with **macOS** and **iOS** next. YOLO detection (plates, signs, mail labels, IDs), optional OCR, custom PII keywords, hybrid stream delay, and an on-machine training hub. Nothing leaves your devices.

| | |
|---|---|
| **Windows** | **v0.9.4** (OBS plugin) |
| **Android** | **v0.9.5-dev** (alpha APK) |
| **Next aligned** | **v0.9.6** — consistent version across platforms + refreshed model |
| **Author** | [medicinalsheep](https://github.com/medicinalsheep) |
| **Contact** | jfkyt@icloud.com |
| **License** | MIT ([LICENSE](LICENSE)) |

**Made in the USA** — built on older hardware, with care, and with **Grok Build (beta)** as a development contribution.  
Full attribution and training history: **[CREDITS.md](CREDITS.md)** · Third-party: **[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)**

---

## Downloads

https://github.com/medicinalsheep/IRLSAFETY/releases

CI overview: **[docs/GITHUB.md](docs/GITHUB.md)**

| Platform | File | Install |
|----------|------|---------|
| **Windows** | `IRLSAFETY+-v0.9.4-win64.zip` | Extract → **install-from-package.bat** (admin) → restart OBS |
| **Android** | `IRLSAFETY+-0.9.5-dev-android.apk` | Sideload — **[android/TESTER.md](android/TESTER.md)** (Samsung A53) |

Local builds:

```bat
scripts\build-windows.bat
scripts\package-v0.1.bat
scripts\package-android-apk.bat
```

---

## Current progress

| Area | Status |
|------|--------|
| **Windows v0.9.4** | Shipped — 4-class YOLO, tray panel, virtual cam, **low-end defaults** (frame skip 8, 4–6 GB VRAM) |
| **Android v0.9.5-dev** | Alpha APK on GitHub — CameraX, GLES boxes, settings, front/rear camera toggle (P16) |
| **`libirlsafety`** | Shared core — Windows OBS + Android NDK; macOS/iOS CMake path next |
| **Training** | `irlsafety_v07` model bundled; **v08 session** planned on JWCOM2 + JWCOM4 |
| **macOS** | OBS plugin — after v0.9.6 version alignment |
| **iOS** | Camera app scaffold — after Android alpha sign-off ([design](docs/DESIGN-ios-v1.md)) |

---

## Roadmap — v0.9.6 and beyond

**v0.9.6** is the next milestone where versions line up across shipped platforms (still pre-1.0):

1. **Android** — A53 alpha checklist complete → promote `0.9.5-dev` to **0.9.6**
2. **Windows** — ship **v0.9.6** with the new `irlsafety_v08` ONNX from JWCOM training
3. **macOS** — OBS plugin build from the same `libirlsafety` + bundled model
4. **iOS** — start **I1** (Xcode + static lib) in parallel; TestFlight is later

**1.0.0** remains a deliberate launch (installer / stores), not an automatic bump. Policy: `docs/VERSIONING.md`.

---

## Quick setup (Windows OBS)

1. Add **IRLSAFETY+** filter to **Display Capture** (top of filter list)
2. Defaults: plates, signs, mail, IDs **ON**; **Screen Text OFF**; frame skip **8**
3. Tray panel → **Reload Model** after install
4. Custom keywords under **Custom PII List** (one per line)

**Verify:** Test Effect ON → red center box · hold a shipping label or ID prop → black boxes appear.

**Existing filters:** remove and re-add the filter to pick up v0.9.4 defaults.

---

## Quick setup (Android alpha)

1. Install APK from Releases (`v0.9.5-dev`)
2. Grant camera · Battery → **Unrestricted** (Samsung One UI)
3. Toggle detection categories · try **Front camera** in settings
4. See **[android/TESTER.md](android/TESTER.md)** and **[docs/ANDROID_ALPHA_REVIEW.md](docs/ANDROID_ALPHA_REVIEW.md)**

---

## v0.9.x highlights (Windows + core)

| Area | What's in today |
|------|-----------------|
| **Detection** | 4 classes — plates, signs, mail labels, IDs (`irlsafety-detect.onnx`) |
| **Performance** | Screen Text OFF by default; frame skip 8 on fresh installs (4–6 GB VRAM) |
| **Censor** | Solid box default; angled quad cover for tilted packages |
| **OBS** | Hybrid delay, secure mode, control dock, dark tray panel |
| **Portable core** | `libirlsafety` — same pipeline on Windows JNI and Android NDK |
| **Android** | On-device YOLO + GLES overlay; no network permission |

---

## Next training session — `irlsafety_v08`

**Machines:** **JWCOM2** (GPU — label + train) · **JWCOM4** (OBS capture → RAM disk `Z:\irlsafety-training`)

You are prepping both machines for the next run. Nothing uploads to the cloud.

### Workflow summary

| Step | Where | Command / action |
|------|-------|------------------|
| Build props | JWCOM2 | `.\jwcom2-build-props.ps1` |
| Print props | JWCOM4 | Labels 4×6 + cardstock licenses from `Z:\irlsafety-training\props\` |
| Capture frames | JWCOM4 | Control dock → Capture Frame (mail angles, IDs in hand, edge crops) |
| Label | JWCOM2 | `.\jwcom2-label.ps1` |
| Ingest + gate | JWCOM2 | `.\jwcom2-prep.ps1 -Ingest` then `-RequireV07` |
| Train + export | JWCOM2 | `.\jwcom2-train.ps1 -RequireV07 -WarmStart -SkipBootstrap -Device 0 -Epochs 100` |
| Deploy | JWCOM4 | Copy `best.onnx` → OBS `models\` → Reload Model |

**Targets:** ≥50 `shipping_label` boxes · ≥30 `id_document` boxes · more angle/glare variety than v07.

**Optional phase 2:** OBB labels + `.\jwcom2-train.ps1 -OBB ...` for angled cover accuracy.

Detailed shot list: **`data/models/TRAINING_SESSION.txt`**  
JWCOM kit guide: **`data/models/LAPTOP_TRAINING.txt`** · kit path: `X:\irlsafety-training-kit`

One-liner train (JWCOM2):

```bat
cd X:\irlsafety-training-kit
.\jwcom2-prep.ps1 -RequireV07
.\jwcom2-train.ps1 -RequireV07 -WarmStart -SkipBootstrap -Device 0 -Epochs 100
```

---

## Build from source (Windows)

**Requirements:** Visual Studio 2022 (C++), CMake 3.28+, OBS 31.x

```bat
scripts\build-windows.bat
scripts\install-to-obs.bat
```

---

## Maintainer: publish releases

**Windows:**

```bat
scripts\package-v0.1.bat
scripts\zip-release.ps1
scripts\publish-release.ps1
```

**Android:** Actions → **Android APK Release** → Run workflow → `v0.9.5-dev` (or `v0.9.6` when bumped)

---

## Project layout

```
src/              libirlsafety core, OBS plugin, pipeline, ONNX, OCR
android/          Kotlin app + JNI (CameraX, GLES overlay)
cmake/            libirlsafety + platform backends (Windows, Android, …)
data/models/      bundled ONNX, training guides, TRAINING_SESSION.txt
data/training/    YOLO scripts, props, class lists
docs/             Android/iOS design, versioning, alpha review
scripts/          build, package, JWCOM2 training helpers
tests/            unit tests
```

---

## Design & docs

| Doc | Contents |
|-----|----------|
| [CREDITS.md](CREDITS.md) | Attribution, model history, roadmap |
| [docs/DESIGN-android-v1.md](docs/DESIGN-android-v1.md) | Android phases P10–P16 |
| [docs/DESIGN-ios-v1.md](docs/DESIGN-ios-v1.md) | iOS plan (post-Android alpha) |
| [docs/VERSIONING.md](docs/VERSIONING.md) | Pre-1.0 version policy |