# Third-Party Notices

IRLSAFETY+ is licensed under the **MIT License** (see [LICENSE](LICENSE)).  
When built and run inside OBS Studio, **GPLv2** obligations from OBS/libobs also apply.

## Direct dependencies (build & runtime)

| Component | License | URL | Role |
|-----------|---------|-----|------|
| OBS Studio / libobs | GPLv2 | https://github.com/obsproject/obs-studio | Host application & plugin API |
| OBS Plugin Template | GPLv2 | https://github.com/obsproject/obs-plugintemplate | Project scaffold & CMake helpers |
| ONNX Runtime *(planned)* | MIT | https://github.com/microsoft/onnxruntime | YOLO inference (GPU/CPU) |
| Ultralytics YOLOv8n *(planned)* | AGPL-3.0 (training code) / model-specific | https://github.com/ultralytics/ultralytics | Object detection model export |
| EasyOCR *(planned)* | Apache 2.0 | https://github.com/JaidedAI/EasyOCR | OCR reference implementation |
| Tesseract OCR *(alternative)* | Apache 2.0 | https://github.com/tesseract-ocr/tesseract | OCR engine option |

## Inspiration & related OBS projects

| Project | License | URL | Notes |
|---------|---------|-----|-------|
| obs-detect | GPLv3 | https://github.com/occ-ai/obs-detect | OBS detection filter patterns |
| obs-ocr | MIT | https://github.com/occ-ai/obs-ocr | OBS OCR integration patterns |

## IRLSAFETY+ source

Copyright (c) 2026 IRLSAFETY+ Contributors — **MIT License**