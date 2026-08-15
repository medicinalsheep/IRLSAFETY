# IRLSAFETY+ — iOS (phase I1 scaffold)

Standalone **camera privacy app** (not an OBS plugin). Shares `libirlsafety` with Windows and Android.

**Status:** I1 scaffold only — not a runnable App Store / TestFlight build yet.  
**Gate met:** Android P15 alpha APK path exists; I1 can proceed in parallel with P16.

## Phases (from `docs/DESIGN-ios-v1.md`)

| Phase | Deliverable | Status |
|-------|-------------|--------|
| **I1** | `ios/` + CMake static `libirlsafety` | **This folder** |
| **I2** | SwiftUI shell + AVFoundation preview | Next |
| **I3** | `CVPixelBuffer` → `irlsafety_frame_view` | |
| **I4** | ONNX Runtime iOS + bundled `irlsafety-detect.onnx` | |
| **I5** | Overlay compositor (Metal / CALayer) | |
| **I6** | Settings (4 categories, frame skip) | |
| **I7** | TestFlight + tester doc | |

## Interim path (today)

Until I7 ships, use **NDI HX Camera / Larix / SRT → OBS on Windows** with the existing IRLSAFETY+ filter. See `data/models/PLATFORMS.txt`.

## Build static core (macOS host)

```bash
cmake -S ios -B build-ios \
  -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=16.0
cmake --build build-ios --config Release
```

I1 compiles with **stub** YOLO/OCR so the C API links without ORT.  
I4 will flip `IRLSAFETY_ENABLE_ONNX=ON` and vendor ONNX Runtime iOS.

## Layout

```
ios/
  CMakeLists.txt          # I1 — static libirlsafety
  README.md               # this file
  IRLSafetyPlus/          # I2+ app shell placeholders
    Info.plist
    IRLSafetyPlusApp.swift
    ContentView.swift
    Bridging/IRLSafetyBridge.h
```

## Privacy

- Camera only when user grants permission
- No cloud inference; model stays on device (same promise as Android)
- `NSCameraUsageDescription` required before any App Store submission

## License

MIT — see [LICENSE](../LICENSE).
