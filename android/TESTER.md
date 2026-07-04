# IRLSAFETY+ Android — Tester Guide

**Version:** 0.9.5-dev (pre-1.0)  
**Primary device:** Samsung Galaxy A53 · One UI 8.0 · Android 16

---

## Install

1. Build: `scripts\package-android-apk.bat` (or Android Studio → Run)
2. Copy `release\IRLSAFETY+-0.9.5-dev-android.apk` to the phone
3. Install → open **IRLSAFETY+** → grant **Camera**

---

## Samsung setup

1. **Settings → Apps → IRLSAFETY+ → Battery → Unrestricted**
2. If preview stutters: increase **Frame skip** to 8–10 in app settings

---

## What to test

| Scenario | Expected |
|----------|----------|
| License plate in view | Black censorship box |
| Street sign | Box over sign |
| Shipping label | Box over label |
| ID / card | Box over document |
| Category OFF | That type no longer boxed |
| Protection OFF | No boxes |

---

## Logcat

```bash
adb logcat -s IRLSAFETY+
```

Look for `detector=ready` and your chosen `skip=` value.

---

## Bug reports

Include device model, One UI / Android version, app version (`0.9.5-dev`), steps, screenshot, and logcat (`IRLSAFETY+` tag).

GitHub issues: https://github.com/medicinalsheep/IRLSAFETY/issues

---

## Privacy

On-device inference only. No account. Camera used while app is in foreground.

See `data/models/SAMSUNG_ANDROID.txt` and `docs/VERSIONING.md`.