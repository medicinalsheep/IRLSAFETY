# libirlsafety — iOS static library helper (I1 scaffold).
# Include from ios/CMakeLists.txt after setting IRLSAFETY_REPO_ROOT.

if(NOT DEFINED IRLSAFETY_REPO_ROOT)
  message(FATAL_ERROR "IRLSAFETY_REPO_ROOT must be set before including libirlsafety_ios.cmake")
endif()

# iOS builds use the child/stub OCR path (no Windows.Media.Ocr).
if(NOT DEFINED IRLSAFETY_OCR_BACKEND)
  set(IRLSAFETY_OCR_BACKEND "stub" CACHE STRING "OCR backend for iOS (stub until v1.1)")
endif()

if(NOT DEFINED IRLSAFETY_ENABLE_ONNX)
  set(IRLSAFETY_ENABLE_ONNX ON CACHE BOOL "Enable YOLO ONNX detection")
endif()

# ONNX Runtime iOS is wired in I4; until then allow stub detection for compile checks.
if(NOT DEFINED IRLSAFETY_HAS_ONNX_RUNTIME)
  set(IRLSAFETY_HAS_ONNX_RUNTIME FALSE)
endif()

include("${IRLSAFETY_REPO_ROOT}/cmake/libirlsafety.cmake")
