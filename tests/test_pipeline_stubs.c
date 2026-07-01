/*
 * IRLSAFETY+ — unit tests for pipeline stub entry points.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "blur/blur_compositor.h"
#include "custom_pii.h"
#include "detection/yolo_onnx.h"
#include "filter_settings.h"
#include "ocr/ocr_engine.h"
#include "pipeline.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Stub for pipeline logging when plugin-support is not linked. */
void obs_log(int log_level, const char *format, ...)
{
	(void)log_level;
	(void)format;
}

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

static int test_custom_pii_load(void)
{
	irlsafety_custom_pii_list list;
	const char *inline_text = "Alice\n# comment\nBob\n";

	TEST_ASSERT(irlsafety_custom_pii_load(inline_text, NULL, &list) == 0);
	TEST_ASSERT(list.count == 2);
	TEST_ASSERT(strcmp(list.entries[0], "Alice") == 0);
	TEST_ASSERT(strcmp(list.entries[1], "Bob") == 0);
	return 0;
}

static int test_frame_skip_logic(void)
{
	irlsafety_filter_settings settings = {.enable_all = true, .frame_skip = 2};

	TEST_ASSERT(irlsafety_filter_should_process_frame(&settings, 0) == true);
	TEST_ASSERT(irlsafety_filter_should_process_frame(&settings, 1) == false);
	TEST_ASSERT(irlsafety_filter_should_process_frame(&settings, 2) == true);

	settings.enable_all = false;
	TEST_ASSERT(irlsafety_filter_should_process_frame(&settings, 0) == false);
	return 0;
}

static int test_pipeline_process_frame(void)
{
	irlsafety_pipeline *pipeline = irlsafety_pipeline_create(NULL);
	uint8_t buffer[16] = {0};
	irlsafety_frame_view frame = make_test_frame(buffer);
	irlsafety_filter_settings settings;

	irlsafety_filter_settings_load(NULL, &settings);
	TEST_ASSERT(pipeline != NULL);
	TEST_ASSERT(irlsafety_pipeline_process_frame(pipeline, &frame, &settings) == 0);

	settings.enable_all = false;
	TEST_ASSERT(irlsafety_pipeline_process_frame(pipeline, &frame, &settings) == 0);

	irlsafety_pipeline_destroy(pipeline);
	return 0;
}

int main(void)
{
	if (test_detect_regions_returns_empty() != 0)
		return 1;
	if (test_custom_pii_load() != 0)
		return 1;
	if (test_frame_skip_logic() != 0)
		return 1;
	if (test_pipeline_process_frame() != 0)
		return 1;
	printf("IRLSAFETY+ pipeline stub tests passed.\n");
	return 0;
}