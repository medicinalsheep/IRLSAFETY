/*
 * IRLSAFETY+ — unit tests for pipeline stub entry points.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "blur/blur_compositor.h"
#include "detection/yolo_onnx.h"
#include "ocr/ocr_engine.h"
#include "pipeline.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_detect_regions_returns_empty(void)
{
	yolo_onnx_context *ctx = yolo_onnx_create("models/yolo.onnx");
	irlsafety_region_list regions = {.count = 99};
	irlsafety_frame_view frame = {0};

	assert(ctx != NULL);
	assert(detect_regions(ctx, &frame, &regions) == 0);
	assert(regions.count == 0);
	yolo_onnx_destroy(ctx);
}

static void test_ocr_regions_returns_empty(void)
{
	ocr_engine_context *ctx = ocr_engine_create();
	irlsafety_region_list regions = {.count = 99};
	irlsafety_frame_view frame = {0};

	assert(ctx != NULL);
	assert(ocr_regions(ctx, &frame, NULL, &regions) == 0);
	assert(regions.count == 0);
	ocr_engine_destroy(ctx);
}

static void test_apply_blur_noop(void)
{
	blur_compositor_context *ctx = blur_compositor_create();
	uint8_t buffer[16] = {0};
	irlsafety_frame_view frame = {.data = buffer, .width = 4, .height = 4, .linesize = 4};

	assert(ctx != NULL);
	assert(apply_blur(ctx, &frame, NULL, 8.0f) == 0);
	blur_compositor_destroy(ctx);
}

static void test_pipeline_process_frame(void)
{
	irlsafety_pipeline *pipeline = irlsafety_pipeline_create(NULL);
	uint8_t buffer[16] = {0};
	irlsafety_frame_view frame = {.data = buffer, .width = 4, .height = 4, .linesize = 4};

	assert(pipeline != NULL);
	assert(irlsafety_pipeline_process_frame(pipeline, &frame, 8.0f) == 0);
	irlsafety_pipeline_destroy(pipeline);
}

int main(void)
{
	test_detect_regions_returns_empty();
	test_ocr_regions_returns_empty();
	test_apply_blur_noop();
	test_pipeline_process_frame();
	printf("IRLSAFETY+ pipeline stub tests passed.\n");
	return 0;
}