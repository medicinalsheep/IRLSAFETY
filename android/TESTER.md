# IRLSAFETY+ Android — Install & Test Guide

**Version:** 0.9.5-dev (pre-1.0)  
**Test phone:** Samsung Galaxy A53 · One UI 8.0 · Android 16

---

## 1. Download from GitHub

1. Open **Releases** on the repo:  
   https://github.com/medicinalsheep/IRLSAFETY/releases
2. Find **`IRLSAFETY+ 0.9.5-dev (Android)`** prerelease (tag `v0.9.5-dev`).
3. Under **Assets**, download **`IRLSAFETY+-0.9.5-dev-android.apk`**.

### APK not on Releases yet?

Someone must run the build once:

1. **Actions** → **Android APK Release** → **Run workflow** → **Run**
2. Wait for green ✓ (~10–15 min)
3. Refresh **Releases** or use **Artifacts** on that run

See **docs/GITHUB.md** for details. Node.js 20 warnings in logs are harmless.

---

## 2. Allow install on Samsung (no PC required)

You do **not** need a computer if you download the APK on the phone.

### Option A — Install from browser (recommended)

1. Open the Releases page in **Chrome** or **Samsung Internet** on the A53.
2. Download the `.apk` file.
3. **Settings → Security and privacy → More security settings → Install unknown apps**
4. Select your browser → turn **Allow from this source** ON.
5. Open **My Files** → **Downloads** → tap `IRLSAFETY+-0.9.5-dev-android.apk` → **Install**.

One UI 8.0 path may also appear as:  
**Settings → Apps → ⋮ menu → Special access → Install unknown apps**

### Option B — USB install (needs Developer mode)

Use this if browser install fails or you prefer `adb`.

#### Enable Developer options

1. **Settings → About phone → Software information**
2. Tap **Build number** seven times → enter PIN → “Developer mode enabled”.

#### Enable USB debugging

1. **Settings → Developer options**
2. Turn **USB debugging** ON.
3. Connect the phone to your PC with a USB cable.
4. On the phone, tap **Allow** when prompted for USB debugging.

#### Install from PC

```bash
adb install -r IRLSAFETY+-0.9.5-dev-android.apk
```

Windows with repo checkout:

```bat
scripts\package-android-apk.bat
scripts\install-android-usb.bat
```

Or install [Platform Tools](https://developer.android.com/tools/releases/platform-tools) and run `adb install -r` on the APK file.

---

## 3. First launch (Samsung A53)

1. Open **IRLSAFETY+** → tap **Allow** for **Camera**.
2. **Settings → Apps → IRLSAFETY+ → Battery → Unrestricted** (important on One UI).
3. Point the rear camera at a license plate, sign, or shipping label.
4. You should see **black boxes** over detected items.

### If preview is slow or stutters

In the app, scroll to settings:

- Set **Frame skip** to **8–10**
- Leave **Prefer GPU (NNAPI)** on (Exynos may show CPU in logs — that is OK)

---

## 4. What to test

| Action | Expected result |
|--------|-----------------|
| Plate / sign / label / ID in frame | Black censorship box |
| Turn off **License plates** | Plates no longer boxed |
| **Protection enabled** OFF | No boxes |
| Confidence **85%** | Fewer detections |
| Frame skip **10** | Smoother preview, slower updates |

Full checklist: `docs/ANDROID_ALPHA_REVIEW.md`

---

## 5. Logs (optional, USB debugging)

```bash
adb logcat -s IRLSAFETY+
```

Healthy line looks like: `detector=ready · EP=... · skip=8`

---

## 6. Bug reports

Include:

- Phone model (e.g. SM-A536B), One UI + Android version
- App version: **0.9.5-dev**
- Steps, screenshot or short video
- Logcat snippet if possible

https://github.com/medicinalsheep/IRLSAFETY/issues

---

## 7. Privacy

- Inference runs only on your phone
- No account or cloud required
- Camera is used while the app is open in the foreground

See also: `data/models/SAMSUNG_ANDROID.txt`, `docs/VERSIONING.md`