cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
option(ENABLE_QT "Use Qt functionality" OFF)
option(IRLSAFETY_BUILD_TESTS "Build pipeline stub unit tests" ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

set(_irlsafety_ocr_sources src/ocr/ocr_engine.c src/ocr/pii_match.c src/ocr/ocr_frame_util.c)

if(OS_WINDOWS)
  list(APPEND _irlsafety_ocr_sources src/ocr/ocr_windows.cpp)
else()
  list(APPEND _irlsafety_ocr_sources src/ocr/ocr_stub.c)
endif()

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE src/plugin-main.c
          src/pii-filter.c
          src/pipeline.c
          src/filter_settings.c
          src/custom_pii.c
          src/frame_convert.c
          src/irlsafety_types.c
          src/detection/yolo_onnx.c
          src/blur/blur_compositor.c
          ${_irlsafety_ocr_sources}
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(OS_WINDOWS)
  set_source_files_properties(src/ocr/ocr_windows.cpp PROPERTIES LANGUAGE CXX)
  target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_20)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE windowsapp)
endif()

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

if(IRLSAFETY_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()