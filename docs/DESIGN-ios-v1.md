# IRLSAFETY+ — iOS v1 Design & Roadmap

**Status:** Draft (planning)  
**Author:** medicinalsheep + Grok Build  
**Last updated:** 2026-07-03

---

## Summary

IRLSAFETY+ on iOS is a **standalone camera protection app** — the same product
category as Android v1, not an OBS plugin (OBS has no iOS plugin host).

**Near-term phone → PC path (no iOS app):** stream the iPhone camera via
**NDI HX Camera**, **Larix Broadcaster**, or **SRT** into OBS on Windows and
run the existing filter — identical to the Android interim workflow.

**iOS v1 MVP:** rear/front camera preview with local YOLO detection (plates,
signs, mail, IDs) and solid-box censorship. Screen OCR and system-wide overlay
are out of scope for v1.

**Gate:** iOS work starts after **Android 0.9.x alpha exit** (tester APK on A53
proves `libirlsafety` on mobile with ORT + GLES).

---

## Goals & non-goals

### Goals (iOS v1)

| Goal | Notes |
|------|-------|
| Local-only privacy | Same promise as Windows/Android — no cloud inference |
| Reuse `irlsafety-detect.onnx` | v0.7 4-class model via ONNX Runtime or Core ML conversion |
| Solid-box censorship | Metal or Core Animation overlay on camera preview |
| Simple toggles | Plates, signs, mail, IDs ON/OFF |
| Offline install | Model bundled in app (~12 MB) |

### Non-goals (v1)

| Item | Defer to |
|------|----------|
| OBS on iOS | N/A |
| Screen recording / ReplayKit redaction | v1.2+ (entitlements + App Review risk) |
| Full-screen OCR | v1.1+ (Vision + child OCR ONNX) |
| Custom PII keyword OCR | v1.1+ |
| System-wide overlay on other apps | Not feasible on iOS without MDM/special entitlements |
| App Store launch marketing | After Android beta validates core |

---

## Target architecture

```
┌─────────────────────────────────────────────────────────────┐
│ libirlsafety (static, C API — same as Android)              │
│  pipeline · region_tracker · blur · yolo_onnx interface     │
└───────────────────────────┬─────────────────────────────────┘
                            │
                            ▼
              iOS adapter (Swift + Objective-C++ bridge)
              AVFoundation capture → CVPixelBuffer → BGRA/NV12
              → irlsafety_pipeline_detect_frame()
              → overlay rects → Metal / CALayer on preview
              SwiftUI settings shell
```

**Universal handoff type:** `irlsafety_frame_view` (same as Windows/Android).

---

## iOS technical design

### Stack

| Layer | Choice | Rationale |
|-------|--------|-----------|
| Shell UI | SwiftUI | Standard for new iOS apps; fast settings screens |
| Camera | AVFoundation `AVCaptureSession` + video output | Full control; no third-party camera wrapper required |
| Pixel format | `kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange` or BGRA | Convert to `irlsafety_frame_view` in Obj-C++ |
| Native core | Xcode target linking `libirlsafety` | Same C sources as Android NDK build |
| Detection | ONNX Runtime iOS **or** Core ML converted model | ORT first for parity; Core ML optional perf path |
| ORT EP | CPU + CoreML EP (Apple Neural Engine) | No NNAPI/DirectML on iOS |
| OCR (v1) | **Disabled** | Vision framework + child OCR in v1.1 |
| Compositor | Metal shader or `CAShapeLayer` rects | Solid boxes over `AVCaptureVideoPreviewLayer` |
| Storage | App bundle + `Application Support/models/` | Copy ONNX on first launch if not using Core ML |

### Frame flow

```
AVCaptureVideoDataOutput (delegate queue)
    → CVPixelBuffer (NV12)
    → Obj-C++: convert to irlsafety_frame_view
    → irlsafety_pipeline_detect_frame() every N frames
    → irlsafety_pipeline_get_overlays()
    → Main thread: update overlay layers on preview
```

### Performance targets (MVP)

| Metric | Target |
|--------|--------|
| Preview | 30 fps on iPhone 12+ |
| Detection interval | Default every 6 frames (~5 Hz) |
| Inference | < 60 ms on A15+ with CoreML EP |
| Thermal | Adaptive frame skip 12–15 under pressure |

### App Store considerations

| Topic | Plan |
|-------|------|
| Camera usage description | Required `NSCameraUsageDescription` — explain on-device PII protection |
| Privacy nutrition labels | No data collected; processing on-device |
| Encryption export | ONNX model only — standard exemption |
| Background camera | Not required for v1 (foreground preview only) |
| ReplayKit extension | Defer — separate entitlement review |

---

## Phased delivery plan

### Phase 0 — Today

**iPhone → PC path (documented, no iOS code)**

