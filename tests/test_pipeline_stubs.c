/*
 * IRLSAFETY+ — unit tests for pipeline stub entry points.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "blur/blur_compositor.h"
#include "detection/yolo_onnx.h"
#include "ocr/ocr_engine.h"
#include "pipeline.h"

#include <stdio.h>
#include <string.h>

#define TEST_ASSERT(cond)                                                                          \
	do {                                                                                       \
		if (!(cond)) {                                                                     \
			fprintf(stderr, "FAIL: %s (%s:%d)\n", #cond, __FILE__, __LINE__);          \
			return 1;                                                                  \
		}                                                                                  \
	} while (0)

static irlsafety_frame_view make_test_frame(uint8_t *buffer)
{
	irlsafety_frame_view frame = {0};
	frame.planes[0] = buffer;
	frame.linesize[0] = 4;
	frame.width = 4;
	frame.height = 4;
	frame.format = IRLSAFETY_FORMAT_BGRA;
	frame.plane_count = 1;
	return frame;
}

static int test_detect_regions_returns_empty(void)
{
	yolo_onnx_context *ctx = yolo_onnx_create("models/yolo.onnx");
	irlsafety_region_list regions = {.count = 99};
	irlsafety_frame_view frame = {0};
	frame.plane_count = 0;

	TEST_ASSERT(ctx != NULL);
	TEST_ASSERT(detect_regions(ctx, &frame, &regions) == 0);
	TEST_ASSERT(regions.count == 0);
	yolo_onnx_destroy(ctx);
	return 0;
}

static int test_ocr_regions_returns_empty(void)
{
	ocr_engine_context *ctx = ocr_engine_create();
	irlsafety_region_list regions = {.count = 99};
	irlsafety_frame_view frame = {0};
	frame.plane_count = 0;

	TEST_ASSERT(ctx != NULL);
	TEST_ASSERT(ocr_regions(ctx, &frame, NULL, &regions) == 0);
	TEST_ASSERT(regions.count == 0);
	ocr_engine_destroy(ctx);
	return 0;
}

static int test_apply_blur_accepts_mutable_planes(void)
{
	blur_compositor_context *ctx = blur_compositor_create();
	uint8_t buffer[16] = {0};
	irlsafety_frame_view frame = make_test_frame(buffer);

	TEST_ASSERT(ctx != NULL);
	frame.planes[0][0] = 42;
	TEST_ASSERT(apply_blur(ctx, &frame, NULL, 8.0f) == 0);
	TEST_ASSERT(frame.planes[0][0] == 42);
	blur_compositor_destroy(ctx);
	return 0;
}

static int test_pipeline_process_frame(void)
{
	irlsafety_pipeline *pipeline = irlsafety_pipeline_create(NULL);
	uint8_t buffer[16] = {0};
	irlsafety_frame_view frame = make_test_frame(buffer);

	TEST_ASSERT(pipeline != NULL);
	TEST_ASSERT(irlsafety_pipeline_process_frame(pipeline, &frame, 8.0f) == 0);
	irlsafety_pipeline_destroy(pipeline);
	return 0;
}

int main(void)
{
	if (test_detect_regions_returns_empty() != 0)
		return 1;
	if (test_ocr_regions_returns_empty() != 0)
		return 1;
	if (test_apply_blur_accepts_mutable_planes() != 0)
		return 1;
	if (test_pipeline_process_frame() != 0)
		return 1;
	printf("IRLSAFETY+ pipeline stub tests passed.\n");
	return 0;
}