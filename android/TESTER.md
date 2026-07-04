# IRLSAFETY+ Android — Alpha Tester Guide

**Version:** v1.0.0-alpha5 (P14)  
**Target device:** Samsung Galaxy A53 · One UI 8.0 · Android 16

---

## Install

1. Enable **Install unknown apps** for your file manager (if sideloading APK).
2. Copy `app-debug.apk` or release APK to the phone.
3. Tap to install → open **IRLSAFETY+**.
4. Grant **Camera** when prompted.

Build from source: open `android/` in Android Studio → Run on device.

---

## First-run setup (Samsung)

1. **Settings → Apps → IRLSAFETY+ → Battery → Unrestricted**
2. Open app → scroll to **Detection settings**
3. If preview stutters: set **Frame skip** to **8–10**
4. Leave **Prefer GPU (NNAPI)** ON; if logcat shows CPU-only, that's OK on Exynos

---

## What to test

| Scenario | Expected |
|----------|----------|
| Point at license plate | Black box over plate |
| Point at street sign | Box over sign text area |
| Shipping label / mail | Box over label |
| ID / card prop | Box over document |
| Turn off **License plates** | Plates no longer boxed |
| **Protection enabled** OFF | No boxes |
| Confidence 85% | Fewer, higher-quality boxes |
| Frame skip 12 | Smoother preview, slower updates |

---

## Logcat

```bash
adb logcat -s IRLSAFETY+
```

Healthy startup:

```
Detection model path: /data/user/0/.../files/models/irlsafety-detect.onnx
frames=N · overlays=N · detector=ready · EP=NNAPI · skip=8
```

---

## Bug reports

Include:

- Device model (e.g. SM-A536B), One UI version, Android version
- APK version (alpha5)
- Steps to reproduce
- Screenshot or screen recording
- Logcat snippet (`IRLSAFETY+` tag)
- Settings used (frame skip, categories ON/OFF)

File issues: GitHub `medicinalsheep/IRLSAFETY` or direct to medicinalsheep.

---

## Privacy

- All inference on-device
- No account, no cloud, no analytics in alpha build
- Camera used only while app is in foreground

See `data/models/SAMSUNG_ANDROID.txt` and `docs/ANDROID_ALPHA_REVIEW.md`.