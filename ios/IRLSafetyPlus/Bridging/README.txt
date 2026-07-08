I3: add IRLSafetyBridge.mm that converts CVPixelBuffer (NV12/BGRA) to
irlsafety_frame_view and calls irlsafety_pipeline_detect_frame / get_overlays.

Swift imports the bridging header; keep all C++/ORT in .mm files.
