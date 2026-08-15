# IRLSAFETY+ — Android v1 Design & Roadmap

**Status:** Draft (post v0.7.0)  
**Last updated:** 2026-07-03

---

## Summary

IRLSAFETY+ today is a **Windows OBS plugin**. Android is **not a port** of that DLL — it is a **new product** built on a shared **`libirlsafety` core** extracted in v0.8.

**Near-term phone use (no Android app):** stream the phone camera via **NDI / SRT / Larix** into OBS on a PC and run the existing filter.

**Android v1 MVP:** a standalone **camera protection app** — rear/front camera preview with local YOLO detection (plates, signs, mail, IDs) and solid-box censorship. Screen OCR and OBS integration are out of scope for v1.

---

## Goals & non-goals

### Goals (Android v1)

| Goal | Notes |
|------|-------|
| Local-only privacy | Same promise as Windows — no cloud inference |
| Reuse `irlsafety-detect.onnx` | v0.7 4-class model, ONNX Runtime Android |
| Solid-box censorship | Fastest path on mobile GPU/CPU |
| Simple toggles | Plates, signs, mail, IDs ON/OFF |
| Offline install | APK bundles model (~12 MB) |

### Non-goals (v1)

| Item | Defer to |
|------|----------|
| OBS plugin on Android | N/A — OBS has no Android plugin host |
| Full-screen OCR / Screen Text | v1.1+ (child OCR backend) |
| Custom PII keyword OCR | v1.1+ |
| Hybrid stream delay | v1.2+ |
| On-device training / labeling | v1.2+ (or companion desktop) |
| System-wide overlay on other apps | Separate product decision (Play policy) |
| iOS | After Android MVP proves core portability — see `docs/DESIGN-ios-v1.md` |

---

## Current architecture (Windows)

```
OBS plugin shell (pii-filter, gpu_frame, Qt dock)
        │
        ▼  irlsafety_frame_view
   pipeline.c  ──► YOLO (yolo_onnx.cpp) + OCR (ocr_windows.cpp)
        │
        ▼  irlsafety_region_list
   blur_compositor / gpu_frame overlays
```

**Platform lock-in today:**

- OCR default: `Windows.Media.Ocr` (WinRT)
- GPU capture: D3D11 (`gpu_frame.c`)
- Build: ONNX + real OCR gated on `OS_WINDOWS` in `CMakeLists.txt`
- Distribution: `irlsafety-plus.dll` into OBS

**Already portable (~22 C files):** `pipeline`, `region_tracker`, `blur_compositor`, `pii_match`, `yolo_preprocess`, `irlsafety_types`, etc. Test target `test_pipeline_stubs` compiles this subgraph without OBS.

---

## Target architecture

```
┌─────────────────────────────────────────────────────────────┐
│ libirlsafety (static/shared, C API, OBS-agnostic)           │
│  types · settings · pipeline · region_tracker · blur        │
│  yolo_onnx.h · ocr_backend.h (interfaces)                   │
└───────────────────────────┬─────────────────────────────────┘
                            │
     ┌──────────────────────┼──────────────────────┐
     ▼                      ▼                      ▼
 OBS adapter          Windows vcam v0.8      Android adapter v1
 pii-filter           DirectShow/MF          CameraX + GLES
 gpu_frame            standalone tray        Kotlin UI
 Qt dock
```

**Universal handoff type:** `irlsafety_frame_view` (BGRA/NV12 planes, width/height).

---

## Prerequisite: v0.8 `libirlsafety` extraction (Windows)

Android work **blocks** on this. Estimated **3–5 weeks** focused effort.

### Refactors

| Change | Files |
|--------|-------|
| Logging callback injection | `pipeline.c` — replace `obs_log` |
| Asset path callback | split `irlsafety_paths.c` → core + OBS adapter |
| Settings split | `filter_settings.c` → `irlsafety_settings.c` + `obs_filter_settings.c` |
| Hybrid delay split | overlay ring buffer in core; OBS stream-delay in adapter |
| CMake target | `add_library(irlsafety STATIC ...)` linked by plugin + tests |

### Acceptance

- OBS plugin behavior unchanged (regression tests pass)
- `test_pipeline_stubs` links only `libirlsafety`
- Second consumer (stub `standalone_main.c`) can call `irlsafety_pipeline_*` with a synthetic BGRA buffer

---

## Android v1 technical design

### Stack

