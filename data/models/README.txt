IRLSAFETY+ detection model (YOLOv8 ONNX) — LOCAL INFERENCE ONLY
===============================================================

PRIVACY
-------
- Detection runs 100% on your device (ONNX Runtime, no cloud)
- Stream frames are never uploaded for inference or training by this plugin
- Train your own model locally — see TRAINING.txt (no Roboflow required)

RELEASE MODEL (bundled in official packages)
------------------------------------------
  irlsafety-detect.onnx   (~12 MB YOLOv8n, US plates + signs, v0.6.1+ trained weights)

Place your model here as:

  irlsafety-detect.onnx

Class indices:
  0 = license_plate
  1 = street_sign
  2 = document
  3 = face

LOCAL TRAINING (recommended)
----------------------------
  scripts\setup-training.ps1
  data\models\TRAINING.txt

Label with LabelImg on your PC. Train with YOLOv8n. Export ONNX. Done.

CHILD OCR MODELS (optional — v0.7)
----------------------------------
  scripts\fetch-ocr-models.ps1   downloads irlsafety-ocr-det.onnx + irlsafety-ocr-rec.onnx
  Build with -DIRLSAFETY_OCR_BACKEND=child to use instead of Windows OCR.

USER GUIDES (Control dock -> Resources)
---------------------------------------
  OCR.txt            — Screen Text, Custom PII, child OCR
  PII_STRATEGY.txt   — cards, tracking, ID numbers, training roadmap
  PLATFORMS.txt      — OBS, Discord, Zoom, GoPro, VLC compatibility
  VIRTUAL_CAMERA.txt — protected output for non-OBS apps (planned)
  TRAINING.txt       — train license plates + street signs locally

INSTALL TO OBS
--------------
  %ProgramFiles%\obs-studio\data\obs-plugins\irlsafety-plus\models\irlsafety-detect.onnx

Enable License Plates / Street Signs in filter settings.