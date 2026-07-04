IRLSAFETY+ Android (P14 settings)
=================================

STATUS: P14 — detection category toggles, confidence + frame skip (persisted).
P13 GLES overlay · P12 ONNX · P11 CameraX.

REQUIREMENTS
------------
- Android Studio Ladybug (2024.2+) or command-line SDK + NDK 26+
- JDK 17
- arm64-v8a device or emulator (API 26+)
- Physical phone recommended (Samsung Galaxy S21+ ideal first tester)

OPEN IN ANDROID STUDIO
----------------------
1. File → Open → select this `android/` folder.
2. Let Gradle sync (downloads SDK components on first open).
3. Build → Make Project.
4. Run on a physical phone (recommended) or emulator.
5. Grant Camera permission when prompted — rear preview should appear.

COMMAND LINE (optional)
-----------------------
  cd android
  gradlew.bat assembleDebug

APK output:
  app/build/outputs/apk/debug/app-debug.apk

WHAT P14 ADDS
-------------
- SettingsPanel: plates / signs / mail / IDs toggles
- Confidence slider (15–85%) and frame skip (1–15)
- Prefer GPU (NNAPI) toggle; settings saved in SharedPreferences
- Hot-swap via irlsafety_pipeline_apply_runtime_settings (no per-frame model reload)

PRIMARY TESTER: Samsung Galaxy A53 (One UI 8.0, Android 16)
See android/TESTER.md and docs/ANDROID_ALPHA_REVIEW.md

BUILD NOTE
----------
The Gradle preBuild task copies:
  data/models/irlsafety-detect.onnx  →  app/src/main/assets/models/
Train or download the model on Windows first if missing.

SAMSUNG GALAXY NOTES
--------------------
See data/models/SAMSUNG_ANDROID.txt in the repo root:
  - Set Battery → Unrestricted for IRLSAFETY+
  - Exynos vs Snapdragon affects NNAPI EP selection
  - Default frame_skip=6 for thermal headroom
  - Point camera at plate/sign/mail label to verify overlay count > 0

ROADMAP (see docs/DESIGN-android-v1.md)
---------------------------------------
  P15  Internal APK + GitHub release (TESTER.md ready)
  P16  Alpha hardening from A53 feedback

iOS app planning: docs/DESIGN-ios-v1.md (starts after Android P15).

PRIVACY
-------
Same as Windows: all inference stays on-device. No account required.