| Layer | Choice | Rationale |
|-------|--------|-----------|
| Shell UI | Kotlin + Jetpack Compose | Fast settings UI, Play Store standard |
| Camera | CameraX → `ImageAnalysis` | Stable across OEMs |
| Native core | NDK + `libirlsafety` | Reuse C pipeline |
| Detection | ONNX Runtime Android 1.20+ | Same model as Windows |
| ORT EP | NNAPI (default), XNNPACK fallback | No DirectML on Android |
| OCR (v1) | **Disabled** | Avoid WinRT; child OCR not required for MVP |
| Compositor | OpenGL ES 2/3 | Draw solid boxes / simple quads over preview |
| Storage | App-private `filesDir/models/` | Copy bundled ONNX on first launch |

### Frame flow

```
CameraX ImageAnalysis (YUV_420_888)
    → JNI: convert to irlsafety_frame_view (NV12 or BGRA)
    → irlsafety_pipeline_detect_frame() every N frames
    → irlsafety_pipeline_get_overlays()
    → GLES: draw censorship rects on SurfaceView/TextureView
    → (optional) MediaRecorder / RTMP out — v1.1
```

### Performance targets (MVP)

| Metric | Target |
|--------|--------|
| Preview | 30 fps on mid-range 2022+ phone |
| Detection interval | Default every 6 frames @ 30 fps (~5 Hz) |
| Inference | &lt; 80 ms per YOLO pass on NNAPI-capable SoC |
| Thermal | Throttle to every 12–15 frames if skin temp high |

### APK contents

- `libirlsafety.so` (arm64-v8a; armeabi-v7a optional)
- `libonnxruntime.so` (ORT Android AAR)
- `assets/models/irlsafety-detect.onnx`
- No OCR models in v1

### Permissions

- `CAMERA` (required)
- No overlay / accessibility in v1
- Privacy policy: all processing on-device

---

## Phased delivery plan

### Phase 0 — Today (v0.7.x)

**Phone → PC path (documented, no code)**

1. Install Larix Broadcaster or NDI HX Camera on phone
2. Add NDI/SRT source in OBS on Windows
3. Apply IRLSAFETY+ filter on that source

See `data/models/PLATFORMS.txt`.

---

### Phase 1 — v0.8: Core extraction + Windows vcam (8–10 weeks)

| PR | Title | Depends |
|----|-------|---------|
| P1 | Add `libirlsafety` CMake target; move portable sources | — |
| P2 | Inject logging + asset path callbacks; split settings | P1 |
| P3 | Split `hybrid_delay` OBS hooks to adapter | P1 |
| P4 | OBS plugin links `libirlsafety`; CI green | P2, P3 |
| P5 | Enable `child` OCR + ONNX on non-Windows CMake paths (stubs → real) | P1 |
| P6 | Windows virtual camera module (per `VIRTUAL_CAMERA.txt`) | P4 |

**Exit:** Standalone C sample can run detection on a PNG frame sequence.

---

### Phase 2 — v0.9: Portable backends (4–6 weeks)

| PR | Title | Depends |
|----|-------|---------|
| P7 | ONNX Runtime abstraction — CPU/NNAPI/DML EP selection | P4 |
| P8 | Ship `child` OCR backend on Windows release builds (optional toggle) | P5, P7 |
| P9 | `libirlsafety` C API header (`irlsafety.h`) frozen for mobile | P4 |

**Exit:** Child OCR works on Windows; YOLO runs via ORT without OBS.

---

### Phase 3 — 0.9.x mobile alpha: Android camera MVP (10–14 weeks)

| PR | Title | Depends |
|----|-------|---------|
| P10 | `android/` Gradle project + NDK CMake linking `libirlsafety` | P9 |
| P11 | CameraX capture → `irlsafety_frame_view` JNI bridge | P10 |
| P12 | ORT Android + bundled `irlsafety-detect.onnx` | P10, P7 |
| P13 | GLES overlay compositor (solid box) | P11 |
| P14 | Settings UI (4 categories, confidence, frame skip) | P13 |
| P15 | Internal APK distribution + tester doc | P14 |

**Exit:** Tester can point phone at label/ID prop and see live censorship in preview.

---

### Phase 4 — v1.1+: Parity features (ongoing)

- Child OCR on Android (PP-OCR ONNX)
- Custom PII keywords
- Front-camera / streaming out (RTMP or system share)
- Angled cover (OBB model path)
- Play Store listing + policy review

---

## Risk register

| Risk | Impact | Mitigation |
|------|--------|------------|
| Core not extracted before Android work | Duplicate logic, 2× maintenance | **Gate:** no P10 until P4 merged |
| ORT NNAPI quality varies by OEM | Missed detections on some phones | XNNPACK fallback; confidence slider |
| Thermal throttling | Dropped frames | Adaptive frame skip; detection-only default |
| Play Store camera apps saturated | Discovery | Position as privacy/IRL tool; open-source core |
| Screen privacy on Android | User expectation mismatch | Clear v1 scope: **camera preview only** |
| Model size on cellular download | Install friction | Wi‑Fi download prompt; slim APK variant later |