1. Larix or NDI HX Camera on iPhone
2. OBS NDI/SRT source on Windows
3. IRLSAFETY+ filter on that source

See `data/models/PLATFORMS.txt`.

---

### Phase 1 — iOS scaffold (after Android P15)

| PR | Title | Depends | Status |
|----|-------|---------|--------|
| I1 | `ios/` + `libirlsafety` static lib via CMake + SwiftUI shell placeholders | Android P15 | **Started** (`ios/`) |
| I2 | SwiftUI shell + AVFoundation preview | I1 | Pending |
| I3 | Pixel buffer → `irlsafety_frame_view` bridge | I2 | Pending |
| I4 | ONNX Runtime iOS + bundled `irlsafety-detect.onnx` | I1, P7 | Pending |
| I5 | Overlay compositor (Metal or CALayer boxes) | I3 | Pending |
| I6 | Settings UI (4 categories, frame skip) | I5 | Pending |
| I7 | TestFlight internal build + tester doc | I6 | Pending |

**Exit:** Tester points iPhone at license plate prop and sees live censorship.

---

### Phase 2 — iOS v1.1+ (parity)

- Vision / child OCR for screen text (where App Review allows)
- Custom PII keywords
- Optional Core ML model package for ANE-only path
- Front camera default for selfie streamers
- Short-form export (sanitized clip save to Photos)

---

## Platform comparison

| Aspect | Android v1 | iOS v1 |
|--------|------------|--------|
| Camera API | CameraX | AVFoundation |
| JNI / bridge | JNI (Kotlin) | Obj-C++ (Swift) |
| GPU inference | NNAPI | CoreML EP |
| Overlay | OpenGL ES | Metal / CALayer |
| Distribution | APK sideload | TestFlight → App Store |
| Min OS | API 26 (Android 8) | iOS 16+ (tentative) |

---

## Risk register

| Risk | Impact | Mitigation |
|------|--------|------------|
| Starting iOS before Android proves core | Duplicate debug effort | **Gate:** I1 after P15 |
| Core ML conversion drift from ONNX | Wrong boxes / missed classes | Keep ONNX as source of truth; validate mAP on device |
| App Review camera justification | Rejection | Clear privacy-first positioning; no covert recording features |
| Thermal on older iPhones (Xr/11) | Janky preview | Higher default frame skip; detection-only mode |
| Swift/C++ interop complexity | Slow I3 | Mirror Android JNI patterns in thin Obj-C++ shim |

---

## Effort summary

| Milestone | Calendar (part-time) | Shippable artifact |
|-----------|---------------------|-------------------|
| iOS scaffold I1–I3 | ~4–6 weeks | Preview + stub pipeline |
| iOS alpha I4–I7 | ~6–8 weeks | TestFlight build |
| iOS v1.1 OCR | ~2–3 months | Vision + child OCR |

**Total to credible iOS tester build:** ~3–4 months after Android P15 exit
(can overlap I1 CMake work with Android P14–P15 polish).

---

## Tester device matrix (planned)

| Device | Role | Notes |
|--------|------|-------|
| Galaxy A53 (Android 16 / One UI 8.0) | **Android alpha primary** | Mid-range Exynos 1280; P12–P15 validation |
| iPhone 16e | iOS alpha primary | Newest phone; CoreML EP + ANE path |
| iPhone XS | iOS perf floor | A12 — higher default frame_skip; thermal watch |
| iPad | iOS layout / tablet | Landscape preview + settings; not phone-only assumptions |

Android ships first (P15 internal APK). iOS TestFlight (I7) starts after Android MVP exit.

---

## Decision log

| Date | Decision |
|------|----------|
| 2026-07-03 | iOS v1 = camera app, not OBS plugin |
| 2026-07-03 | OCR out of iOS v1; detection-first (matches Android) |
| 2026-07-03 | Reuse `irlsafety-detect.onnx`; Core ML conversion optional |
| 2026-07-03 | iOS starts after Android MVP exit (P15) |
| 2026-07-03 | NDI/SRT → OBS remains official iPhone path until iOS alpha |
| 2026-07-08 | I1 scaffold landed (`ios/CMakeLists.txt`, SwiftUI placeholders) |

---

## References

- `docs/DESIGN-android-v1.md` — Android MVP (template for iOS phases)
- `data/models/PLATFORMS.txt` — adapter matrix
- `data/models/SAMSUNG_ANDROID.txt` — Samsung Android tuning (closest OEM doc for iOS analog)
- `src/irlsafety.h` — frozen C API (P9)
- `cmake/libirlsafety.cmake` — portable core sources

---

## Next action (recommended)

1. Finish **Android P11** — CameraX → JNI (done)
2. **P12** — ONNX Runtime Android + model asset
3. **P13–P15** — GLES overlay, settings, internal APK
4. **I1** — begin `ios/` Xcode project once P15 validates mobile core