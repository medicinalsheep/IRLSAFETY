set(_irlsafety_core_sources
    ../src/pipeline.c
    ../src/region_tracker.c
    ../src/hybrid_delay.c
    ../src/filter_settings.c
    ../src/custom_pii.c
    ../src/irlsafety_types.c
    ../src/detection/yolo_onnx_stub.c
    ../src/detection/yolo_preprocess.c
    ../src/ocr/ocr_engine.c
    ../src/ocr/pii_match.c
    ../src/ocr/ocr_frame_util.c
    ../src/ocr/ocr_stub.c
    ../src/blur/blur_compositor.c)

add_executable(test_pipeline_stubs test_pipeline_stubs.c mocks/obs-mock.c ${_irlsafety_core_sources})
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
    ${_irlsafety_core_sources})

add_executable(test_obs_filter ${_irlsafety_obs_test_sources})
target_compile_definitions(test_obs_filter PRIVATE IRLSAFETY_TEST_BUILD)
target_include_directories(
  test_obs_filter
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/mocks" "${CMAKE_CURRENT_SOURCE_DIR}/../src")
add_test(NAME obs_filter_entry_points COMMAND test_obs_filter)