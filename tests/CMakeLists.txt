add_executable(test_pipeline_stubs test_pipeline_stubs.c
                                     ../src/pipeline.c
                                     ../src/detection/yolo_onnx.c
                                     ../src/ocr/ocr_engine.c
                                     ../src/blur/blur_compositor.c)

target_include_directories(test_pipeline_stubs PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")

add_test(NAME pipeline_stubs COMMAND test_pipeline_stubs)