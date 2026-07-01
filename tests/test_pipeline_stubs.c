/*
 * IRLSAFETY+ — unit tests for pipeline, PII match, and blur.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "blur/blur_compositor.h"
#include "custom_pii.h"
#include "detection/yolo_onnx.h"
#include "filter_settings.h"
#include "ocr/ocr_engine.h"
#include "ocr/pii_match.h"
#include "pipeline.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define TEST_ASSERT(cond)                                                                          \
	do {                                                                                       \
		if (!(cond)) {                                                                     \
			fprintf(stderr, "FAIL: %s (%s:%d)\n", #cond, __FILE__, __LINE__);          \
			return 1;                                                                  \
		}                                                                                  \
	} while (0)

void obs_log(int log_level, const char *format, ...)
{
	(void)log_level;
	(void)format;
}

static irlsafety_frame_view make_test_frame(uint8_t *buffer)
{
	irlsafety_frame_view frame = {0};
	frame.planes[0] = buffer;
	frame.linesize[0] = 16;
	frame.width = 16;
	frame.height = 16;
	frame.format = IRLSAFETY_FORMAT_BGRA;
	frame.plane_count = 1;
	return frame;
}

static int test_ci_contains(void)
{
	TEST_ASSERT(irlsafety_ci_contains("Hello John Smith", "john") == true);
	TEST_ASSERT(irlsafety_ci_contains("JANE DOE", "jane doe") == true);
	TEST_ASSERT(irlsafety_ci_contains("no match here", "zzzz") == false);
	return 0;
}

static int test_match_custom_pii_hits(void)
{
	irlsafety_ocr_hit_list hits = {0};
	irlsafety_custom_pii_list pii = {0};
	irlsafety_region_list regions = {0};

	strcpy(hits.hits[0].text, "ID: John Smith");
	hits.hits[0].x = 10.0f;
	hits.hits[0].y = 20.0f;
	hits.hits[0].width = 80.0f;
	hits.hits[0].height = 24.0f;
	hits.count = 1;

	strcpy(pii.entries[0], "John Smith");
	pii.count = 1;

	TEST_ASSERT(irlsafety_match_custom_pii_hits(&hits, &pii, 1.0f, 1.0f, &regions) == 0);
	TEST_ASSERT(regions.count == 1);
	TEST_ASSERT(regions.regions[0].width > 80.0f);
	return 0;
}

static int test_apply_blur_modifies_pixels(void)
{
	blur_compositor_context *ctx = blur_compositor_create();
	uint8_t buffer[16 * 16 * 4];
	irlsafety_frame_view frame = make_test_frame(buffer);
	irlsafety_region_list regions = {0};
	uint8_t before;

	for (size_t i = 0; i < sizeof(buffer); i++)
		buffer[i] = (uint8_t)(i * 17);
	regions.regions[0] = (irlsafety_rect){.x = 4, .y = 4, .width = 8, .height = 8, .confidence = 1.0f};
	regions.count = 1;

	before = buffer[(8 * 16 + 8) * 4];
	TEST_ASSERT(apply_blur(ctx, &frame, &regions, 12.0f) == 0);
	TEST_ASSERT(buffer[(8 * 16 + 8) * 4] != before);

	blur_compositor_destroy(ctx);
	return 0;
}

static int test_pipeline_process_frame(void)
{
	irlsafety_pipeline *pipeline = irlsafety_pipeline_create(NULL);
	uint8_t buffer[16 * 16 * 4];
	irlsafety_frame_view frame = make_test_frame(buffer);
	irlsafety_filter_settings settings;

	memset(buffer, 128, sizeof(buffer));
	irlsafety_filter_settings_load(NULL, &settings);
	settings.cat_custom_pii = true;
	strcpy(settings.custom_pii_inline, "TestName\n");

	TEST_ASSERT(pipeline != NULL);
	TEST_ASSERT(irlsafety_pipeline_update_settings(pipeline, &settings) == 0);
	TEST_ASSERT(irlsafety_pipeline_process_frame(pipeline, &frame, &settings) == 0);

	settings.enable_all = false;
	TEST_ASSERT(irlsafety_pipeline_process_frame(pipeline, &frame, &settings) == 0);

	irlsafety_pipeline_destroy(pipeline);
	return 0;
}

int main(void)
{
	if (test_ci_contains() != 0)
		return 1;
	if (test_match_custom_pii_hits() != 0)
		return 1;
	if (test_apply_blur_modifies_pixels() != 0)
		return 1;
	if (test_pipeline_process_frame() != 0)
		return 1;
	printf("IRLSAFETY+ pipeline stub tests passed.\n");
	return 0;
}