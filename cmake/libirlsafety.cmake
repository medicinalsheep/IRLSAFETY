# libirlsafety — shared detection/OCR/censor core (v0.8 extraction).

set(
  IRLSAFETY_LIB_SOURCES
  src/pipeline.c
  src/region_tracker.c
  src/hybrid_delay.c
  src/irlsafety_settings.c
  src/irlsafety_log.c
  src/irlsafety_runtime.c
  src/irlsafety_paths.c
  src/custom_pii.c
  src/irlsafety_types.c
  src/irlsafety_geometry.c
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

set(_irlsafety_ocr_has_windows FALSE)
set(_irlsafety_ocr_has_child FALSE)
set(_irlsafety_lib_ocr_sources)

if(IRLSAFETY_OCR_BACKEND STREQUAL "stub")
  list(APPEND _irlsafety_lib_ocr_sources src/ocr/ocr_stub.c)
elseif(IRLSAFETY_OCR_BACKEND STREQUAL "child" AND IRLSAFETY_ENABLE_ONNX AND IRLSAFETY_HAS_ONNX_RUNTIME)
  list(APPEND _irlsafety_lib_ocr_sources src/ocr/ocr_child_onnx.cpp src/ocr/ocr_router.cpp)
  set(_irlsafety_ocr_has_child TRUE)
elseif(IRLSAFETY_OCR_BACKEND STREQUAL "auto" AND OS_WINDOWS AND IRLSAFETY_ENABLE_ONNX AND IRLSAFETY_HAS_ONNX_RUNTIME)
  list(APPEND _irlsafety_lib_ocr_sources src/ocr/ocr_windows.cpp src/ocr/ocr_child_onnx.cpp src/ocr/ocr_router.cpp)
  set(_irlsafety_ocr_has_windows TRUE)
  set(_irlsafety_ocr_has_child TRUE)
elseif(OS_WINDOWS AND NOT IRLSAFETY_OCR_BACKEND STREQUAL "stub")
  list(APPEND _irlsafety_lib_ocr_sources src/ocr/ocr_windows.cpp src/ocr/ocr_router.cpp)
  set(_irlsafety_ocr_has_windows TRUE)
elseif(IRLSAFETY_ENABLE_ONNX AND IRLSAFETY_HAS_ONNX_RUNTIME)
  list(APPEND _irlsafety_lib_ocr_sources src/ocr/ocr_child_onnx.cpp src/ocr/ocr_router.cpp)
  set(_irlsafety_ocr_has_child TRUE)
else()
  list(APPEND _irlsafety_lib_ocr_sources src/ocr/ocr_stub.c)
endif()
list(APPEND IRLSAFETY_LIB_SOURCES ${_irlsafety_lib_ocr_sources})

set(_irlsafety_lib_detection_sources)
if(IRLSAFETY_ENABLE_ONNX AND IRLSAFETY_HAS_ONNX_RUNTIME)
  list(APPEND _irlsafety_lib_detection_sources src/detection/yolo_onnx.cpp src/onnx/ort_ep.cpp)
else()
  list(APPEND _irlsafety_lib_detection_sources src/detection/yolo_onnx_stub.c)
endif()
list(APPEND IRLSAFETY_LIB_SOURCES ${_irlsafety_lib_detection_sources})

function(irlsafety_configure_lib_target target)
  target_include_directories(${target} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

  if(_irlsafety_ocr_has_child)
    set_source_files_properties(src/ocr/ocr_child_onnx.cpp PROPERTIES LANGUAGE CXX)
    target_compile_definitions(${target} PUBLIC IRLSAFETY_OCR_HAS_CHILD=1 IRLSAFETY_OCR_BACKEND_CHILD=1)
  endif()

  if(_irlsafety_ocr_has_windows)
    set_source_files_properties(src/ocr/ocr_windows.cpp PROPERTIES LANGUAGE CXX)
    target_compile_definitions(${target} PUBLIC IRLSAFETY_OCR_HAS_WINDOWS=1)
  endif()

  if(_irlsafety_ocr_has_child OR _irlsafety_ocr_has_windows)
    set_source_files_properties(src/ocr/ocr_router.cpp PROPERTIES LANGUAGE CXX)
    target_compile_features(${target} PUBLIC cxx_std_20)
  endif()

  if(_irlsafety_ocr_has_windows)
    target_link_libraries(${target} PRIVATE windowsapp)
  endif()

  if(IRLSAFETY_ENABLE_ONNX AND IRLSAFETY_HAS_ONNX_RUNTIME)
    set_source_files_properties(src/detection/yolo_onnx.cpp src/onnx/ort_ep.cpp PROPERTIES LANGUAGE CXX)
    target_compile_definitions(${target} PUBLIC IRLSAFETY_ENABLE_ONNX=1)
    target_compile_features(${target} PUBLIC cxx_std_20)
    target_link_libraries(${target} PRIVATE onnxruntime)
  endif()
endfunction()

add_library(libirlsafety STATIC ${IRLSAFETY_LIB_SOURCES})
irlsafety_configure_lib_target(libirlsafety)

# Test build: stub backends, no WinRT/ONNX compile in test executables.
set(IRLSAFETY_TEST_LIB_SOURCES ${IRLSAFETY_LIB_SOURCES})
list(FILTER IRLSAFETY_TEST_LIB_SOURCES EXCLUDE REGEX "yolo_onnx\\.cpp$|ort_ep\\.cpp$|ocr_windows\\.cpp$|ocr_child_onnx\\.cpp$|ocr_router\\.cpp$")
list(APPEND IRLSAFETY_TEST_LIB_SOURCES src/detection/yolo_onnx_stub.c src/ocr/ocr_stub.c)

add_library(libirlsafety_test STATIC ${IRLSAFETY_TEST_LIB_SOURCES})
target_compile_definitions(libirlsafety_test PUBLIC IRLSAFETY_TEST_BUILD)
irlsafety_configure_lib_target(libirlsafety_test)