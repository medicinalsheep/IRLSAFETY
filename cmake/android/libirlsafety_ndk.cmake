# Build libirlsafety for Android NDK (P10+). Include from android/app/src/main/cpp/CMakeLists.txt.

if(NOT ANDROID)
  message(FATAL_ERROR "libirlsafety_ndk.cmake requires an Android NDK toolchain")
endif()

set(IRLSAFETY_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")

if(NOT DEFINED IRLSAFETY_ENABLE_ONNX)
  set(IRLSAFETY_ENABLE_ONNX OFF)
endif()

if(NOT DEFINED IRLSAFETY_OCR_BACKEND)
  set(IRLSAFETY_OCR_BACKEND "stub")
endif()

set(IRLSAFETY_BUILD_TESTS OFF)

if(IRLSAFETY_ENABLE_ONNX)
  include("${CMAKE_CURRENT_LIST_DIR}/onnxruntime_android.cmake")
endif()

include("${IRLSAFETY_REPO_ROOT}/cmake/libirlsafety.cmake")