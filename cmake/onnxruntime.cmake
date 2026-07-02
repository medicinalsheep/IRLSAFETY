# Fetch/link ONNX Runtime for IRLSAFETY+ YOLO detection (Windows x64).

set(IRLSAFETY_ONNXRUNTIME_VERSION "1.20.1")
set(_ort_root "${CMAKE_CURRENT_SOURCE_DIR}/.deps/onnxruntime-win-x64-${IRLSAFETY_ONNXRUNTIME_VERSION}")
set(_ort_zip "${CMAKE_CURRENT_SOURCE_DIR}/.deps/onnxruntime-win-x64-${IRLSAFETY_ONNXRUNTIME_VERSION}.zip")
set(_ort_url
    "https://github.com/microsoft/onnxruntime/releases/download/v${IRLSAFETY_ONNXRUNTIME_VERSION}/onnxruntime-win-x64-${IRLSAFETY_ONNXRUNTIME_VERSION}.zip"
)

if(NOT EXISTS "${_ort_root}/include/onnxruntime_c_api.h")
  if(NOT EXISTS "${_ort_zip}")
    message(STATUS "IRLSAFETY+: downloading ONNX Runtime ${IRLSAFETY_ONNXRUNTIME_VERSION}...")
    file(DOWNLOAD "${_ort_url}" "${_ort_zip}" SHOW_PROGRESS TLS_VERIFY ON STATUS _ort_status)

    list(GET _ort_status 0 _ort_code)
    if(NOT _ort_code EQUAL 0)
      message(FATAL_ERROR "IRLSAFETY+: failed to download ONNX Runtime (${_ort_url})")
    endif()
  endif()

  message(STATUS "IRLSAFETY+: extracting ONNX Runtime...")
  file(ARCHIVE_EXTRACT INPUT "${_ort_zip}" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/.deps")
endif()

add_library(onnxruntime SHARED IMPORTED GLOBAL)
set_target_properties(
  onnxruntime
  PROPERTIES IMPORTED_IMPLIB "${_ort_root}/lib/onnxruntime.lib"
             IMPORTED_LOCATION "${_ort_root}/lib/onnxruntime.dll"
             INTERFACE_INCLUDE_DIRECTORIES "${_ort_root}/include")

set(IRLSAFETY_ONNXRUNTIME_DLL_DIR "${_ort_root}/lib" CACHE INTERNAL "")