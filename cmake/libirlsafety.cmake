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

function(irlsafety_configure_lib_target target)
  target_include_directories(${target} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

  if(OS_WINDOWS)
    if(IRLSAFETY_OCR_BACKEND STREQUAL "child" AND IRLSAFETY_ENABLE_ONNX)
      set_source_files_properties(src/ocr/ocr_child_onnx.cpp PROPERTIES LANGUAGE CXX)
    elseif(NOT IRLSAFETY_OCR_BACKEND STREQUAL "stub")
      set_source_files_properties(src/ocr/ocr_windows.cpp PROPERTIES LANGUAGE CXX)
    endif()
    target_compile_features(${target} PUBLIC cxx_std_20)
    target_link_libraries(${target} PRIVATE windowsapp)
  endif()

  if(IRLSAFETY_OCR_BACKEND STREQUAL "child" AND IRLSAFETY_ENABLE_ONNX AND OS_WINDOWS)
    target_compile_definitions(${target} PUBLIC IRLSAFETY_OCR_BACKEND_CHILD=1)
  endif()

  if(IRLSAFETY_ENABLE_ONNX AND OS_WINDOWS)
    set_source_files_properties(src/detection/yolo_onnx.cpp PROPERTIES LANGUAGE CXX)
    target_compile_definitions(${target} PUBLIC IRLSAFETY_ENABLE_ONNX=1)
    target_link_libraries(${target} PRIVATE onnxruntime)
  endif()
endfunction()

add_library(libirlsafety STATIC ${IRLSAFETY_LIB_SOURCES})
irlsafety_configure_lib_target(libirlsafety)

# Test build: stub backends, no WinRT/ONNX compile in test executables.
set(IRLSAFETY_TEST_LIB_SOURCES ${IRLSAFETY_LIB_SOURCES})
list(FILTER IRLSAFETY_TEST_LIB_SOURCES EXCLUDE REGEX "yolo_onnx\\.cpp$|ocr_windows\\.cpp$|ocr_child_onnx\\.cpp$")
list(APPEND IRLSAFETY_TEST_LIB_SOURCES src/detection/yolo_onnx_stub.c src/ocr/ocr_stub.c)

add_library(libirlsafety_test STATIC ${IRLSAFETY_TEST_LIB_SOURCES})
target_compile_definitions(libirlsafety_test PUBLIC IRLSAFETY_TEST_BUILD)
irlsafety_configure_lib_target(libirlsafety_test)