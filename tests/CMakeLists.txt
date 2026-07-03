# Test builds compile libirlsafety sources with IRLSAFETY_TEST_BUILD (see cmake/libirlsafety.cmake).

set(_irlsafety_test_core_sources)
foreach(_src IN LISTS IRLSAFETY_LIB_SOURCES)
  if(_src STREQUAL "src/detection/yolo_onnx.cpp")
    list(APPEND _irlsafety_test_core_sources "${CMAKE_CURRENT_SOURCE_DIR}/../src/detection/yolo_onnx_stub.c")
  elseif(_src MATCHES "^src/ocr/ocr_(windows|child)")
    list(APPEND _irlsafety_test_core_sources "${CMAKE_CURRENT_SOURCE_DIR}/../src/ocr/ocr_stub.c")
  else()
    list(APPEND _irlsafety_test_core_sources "${CMAKE_CURRENT_SOURCE_DIR}/../${_src}")
  endif()
endforeach()

add_executable(test_pipeline_stubs test_pipeline_stubs.c mocks/obs-mock.c ${_irlsafety_test_core_sources})
target_compile_definitions(test_pipeline_stubs PRIVATE IRLSAFETY_TEST_BUILD)
target_include_directories(
  test_pipeline_stubs PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/mocks" "${CMAKE_CURRENT_SOURCE_DIR}/../src")
add_test(NAME pipeline_stubs COMMAND test_pipeline_stubs)

set(_irlsafety_obs_test_sources
    test_obs_filter.c
    mocks/obs-mock.c
    mocks/plugin-support-test.c
    ../src/plugin-main.c
    ../src/pii-filter.c
    ../src/frame_convert.c
    ${_irlsafety_test_core_sources})

add_executable(test_obs_filter ${_irlsafety_obs_test_sources})
target_compile_definitions(test_obs_filter PRIVATE IRLSAFETY_TEST_BUILD)
target_include_directories(
  test_obs_filter
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/mocks" "${CMAKE_CURRENT_SOURCE_DIR}/../src")
add_test(NAME obs_filter_entry_points COMMAND test_obs_filter)