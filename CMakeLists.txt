cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
option(ENABLE_QT "Use Qt functionality" OFF)
option(IRLSAFETY_BUILD_TESTS "Build pipeline stub unit tests" ON)
option(IRLSAFETY_ENABLE_ONNX "Enable YOLO ONNX detection (license plates, signs)" ON)
if(OS_WINDOWS)
  set(_irlsafety_ocr_backend_default "auto")
else()
  set(_irlsafety_ocr_backend_default "child")
endif()
set(IRLSAFETY_OCR_BACKEND "${_irlsafety_ocr_backend_default}" CACHE STRING "OCR backend: auto, windows, child, stub")
set_property(CACHE IRLSAFETY_OCR_BACKEND PROPERTY STRINGS auto windows child stub)

include(compilerconfig)
include(defaults)
include(helpers)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/onnxruntime_detect.cmake")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/libirlsafety.cmake")

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs libirlsafety)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE IRLSAFETY_HAS_FRONTEND_API=1)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core Gui)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

set(_irlsafety_plugin_sources
    src/plugin-main.c
    src/pii-filter.c
    src/frame_convert.c
    src/gpu_frame.c
    src/irlsafety_control.c
    src/filter_property_ui.c
    src/filter_settings.c
    src/irlsafety_obs_adapter.c)

if(ENABLE_FRONTEND_API AND ENABLE_QT)
  list(APPEND _irlsafety_plugin_sources
              src/ui/control_widget.cpp
              src/ui/control_dock.cpp
              src/ui/tray_panel.cpp
              src/ui/ui_register.cpp
              src/ui/ui_theme.cpp
              src/ui/onboarding_dialog.cpp
              src/ui/censor_log_dialog.cpp
              src/ui/irlsafety_resources.qrc)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE IRLSAFETY_HAS_CONTROL_DOCK=1)
endif()

target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${_irlsafety_plugin_sources})

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(IRLSAFETY_HAS_ONNX_RUNTIME AND OS_WINDOWS)
  file(GLOB _ort_runtime_dlls "${IRLSAFETY_ONNXRUNTIME_DLL_DIR}/*.dll")
  # Ensure rundir exists first — OBS template copy steps may run after this
  # POST_BUILD, and cmake -E copy_if_different fails if the dest dir is missing.
  add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/rundir/$<CONFIG>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_ort_runtime_dlls}
            "${CMAKE_CURRENT_BINARY_DIR}/rundir/$<CONFIG>"
    COMMENT "Copy ONNX Runtime DLLs to rundir")
  # Ship ORT next to the plugin in CI packages (install prefix layout).
  install(FILES ${_ort_runtime_dlls} DESTINATION "${CMAKE_PROJECT_NAME}/bin/64bit")
endif()

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

if(IRLSAFETY_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()