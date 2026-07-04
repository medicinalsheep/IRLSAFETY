# ONNX Runtime for Android NDK via Gradle prefab (onnxruntime-android AAR).
# Requires android/app/build.gradle.kts: buildFeatures.prefab = true and the ORT dependency.

if(NOT ANDROID)
  message(FATAL_ERROR "onnxruntime_android.cmake requires an Android NDK toolchain")
endif()

if(NOT IRLSAFETY_ENABLE_ONNX)
  return()
endif()

find_package(onnxruntime REQUIRED CONFIG)

if(TARGET onnxruntime::onnxruntime)
  set(IRLSAFETY_HAS_ONNX_RUNTIME TRUE)
  set(IRLSAFETY_ONNX_LINK_TARGET onnxruntime::onnxruntime)
  message(STATUS "IRLSAFETY+: ONNX Runtime Android prefab linked (onnxruntime::onnxruntime)")
else()
  message(FATAL_ERROR "IRLSAFETY+: onnxruntime-android prefab package missing onnxruntime::onnxruntime target")
endif()