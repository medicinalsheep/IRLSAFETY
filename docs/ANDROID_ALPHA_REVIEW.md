# IRLSAFETY+ — Android Alpha Logic Review & Next Phases

**Status:** Living doc (post P14)  
**Last updated:** 2026-07-03  
**Primary tester device:** Samsung Galaxy A53 · One UI 8.0 · Android 16

---

## Alpha exit criteria (P15)

A tester on the A53 can:

1. Install debug APK without Android Studio
2. Grant camera permission, see live preview
3. Point at plate / sign / mail label / ID prop → **black boxes appear**
4. Toggle categories OFF → matching objects no longer censored
5. Raise frame skip → smoother preview under thermal load
6. Confirm `detector=ready` in logcat; no cloud traffic

---

## Logic review checklist

Run through on **Galaxy A53** before sharing `0.9.5-dev` APK.

### Pipeline & performance

| # | Check | Pass? | Notes |
|---|-------|-------|-------|
| L1 | Model loads once at startup (not per frame) | | Fixed P14: removed per-frame `update_settings` |
| L2 | Settings hot-swap does not clear overlay tracker | | `irlsafety_pipeline_apply_runtime_settings` |
| L3 | `frame_skip` changes take effect without restart | | Toggle 6 → 10, watch status line |
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

### Privacy

| # | Check | Pass? | Notes |
|---|-------|-------|-------|
| P1 | No network permission in manifest | | |
| P2 | Model stays in app-private `filesDir` | | |
| P3 | No crash on deny camera permission | | |

---

## Known gaps (acceptable for alpha)

| Gap | Target |
|-----|--------|
| Front camera switch | P15 or v1.1 |
| Angled (OBB) box drawing | v1.1 (model path exists on Windows) |
| Screen OCR | v1.1 (child OCR ONNX) |
| Settings sheet / bottom nav polish | P15 tester feedback |
| Overlay color picker | v1.1 |
| Secure mode / hybrid delay on mobile | Off by default; evaluate v1.2 |

---

## Phase plan (after P14)

### P15 — Internal tester package (next, ~1 week)

| Task | Deliverable |
|------|-------------|
| `scripts/build-android-release.bat` | Signed or debug APK + version stamp |
| `android/TESTER.md` | Install, A53 tuning, logcat, bug report template |
| `scripts/package-android-apk.bat` → `release/IRLSAFETY+-0.9.5-dev-android.apk` | Sideload package |
| Smoke test matrix | A53 sign-off on checklist above |

**Exit:** medicinalsheep ships APK to self + 1–2 trusted testers.

---

### P16 — Alpha hardening (optional, ~2 weeks)

Based on A53 feedback:

- Front / rear camera toggle
- Analysis resolution selector (720p vs 1080p)
- `yolo_onnx_load_model` skip if path unchanged (Windows + Android)
- Overlay alignment fix if FILL_CENTER mapping drifts on A53
- Adaptive frame skip under thermal (read `PowerManager` / frame time)

---

### 0.10+ / pre-1.0 beta — Android feature parity slice (~1–2 months)

| Feature | Source |
|---------|--------|
| Child OCR (PP-OCR ONNX) | Windows `ocr_child_onnx.cpp` |
| Custom PII keywords | `custom_pii.c` |
| Censor color / blur mode | `blur_compositor.c` |
| Optional RTMP out | New adapter |

---

### iOS phase (after Android P15 exit)

See `docs/DESIGN-ios-v1.md`. Tester hardware:

| Device | Role |
|--------|------|
| iPhone 16e | Primary — CoreML EP |
| iPhone XS | Perf floor — frame_skip defaults higher |
| iPad | Tablet layout QA |

**I1** Xcode + `libirlsafety` static lib can start in parallel with P16 if Android alpha is stable.

---

### Windows (ongoing)

| Item | Status |
|------|--------|
| v0.9.4 OBS plugin | Shipped |
| Virtual camera | v0.8 done |
| Frame-drop tuning doc | INSTALL.txt |

---

## Decision log

| Date | Decision |
|------|----------|
| 2026-07-03 | P14 adds `apply_runtime_settings` — no tracker clear on toggle |
| 2026-07-03 | Settings persist via SharedPreferences |
| 2026-07-03 | A53 is primary Android alpha device |
| 2026-07-03 | P15 before iOS I1 scaffold |

---

## Recommended order

```
P14 settings (done)
    → P15 tester APK + A53 sign-off
    → P16 hardening from feedback (parallel: iOS I1 scaffold)
    → pre-1.0 beta OCR
    → iOS TestFlight (I7)
```