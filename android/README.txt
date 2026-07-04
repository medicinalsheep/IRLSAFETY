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

BUILD
-----
  scripts\package-android-apk.bat

Output:
  release\IRLSAFETY+-0.9.5-dev-android.apk

OPEN IN ANDROID STUDIO
----------------------
  File → Open → android\

TESTING
-------
  See android\TESTER.md and docs\VERSIONING.md

PRIVACY
-------
All inference on-device. No cloud. No account.