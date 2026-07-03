# Detect/link ONNX Runtime (Windows bundle or system install on other OS).

set(IRLSAFETY_HAS_ONNX_RUNTIME FALSE)

if(NOT IRLSAFETY_ENABLE_ONNX)
  return()
endif()

if(OS_WINDOWS)
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/onnxruntime.cmake")
  set(IRLSAFETY_HAS_ONNX_RUNTIME TRUE)
  return()
endif()

find_path(
  IRLSAFETY_ONNXRUNTIME_INCLUDE_DIR
  NAMES onnxruntime_c_api.h
  PATHS ENV ONNXRUNTIME_ROOT ENV ONNXRUNTIME_HOME /usr/local /usr
  PATH_SUFFIXES include)

find_library(
  IRLSAFETY_ONNXRUNTIME_LIBRARY
  NAMES onnxruntime
  PATHS ENV ONNXRUNTIME_ROOT ENV ONNXRUNTIME_HOME /usr/local/lib /usr/lib)

if(IRLSAFETY_ONNXRUNTIME_INCLUDE_DIR AND IRLSAFETY_ONNXRUNTIME_LIBRARY)
  if(NOT TARGET onnxruntime)
    add_library(onnxruntime SHARED IMPORTED GLOBAL)
    set_target_properties(
      onnxruntime
      PROPERTIES IMPORTED_LOCATION "${IRLSAFETY_ONNXRUNTIME_LIBRARY}"
                 INTERFACE_INCLUDE_DIRECTORIES "${IRLSAFETY_ONNXRUNTIME_INCLUDE_DIR}")
  endif()
  set(IRLSAFETY_HAS_ONNX_RUNTIME TRUE)
  message(STATUS "IRLSAFETY+: ONNX Runtime found at ${IRLSAFETY_ONNXRUNTIME_LIBRARY}")
else()
  message(STATUS "IRLSAFETY+: ONNX Runtime not found — using detection/OCR ONNX stubs on this platform")
endif()