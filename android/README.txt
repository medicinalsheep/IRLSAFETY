IRLSAFETY+ Android (P13 GLES overlay)
====================================

STATUS: P13 — solid-box GLES censorship drawn over CameraX preview.
ONNX Runtime + irlsafety-detect.onnx load on first launch (P12).

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

WHAT P13 PROVES
---------------
- GLES2 transparent overlay (GlesCensorOverlay) draws black censorship boxes
- JNI returns tracked overlay rects; coordinates mapped to PreviewView FILL_CENTER
- Region tracker tick runs every analysis frame for hold/predict between detections
- Point camera at plate/sign/mail label — boxes should appear over preview

PRIMARY TESTER: Samsung Galaxy A53 (One UI 8.0, Android 16) — mid-range Exynos path

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
  P14  Settings toggles (4 detection categories)
  P15  Internal APK + tester doc

iOS app planning: docs/DESIGN-ios-v1.md (starts after Android P15).

PRIVACY
-------
Same as Windows: all inference stays on-device. No account required.