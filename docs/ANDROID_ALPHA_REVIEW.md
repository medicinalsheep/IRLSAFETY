# IRLSAFETY+ — Android Alpha Logic Review & Next Phases

**Status:** Living doc (post P16 code complete)  
**Last updated:** 2026-07-08  
**Primary tester device:** Samsung Galaxy A53 · One UI 8.0 · Android 16  
**App version:** `0.9.6-dev` (promote to `0.9.6` after checklist sign-off)

---

## Alpha exit criteria (for 0.9.6)

A tester on the A53 can:

1. Install debug APK without Android Studio
2. Grant camera permission, see live preview
3. Point at plate / sign / mail label / ID prop → **black boxes appear**
4. Toggle categories OFF → matching objects no longer censored
5. Raise frame skip → smoother preview under thermal load
6. Confirm `detector=ready` in logcat; no cloud traffic
7. Switch front/rear camera and 720p/1080p analysis without crash

---

## Logic review checklist

Run through on **Galaxy A53** before promoting `0.9.6-dev` → `0.9.6`.

### Pipeline & performance

| # | Check | Pass? | Notes |
|---|-------|-------|-------|
| L1 | Model loads once at startup (not per frame) | | P14 + P16: skip reload if path/EP unchanged |
| L2 | Settings hot-swap does not clear overlay tracker | | `irlsafety_pipeline_apply_runtime_settings` |
| L3 | `frame_skip` changes take effect without restart | | Default **8** (aligned with Windows) |
| L4 | Disabling all categories stops detection runs | | `enable_all` OFF → overlays clear |
| L5 | Confidence slider filters weak detections | | 85% vs 15% on same scene |
| L6 | Prefer GPU toggle reloads EP only when changed | | NNAPI ↔ CPU in logcat |

### Overlay alignment

| # | Check | Pass? | Notes |
|---|-------|-------|-------|
| O1 | Boxes align with objects at preview center | | FILL_CENTER mapper |
| O2 | Boxes stay stable between detection frames | | Region tracker + tick |
| O3 | Boxes track moving objects reasonably | | Hold/predict between skips |
| O4 | No boxes when protection OFF | | |

### Samsung A53 specifics

| # | Check | Pass? | Notes |
|---|-------|-------|-------|
| S1 | Battery → Unrestricted prevents preview freeze | | One UI 8.0 |
| S2 | Exynos EP shows in status (NNAPI or CPU) | | |
| S3 | frame_skip 8–10 smooth on 5+ min session | | Thermal |
| S4 | Android 16 permission flow works | | |
| S5 | 720p analysis cooler than 1080p | | P16 selector |

### Privacy

| # | Check | Pass? | Notes |
|---|-------|-------|-------|
| P1 | No network permission in manifest | | |
| P2 | Model stays in app-private `filesDir` | | |
| P3 | No crash on deny camera permission | | |

---

## P16 status (code)

| Task | Status |
|------|--------|
| Front / rear camera toggle | **Done** |
| Analysis resolution 720p / 1080p | **Done** |
| `yolo_onnx_load_model` skip if path + prefer_gpu unchanged | **Done** (Windows + Android) |
| Default frame_skip **8** (match libirlsafety) | **Done** |
| Overlay alignment fix if FILL_CENTER drifts | **Needs A53** |
| Adaptive frame skip under thermal | **Deferred** (manual skip 8–15 sufficient for alpha) |

---

## Known gaps (acceptable for alpha)

| Gap | Target |
|-----|--------|
| Angled (OBB) box drawing | v1.1 (model path exists on Windows) |
| Screen OCR | v1.1 (child OCR ONNX) |
| Settings sheet polish | Tester feedback |
| Overlay color picker | v1.1 |
| Secure mode / hybrid delay on mobile | Off by default; evaluate v1.2 |

---

## Phase plan

### P15 — Internal tester package (done)

| Task | Deliverable |
|------|-------------|
| `scripts/package-android-apk.bat` | Local APK build |
| `.github/workflows/android-apk.yaml` | GitHub prerelease with APK asset |
| `android/TESTER.md` | Download, Samsung dev mode, sideload, tests |
| `android/RELEASE_NOTES.txt` | Release description |

### P16 — Alpha hardening (code done · device sign-off open)

Fill checklist above on A53 → drop `-dev` → **0.9.6**.

### 0.10+ / pre-1.0 beta — Android feature parity slice

| Feature | Source |
|---------|--------|
| Child OCR (PP-OCR ONNX) | Windows `ocr_child_onnx.cpp` |
| Custom PII keywords | `custom_pii.c` |
| Censor color / blur mode | `blur_compositor.c` |
| Optional RTMP out | New adapter |

### iOS (I1 started)

See `docs/DESIGN-ios-v1.md` and `ios/README.md`.

| Device | Role |
|--------|------|
| iPhone 16e | Primary — CoreML EP |
| iPhone XS | Perf floor — frame_skip defaults higher |
| iPad | Tablet layout QA |

---

## Decision log

| Date | Decision |
|------|----------|
| 2026-07-03 | P14 adds `apply_runtime_settings` — no tracker clear on toggle |
| 2026-07-03 | Settings persist via SharedPreferences |
| 2026-07-03 | A53 is primary Android alpha device |
| 2026-07-03 | P15 before iOS I1 scaffold |
| 2026-07-08 | frame_skip default **8** on Android (match Windows/libirlsafety) |
| 2026-07-08 | P16: analysis resolution + model skip-reload; iOS I1 scaffold |
| 2026-07-08 | Version line `0.9.6-dev` until A53 checklist signed |

---

## Recommended order

```
P14–P16 code (done)
    → A53 checklist sign-off
    → promote 0.9.6-dev → 0.9.6 + Windows 0.9.6 with v08 model
    → iOS I2–I5
    → pre-1.0 beta OCR
    → iOS TestFlight (I7)
```