---

## Effort summary

| Milestone | Calendar (part-time) | Shippable artifact |
|-----------|---------------------|-------------------|
| v0.8 core + vcam | ~2–3 months | Windows standalone path |
| v0.9 portable OCR/YOLO | ~1–1.5 months | Child OCR release |
| 0.9.x Android alpha | ~3–4 months | Internal APK |
| pre-1.0 Android beta | ~2 months | OCR + keywords |

**Total to credible Android tester build:** ~6–9 months after v0.8 starts (overlapping work possible on P10–P12 once P9 lands).

---

## Decision log

| Date | Decision |
|------|----------|
| 2026-07-03 | Android v1 = camera app, not OBS plugin |
| 2026-07-03 | OCR out of Android v1; detection-first |
| 2026-07-03 | Reuse v0.7 `irlsafety-detect.onnx`; no retrain required for MVP |
| 2026-07-03 | NDI/SRT → OBS remains official phone path until 0.9.x mobile alpha ships |
| 2026-07-03 | P2–P4 merged: `libirlsafety` OBS-agnostic; plugin owns OBS adapters |

---

## References

- `data/models/PLATFORMS.txt` — adapter matrix
- `data/models/SAMSUNG_ANDROID.txt` — Galaxy One UI / SoC tuning
- `docs/DESIGN-ios-v1.md` — iOS phased plan (post P15)
- `docs/ANDROID_ALPHA_REVIEW.md` — logic review checklist + P15–P16 plan
- `android/TESTER.md` — tester install guide (A53)
- `docs/VERSIONING.md` — naming and pre-1.0 version policy
- `data/models/VIRTUAL_CAMERA.txt` — v0.8 vcam plan
- `data/models/OCR.txt` — child OCR cross-platform path
- `tests/CMakeLists.txt` — portable core test subgraph
- `CMakeLists.txt` — backend selection (`OS_WINDOWS` gates)

---

## Next action (recommended)

1. ~~Finish **v0.7.1** Windows perf release~~ — shipped (screen text OFF, perf fixes)
2. ~~**P1** `libirlsafety` CMake target~~ — done (`cmake/libirlsafety.cmake`)
3. ~~**P2** logging + path callbacks; settings split~~ — done (`irlsafety_log`, `irlsafety_paths`, `irlsafety_settings`, `filter_settings` OBS adapter)
4. ~~**P3** hybrid delay OBS adapter~~ — done (`irlsafety_obs_adapter`, portable `hybrid_delay`)
5. ~~**P4** tests link `libirlsafety_test`; CI green~~ — done (`libirlsafety_test.lib`, stub backends)
6. ~~**P5** `child` OCR + ONNX on non-Windows CMake paths~~ — done (`cmake/onnxruntime_detect.cmake`)
7. ~~**P6** Windows virtual camera module~~ — done (OBS Virtual Cam hooks + tray controls)
8. ~~**Tray panel** dark-mode UI~~ — done (`tray_panel.cpp`, `data/ui/dark_theme.qss`)
9. ~~**P7** ONNX Runtime abstraction — CPU/NNAPI/DML EP selection~~ — done (`src/onnx/ort_ep.cpp`)
10. ~~**P8** child OCR dual-backend toggle on Windows~~ — done (`ocr_router.cpp`, `use_child_ocr` setting)
11. ~~**P9** frozen `irlsafety.h` C API~~ — done (`src/irlsafety.h`, API version 1)
12. ~~**P10** `android/` Gradle + NDK JNI bridge~~ — done (`android/`, `irlsafety_jni.cpp`, `cmake/android/libirlsafety_ndk.cmake`)
13. ~~**P11** CameraX → `irlsafety_frame_view` JNI~~ — done (`CameraSession.kt`, `yuv_convert.cpp`, `nativeProcessCameraFrame`)
14. ~~**P12** ONNX Runtime Android + bundled `irlsafety-detect.onnx`~~ — done (`onnxruntime-android` prefab, `ModelInstaller.kt`, Gradle asset copy)
15. ~~**P13** GLES solid-box overlay on preview~~ — done (`GlesCensorOverlay`, `nativeGetOverlayRects`, region tick)
16. ~~**P14** settings UI (4 detection categories, confidence, frame skip)~~ — done (`SettingsPanel`, `apply_runtime_settings`)
17. ~~**P15** internal tester APK + GitHub release workflow~~ — done (`android-apk.yaml`, `TESTER.md`, `package-android-apk.bat`)
18. **P16** (Android): next — hardening from A53 feedback