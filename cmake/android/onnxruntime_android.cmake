# ONNX Runtime for Android NDK.
# Prefers extracted AAR prefab (CI/local) at IRLSAFETY_ONNX_ANDROID_ROOT; falls back to Gradle prefab.

if(NOT ANDROID)
  message(FATAL_ERROR "onnxruntime_android.cmake requires an Android NDK toolchain")
endif()

if(NOT IRLSAFETY_ENABLE_ONNX)
  return()
endif()

function(irlsafety_android_link_ort_imported ort_module_dir)
  set(_ort_lib "${ort_module_dir}/libs/android.${ANDROID_ABI}/libonnxruntime.so")
  set(_ort_include "${ort_module_dir}/include")

  if(NOT EXISTS "${_ort_lib}")
    message(FATAL_ERROR "IRLSAFETY+: ONNX Runtime library not found at ${_ort_lib}")
  endif()

  if(NOT TARGET onnxruntime)
    add_library(onnxruntime SHARED IMPORTED GLOBAL)
  endif()

  set_target_properties(
    onnxruntime
    PROPERTIES IMPORTED_LOCATION "${_ort_lib}"
               INTERFACE_INCLUDE_DIRECTORIES "${_ort_include}")
  set(IRLSAFETY_ONNX_LINK_TARGET onnxruntime PARENT_SCOPE)
endfunction()

if(DEFINED IRLSAFETY_ONNX_ANDROID_ROOT AND IRLSAFETY_ONNX_ANDROID_ROOT)
  set(_ort_module "${IRLSAFETY_ONNX_ANDROID_ROOT}/prefab/modules/onnxruntime")
  if(EXISTS "${_ort_module}/libs/android.${ANDROID_ABI}/libonnxruntime.so")
    irlsafety_android_link_ort_imported("${_ort_module}")
    set(IRLSAFETY_HAS_ONNX_RUNTIME TRUE)
    message(STATUS "IRLSAFETY+: ONNX Runtime linked from ${IRLSAFETY_ONNX_ANDROID_ROOT}")
    return()
  endif()
endif()

find_package(onnxruntime CONFIG QUIET)

if(TARGET onnxruntime::onnxruntime)
  set(IRLSAFETY_HAS_ONNX_RUNTIME TRUE)
  set(IRLSAFETY_ONNX_LINK_TARGET onnxruntime::onnxruntime)
  message(STATUS "IRLSAFETY+: ONNX Runtime Android prefab linked (onnxruntime::onnxruntime)")
  return()
endif()

message(FATAL_ERROR "IRLSAFETY+: ONNX Runtime not found — set IRLSAFETY_ONNX_ANDROID_ROOT or enable prefab")