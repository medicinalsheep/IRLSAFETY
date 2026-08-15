IRLSAFETY+ detection model (YOLOv8 ONNX) — LOCAL INFERENCE ONLY
===============================================================

PRIVACY
-------
- Detection runs 100% on your device (ONNX Runtime, no cloud)
- Stream frames are never uploaded for inference or training by this plugin
- Train your own model locally — see TRAINING.txt (no Roboflow required)

RELEASE MODEL (bundled in official packages)
------------------------------------------
  irlsafety-detect.onnx   (~12 MB YOLOv8n)

  v0.6.x bundles: US license_plate + street_sign (2 classes)
  v0.7+ training:  4 classes — add shipping_label + id_document locally

Place your model here as:

  irlsafety-detect.onnx

Class indices (v0.7 training taxonomy):
  0 = license_plate
  1 = street_sign
  2 = shipping_label   (UPS, FedEx, Amazon, USPS, parcels)
  3 = id_document      (driver license, state ID)

LOCAL TRAINING
--------------
  scripts\train-model.ps1
  scripts\train-model.ps1 -Device 0 -OBB    (angled labels)

Guides:
  TRAINING.txt           — full pipeline
  TRAINING_SESSION.txt   — mail + ID capture checklist (v08)
  LAPTOP_TRAINING.txt    — single-machine GPU workflow

CHILD OCR MODELS (optional)
---------------------------
  scripts\fetch-ocr-models.ps1
  Build with -DIRLSAFETY_OCR_BACKEND=child to use instead of Windows OCR.

USER GUIDES (Control dock -> Resources)
---------------------------------------
  OCR.txt            — Screen Text, Custom PII, child OCR
  PII_STRATEGY.txt   — cards, tracking, ID numbers, training roadmap
  PLATFORMS.txt      — OBS, Discord, Zoom, GoPro, VLC compatibility
  VIRTUAL_CAMERA.txt — protected output for non-OBS apps (planned)

INSTALL TO OBS
--------------
  %ProgramFiles%\obs-studio\data\obs-plugins\irlsafety-plus\models\irlsafety-detect.onnx

Enable detection categories in filter settings, then Reload Model in Control dock.