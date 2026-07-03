add_executable(test_pipeline_stubs test_pipeline_stubs.c mocks/obs-mock.c ../src/filter_settings.c)
target_link_libraries(test_pipeline_stubs PRIVATE libirlsafety_test)
target_compile_definitions(test_pipeline_stubs PRIVATE IRLSAFETY_TEST_BUILD)
target_include_directories(
  test_pipeline_stubs PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/mocks" "${CMAKE_CURRENT_SOURCE_DIR}/../src")
add_test(NAME pipeline_stubs COMMAND test_pipeline_stubs)

add_executable(test_obs_filter test_obs_filter.c mocks/obs-mock.c mocks/plugin-support-test.c
                                             mocks/irlsafety_obs_adapter_test.c ../src/plugin-main.c ../src/pii-filter.c
                                             ../src/frame_convert.c ../src/filter_settings.c)
target_link_libraries(test_obs_filter PRIVATE libirlsafety_test)
target_compile_definitions(test_obs_filter PRIVATE IRLSAFETY_TEST_BUILD)
target_include_directories(
  test_obs_filter
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/mocks" "${CMAKE_CURRENT_SOURCE_DIR}/../src")
add_test(NAME obs_filter_entry_points COMMAND test_obs_filter)