# Third-Party Notices

IRLSAFETY+ is licensed under the **MIT License** (see [LICENSE](LICENSE)).  
Copyright (c) 2026 **IRLSAFETY+ contributors**

When built and run inside OBS Studio, **GPLv2** obligations from OBS/libobs also apply to the combined distribution.

## Direct dependencies (build & runtime)

| Component | License | URL | Role |
|-----------|---------|-----|------|
| OBS Studio / libobs | GPLv2 | https://github.com/obsproject/obs-studio | Host application & plugin API |
| OBS Plugin Template | GPLv2 | https://github.com/obsproject/obs-plugintemplate | Project scaffold & CMake helpers |
| ONNX Runtime | MIT | https://github.com/microsoft/onnxruntime | YOLO ONNX inference (CPU/DirectML) |
| Ultralytics YOLOv8 | AGPL-3.0 (training toolchain) | https://github.com/ultralytics/ultralytics | Model training & ONNX export |
| Windows.Media.Ocr | Windows SDK / system | Microsoft | Default OCR backend (local) |
| stb_image | MIT / Public Domain | https://github.com/nothings/stb | Custom overlay image loading |
| Qt 6 | LGPL v3 / commercial | https://www.qt.io | Control dock UI (when enabled) |

## Bundled models

| Asset | Origin | Notes |
|-------|--------|-------|
| `irlsafety-detect.onnx` | Trained locally | YOLOv8n export; US plates + signs |
| `irlsafety-ocr-*.onnx` | Optional child OCR path | PP-OCR style det/rec for future backend |

Training with Ultralytics is subject to **AGPL-3.0** for the training code. Exported ONNX weights are used at runtime via ONNX Runtime (MIT). Consult Ultralytics licensing if you redistribute training scripts or derivative training services.

## Inspiration & related OBS projects

| Project | License | URL | Notes |
|---------|---------|-----|-------|
| obs-detect | GPLv3 | https://github.com/occ-ai/obs-detect | OBS detection filter patterns |
| obs-ocr | MIT | https://github.com/occ-ai/obs-ocr | OBS OCR integration patterns |

## Development tools (not shipped)

| Tool | Notes |
|------|-------|
| Grok Build (beta) | AI-assisted development contribution during v0.6.x; not a runtime dependency |

## IRLSAFETY+ source

Copyright (c) 2026 **IRLSAFETY+ contributors** — **MIT License**

See [CREDITS.md](CREDITS.md) for design overview, training summary, and attribution.