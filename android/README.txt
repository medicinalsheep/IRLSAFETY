IRLSAFETY+ Android (P10 scaffold)
=================================

STATUS: P10 — Gradle + NDK links libirlsafety (detection ONNX stub until P12).

REQUIREMENTS
------------
- Android Studio Ladybug (2024.2+) or command-line SDK + NDK 26+
- JDK 17
- arm64-v8a device or emulator (API 26+)

OPEN IN ANDROID STUDIO
----------------------
1. File → Open → select this `android/` folder.
2. Let Gradle sync (downloads SDK components on first open).
3. Build → Make Project.
4. Run on a physical phone (recommended) or emulator.

COMMAND LINE (optional)
-----------------------
  cd android
  gradlew.bat assembleDebug

APK output:
  app/build/outputs/apk/debug/app-debug.apk

WHAT P10 PROVES
---------------
- `libirlsafety` compiles for Android arm64-v8a
- JNI bridge creates/destroys `irlsafety_pipeline`
- Logs route to logcat tag `IRLSAFETY+`

ROADMAP (see docs/DESIGN-android-v1.md)
---------------------------------------
  P11  CameraX → irlsafety_frame_view
  P12  ONNX Runtime Android + irlsafety-detect.onnx asset
  P13  GLES solid-box overlay on preview
  P14  Settings toggles (4 detection categories)

PRIVACY
-------
Same as Windows: all inference stays on-device. No account required.