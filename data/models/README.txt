IRLSAFETY+ detection model (YOLOv8 ONNX) — LOCAL INFERENCE ONLY
===============================================================

PRIVACY
-------
- Detection runs 100% on your device (ONNX Runtime, no cloud)
- Stream frames are never uploaded for inference or training by this plugin
- Train your own model locally — see TRAINING.txt (no Roboflow required)

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

INSTALL TO OBS
--------------
  %ProgramFiles%\obs-studio\data\obs-plugins\irlsafety-plus\models\irlsafety-detect.onnx

Enable License Plates / Street Signs in filter settings.