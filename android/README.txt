IRLSAFETY+ Android (P11 CameraX bridge)
========================================

STATUS: P11 — CameraX preview feeds libirlsafety via JNI.
Detection ONNX is stub until P12 (expect detector=stub in status line).

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

WHAT P11 PROVES
---------------
- CameraX ImageAnalysis (YUV_420_888) → JNI → irlsafety_frame_view
- Live preview in Compose via PreviewView
- Per-frame overlay count (0 until model loads in P12)
- Logs route to logcat tag IRLSAFETY+

SAMSUNG GALAXY NOTES
--------------------
See data/models/SAMSUNG_ANDROID.txt in the repo root:
  - Set Battery → Unrestricted for IRLSAFETY+
  - Exynos vs Snapdragon affects NNAPI path (P12)
  - Default frame_skip=6 for thermal headroom

ROADMAP (see docs/DESIGN-android-v1.md)
---------------------------------------
  P12  ONNX Runtime Android + irlsafety-detect.onnx asset
  P13  GLES solid-box overlay on preview
  P14  Settings toggles (4 detection categories)
  P15  Internal APK + tester doc

iOS app planning: docs/DESIGN-ios-v1.md (starts after Android P15).

PRIVACY
-------
Same as Windows: all inference stays on-device. No account required.