IRLSAFETY+ Android
==================

VERSION: 0.9.5-dev (pre-1.0; aligns with Windows 0.9.x line)

FEATURES
--------
- CameraX live preview with on-device YOLO detection
- GLES solid-box censorship overlay
- Settings: four detection categories, confidence, frame skip (saved locally)

REQUIREMENTS
------------
- Android Studio Ladybug (2024.2+) or SDK + NDK 26+
- JDK 17, arm64-v8a device (API 26+)

DOWNLOAD (testers)
------------------
  https://github.com/medicinalsheep/IRLSAFETY/releases
  Asset: IRLSAFETY+-0.9.5-dev-android.apk
  Install: android\TESTER.md

BUILD (developers)
------------------
  scripts\package-android-apk.bat
  release\IRLSAFETY+-0.9.5-dev-android.apk

PUBLISH TO GITHUB
-----------------
  Actions → Android APK Release → Run workflow
  See docs/GITHUB.md

OPEN IN ANDROID STUDIO
----------------------
  File → Open → android\

TESTING
-------
  See android\TESTER.md and docs\VERSIONING.md

PRIVACY
-------
All inference on-device. No cloud. No account.