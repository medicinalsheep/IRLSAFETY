# libirlsafety — shared detection/OCR/censor core (v0.8 P1 extraction).
# Linked by the OBS plugin today; future: Android NDK, standalone desktop.

set(
  IRLSAFETY_LIB_SOURCES
  src/pipeline.c
  src/region_tracker.c
  src/hybrid_delay.c
  src/filter_settings.c
  src/custom_pii.c
  src/irlsafety_types.c
  src/irlsafety_geometry.c
  src/irlsafety_paths.c
  src/irlsafety_shutdown.c
  src/frame_sample.c
  src/virtual_cam/virtual_cam_stub.c
  src/blur/blur_compositor.c
  src/blur/overlay_image.c
  src/detection/yolo_preprocess.c
  src/ocr/ocr_engine.c
  src/ocr/pii_match.c
  src/ocr/pii_patterns.c
  src/ocr/ocr_frame_util.c)

set(_irlsafety_lib_ocr_sources)
if(IRLSAFETY_OCR_BACKEND STREQUAL "child" AND IRLSAFETY_ENABLE_ONNX AND OS_WINDOWS)
  list(APPEND _irlsafety_lib_ocr_sources src/ocr/ocr_child_onnx.cpp)
elseif(IRLSAFETY_OCR_BACKEND STREQUAL "stub")
  list(APPEND _irlsafety_lib_ocr_sources src/ocr/ocr_stub.c)
elseif(OS_WINDOWS)
  list(APPEND _irlsafety_lib_ocr_sources src/ocr/ocr_windows.cpp)
else()
  list(APPEND _irlsafety_lib_ocr_sources src/ocr/ocr_stub.c)
endif()
list(APPEND IRLSAFETY_LIB_SOURCES ${_irlsafety_lib_ocr_sources})

set(_irlsafety_lib_detection_sources)
if(IRLSAFETY_ENABLE_ONNX AND OS_WINDOWS)
  list(APPEND _irlsafety_lib_detection_sources src/detection/yolo_onnx.cpp)
else()
  list(APPEND _irlsafety_lib_detection_sources src/detection/yolo_onnx_stub.c)
endif()
list(APPEND IRLSAFETY_LIB_SOURCES ${_irlsafety_lib_detection_sources})

add_library(libirlsafety STATIC ${IRLSAFETY_LIB_SOURCES})

target_include_directories(libirlsafety PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_link_libraries(libirlsafety PUBLIC OBS::libobs)

if(ENABLE_FRONTEND_API)
  target_link_libraries(libirlsafety PUBLIC OBS::obs-frontend-api)
  target_compile_definitions(libirlsafety PUBLIC IRLSAFETY_HAS_FRONTEND_API=1)
endif()

if(OS_WINDOWS)
  if(IRLSAFETY_OCR_BACKEND STREQUAL "child" AND IRLSAFETY_ENABLE_ONNX)
    set_source_files_properties(src/ocr/ocr_child_onnx.cpp PROPERTIES LANGUAGE CXX)
  elseif(NOT IRLSAFETY_OCR_BACKEND STREQUAL "stub")
    set_source_files_properties(src/ocr/ocr_windows.cpp PROPERTIES LANGUAGE CXX)
  endif()
  target_compile_features(libirlsafety PUBLIC cxx_std_20)
  target_link_libraries(libirlsafety PRIVATE windowsapp)
endif()

if(IRLSAFETY_OCR_BACKEND STREQUAL "child" AND IRLSAFETY_ENABLE_ONNX AND OS_WINDOWS)
  target_compile_definitions(libirlsafety PUBLIC IRLSAFETY_OCR_BACKEND_CHILD=1)
endif()

if(IRLSAFETY_ENABLE_ONNX AND OS_WINDOWS)
  set_source_files_properties(src/detection/yolo_onnx.cpp PROPERTIES LANGUAGE CXX)
  target_compile_definitions(libirlsafety PUBLIC IRLSAFETY_ENABLE_ONNX=1)
  target_link_libraries(libirlsafety PRIVATE onnxruntime)
endif()