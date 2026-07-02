/*
 * IRLSAFETY+ — unit tests for pipeline, PII match, and blur.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "blur/blur_compositor.h"
#include "custom_pii.h"
#include "frame_sample.h"
#include "ocr/ocr_frame_util.h"
#include "detection/yolo_onnx.h"
#include "filter_settings.h"
#include "ocr/ocr_engine.h"
#include "ocr/pii_match.h"
#include "ocr/pii_patterns.h"
#include "pipeline.h"
#include "hybrid_delay.h"
#include "region_tracker.h"

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
	frame.linesize[0] = 16 * 4;
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

static int test_match_email_line_hit(void)
{
	irlsafety_ocr_hit_list hits = {0};
	irlsafety_custom_pii_list pii = {0};
	irlsafety_region_list regions = {0};

	strcpy(hits.hits[0].text, "Contact me at user@example.com today");
	hits.hits[0].x = 10.0f;
	hits.hits[0].y = 20.0f;
	hits.hits[0].width = 220.0f;
	hits.hits[0].height = 18.0f;
	hits.count = 1;

	strcpy(pii.entries[0], "user@example.com");
	pii.count = 1;

	TEST_ASSERT(irlsafety_match_custom_pii_hits(&hits, &pii, 1.0f, 1.0f, 0.30f, 1.0f, &regions) == 0);
	TEST_ASSERT(regions.count == 1);
	return 0;
}

static int test_cover_while_typing(void)
{
	irlsafety_ocr_hit_list hits = {0};
	irlsafety_custom_pii_list pii = {0};
	irlsafety_region_list regions = {0};
	irlsafety_filter_settings settings;

	irlsafety_filter_settings_load(NULL, &settings);
	settings.cat_custom_pii = true;
	settings.cover_while_typing = true;

	strcpy(pii.entries[0], "John Smith");
	pii.count = 1;

	strcpy(hits.hits[0].text, "J");
	hits.hits[0].x = 10.0f;
	hits.hits[0].y = 20.0f;
	hits.hits[0].width = 12.0f;
	hits.hits[0].height = 18.0f;
	hits.count = 1;

	TEST_ASSERT(irlsafety_filter_effective_partial_threshold(&settings) <= 0.02f);
	TEST_ASSERT(irlsafety_match_custom_pii_hits(&hits, &pii, 1.0f, 1.0f, 0.30f,
						    irlsafety_filter_effective_partial_threshold(&settings),
						    &regions) == 0);
	TEST_ASSERT(regions.count == 1);
	return 0;
}

static int test_partial_pii_threshold(void)
{
	irlsafety_ocr_hit_list hits = {0};
	irlsafety_custom_pii_list pii = {0};
	irlsafety_region_list regions = {0};

	strcpy(pii.entries[0], "John Smith");
	pii.count = 1;

	strcpy(hits.hits[0].text, "John Sm");
	hits.hits[0].x = 10.0f;
	hits.hits[0].y = 20.0f;
	hits.hits[0].width = 70.0f;
	hits.hits[0].height = 18.0f;
	hits.count = 1;

	TEST_ASSERT(irlsafety_keyword_coverage_ratio(hits.hits[0].text, pii.entries[0]) >= 0.50f);
	TEST_ASSERT(irlsafety_match_custom_pii_hits(&hits, &pii, 1.0f, 1.0f, 0.0f, 0.50f, &regions) == 0);
	TEST_ASSERT(regions.count == 1);
	TEST_ASSERT(regions.regions[0].width > 70.0f);

	strcpy(hits.hits[0].text, "John");
	regions.count = 0;
	TEST_ASSERT(irlsafety_keyword_coverage_ratio(hits.hits[0].text, pii.entries[0]) < 0.50f);
	TEST_ASSERT(irlsafety_match_custom_pii_hits(&hits, &pii, 1.0f, 1.0f, 0.0f, 0.50f, &regions) == 0);
	TEST_ASSERT(regions.count == 0);
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

	TEST_ASSERT(irlsafety_match_custom_pii_hits(&hits, &pii, 1.0f, 1.0f, 0.30f, 1.0f, &regions) == 0);
	TEST_ASSERT(regions.count == 1);
	TEST_ASSERT(regions.regions[0].width > 80.0f);
	return 0;
}

static int test_apply_censor_box_fills_pixels(void)
{
	blur_compositor_context *ctx = blur_compositor_create();
	uint8_t buffer[16 * 16 * 4];
	irlsafety_frame_view frame = make_test_frame(buffer);
	irlsafety_region_list regions = {0};
	irlsafety_censor_options options;
	irlsafety_rect region;

	memset(&options, 0, sizeof(options));
	options.mode = IRLSAFETY_CENSOR_BOX;
	options.blur_strength = 12.0f;
	options.color = 0xFF000000;

	region.x = 4.0f;
	region.y = 4.0f;
	region.width = 8.0f;
	region.height = 8.0f;
	region.confidence = 1.0f;

	for (size_t i = 0; i < sizeof(buffer); i++)
		buffer[i] = 255;
	regions.regions[0] = region;
	regions.count = 1;

	TEST_ASSERT(apply_censor(ctx, &frame, &regions, &options) == 0);
	TEST_ASSERT(buffer[(8 * 16 + 8) * 4] == 0);
	TEST_ASSERT(buffer[(8 * 16 + 8) * 4 + 1] == 0);
	TEST_ASSERT(buffer[(8 * 16 + 8) * 4 + 2] == 0);

	blur_compositor_destroy(ctx);
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

static int test_region_tracker_eviction(void)
{
	irlsafety_region_tracker tracker;
	irlsafety_region_list detected;
	irlsafety_region_list out;

	memset(&tracker, 0, sizeof(tracker));
	memset(&detected, 0, sizeof(detected));
	memset(&out, 0, sizeof(out));

	detected.regions[0] = (irlsafety_rect){.x = 10, .y = 10, .width = 40, .height = 20, .confidence = 1.0f};
	detected.count = 1;

	irlsafety_region_tracker_update(&tracker, &detected, 640, 480, 1);
	TEST_ASSERT(tracker.count == 1);

	irlsafety_region_tracker_update(&tracker, &detected, 640, 480, 1);
	TEST_ASSERT(tracker.count == 1);

	detected.count = 0;
	for (int i = 0; i < IRLSAFETY_TRACKER_MISS_EVICT - 1; i++) {
		irlsafety_region_tracker_update(&tracker, &detected, 640, 480, 1);
		TEST_ASSERT(tracker.count == 1);
	}

	irlsafety_region_tracker_update(&tracker, &detected, 640, 480, 1);
	TEST_ASSERT(tracker.count == 0);

	irlsafety_region_tracker_copy_regions(&tracker, &out);
	TEST_ASSERT(out.count == 0);
	return 0;
}

static int test_hybrid_delay_lookup(void)
{
	irlsafety_hybrid_delay_runtime runtime;
	irlsafety_region_list regions = {0};
	irlsafety_region_list delayed = {0};
	irlsafety_filter_settings settings;

	irlsafety_hybrid_delay_runtime_init(&runtime);
	irlsafety_filter_settings_load(NULL, &settings);
	settings.hybrid_delay_enable = true;
	settings.global_delay_sec = 0.5f;

	regions.regions[0] = (irlsafety_rect){.x = 40, .y = 40, .width = 120, .height = 24, .confidence = 1.0f};
	regions.count = 1;

	irlsafety_hybrid_delay_record(&runtime, 30, &regions, true);
	irlsafety_hybrid_delay_tick(&runtime, &settings, 45, 1, true);
	TEST_ASSERT(runtime.effective_delay_sec > 0.0);

	TEST_ASSERT(irlsafety_hybrid_delay_lookup(&runtime, 45, runtime.effective_delay_sec, &delayed) == true);
	TEST_ASSERT(delayed.count == 1);
	return 0;
}

static int test_pipeline_process_frame(void)
{
	irlsafety_pipeline *pipeline = irlsafety_pipeline_create();
	uint8_t buffer[16 * 16 * 4];
	irlsafety_frame_view frame = make_test_frame(buffer);
	irlsafety_filter_settings settings;

	memset(buffer, 128, sizeof(buffer));
	irlsafety_filter_settings_load(NULL, &settings);
	settings.cat_custom_pii = true;
	strcpy(settings.custom_pii_inline, "TestName\n");

	TEST_ASSERT(pipeline != NULL);
	TEST_ASSERT(irlsafety_pipeline_update_settings(pipeline, &settings) == 0);
	TEST_ASSERT(irlsafety_pipeline_detect_frame(pipeline, &frame, &settings, 0, frame.width, frame.height, false) ==
		    0);
	TEST_ASSERT(irlsafety_pipeline_apply_cpu_censor(pipeline, &frame, &settings, frame.width, frame.height) == 0);

	settings.enable_all = false;
	TEST_ASSERT(irlsafety_pipeline_detect_frame(pipeline, &frame, &settings, 1, frame.width, frame.height, false) ==
		    0);

	irlsafety_pipeline_destroy(pipeline);
	return 0;
}

static int test_sensitive_patterns(void)
{
	irlsafety_ocr_hit_list hits = {0};
	irlsafety_region_list regions = {0};

	TEST_ASSERT(irlsafety_text_has_sensitive_pattern("Card 4111 1111 1111 1111") == true);
	TEST_ASSERT(irlsafety_sensitive_pattern_classify("Tracking 1Z999AA10123456784") ==
		    IRLSAFETY_PATTERN_TRACKING_NUMBER);
	TEST_ASSERT(irlsafety_sensitive_pattern_classify("SSN 123-45-6789") == IRLSAFETY_PATTERN_SSN);
	TEST_ASSERT(irlsafety_text_has_sensitive_pattern("hello world") == false);

	strcpy(hits.hits[0].text, "UPS 1Z999AA10123456784");
	hits.hits[0].x = 5.0f;
	hits.hits[0].y = 10.0f;
	hits.hits[0].width = 180.0f;
	hits.hits[0].height = 20.0f;
	hits.count = 1;

	TEST_ASSERT(irlsafety_match_sensitive_pattern_hits(&hits, 1.0f, 1.0f, 0.0f, &regions) == 0);
	TEST_ASSERT(regions.count == 1);
	return 0;
}

static int test_yuy2_frame_read_and_censor(void)
{
	uint8_t yuy2[16] = {235, 128, 235, 128, 235, 128, 235, 128, 16, 128, 16, 128, 16, 128, 16, 128};
	irlsafety_frame_view frame = {0};
	uint8_t bgra[32];
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	blur_compositor_context *ctx;
	irlsafety_region_list regions = {0};
	irlsafety_censor_options opts = {.mode = IRLSAFETY_CENSOR_BOX, .color = 0xFF000000, .blur_strength = 12.0f};

	frame.planes[0] = yuy2;
	frame.linesize[0] = 8;
	frame.width = 4;
	frame.height = 2;
	frame.format = IRLSAFETY_FORMAT_YUY2;
	frame.plane_count = 1;

	irlsafety_frame_read_rgb(&frame, 0, 0, &r, &g, &b);
	TEST_ASSERT(r > 200);
	irlsafety_frame_read_rgb(&frame, 0, 1, &r, &g, &b);
	TEST_ASSERT(r < 50);

	TEST_ASSERT(irlsafety_frame_to_bgra_scaled(&frame, bgra, 4, 2) == 0);
	TEST_ASSERT(bgra[2] > 200);

	ctx = blur_compositor_create();
	regions.regions[0] = (irlsafety_rect){.x = 0.0f, .y = 0.0f, .width = 2.0f, .height = 2.0f, .confidence = 1.0f};
	regions.count = 1;
	TEST_ASSERT(apply_censor(ctx, &frame, &regions, &opts) == 0);
	TEST_ASSERT(irlsafety_yuy2_read_y(yuy2, 0) < 32);
	blur_compositor_destroy(ctx);
	return 0;
}

int main(void)
{
	if (test_yuy2_frame_read_and_censor() != 0)
		return 1;
	if (test_ci_contains() != 0)
		return 1;
	if (test_sensitive_patterns() != 0)
		return 1;
	if (test_match_custom_pii_hits() != 0)
		return 1;
	if (test_partial_pii_threshold() != 0)
		return 1;
	if (test_cover_while_typing() != 0)
		return 1;
	if (test_match_email_line_hit() != 0)
		return 1;
	if (test_apply_censor_box_fills_pixels() != 0)
		return 1;
	if (test_apply_blur_modifies_pixels() != 0)
		return 1;
	if (test_region_tracker_eviction() != 0)
		return 1;
	if (test_hybrid_delay_lookup() != 0)
		return 1;
	if (test_pipeline_process_frame() != 0)
		return 1;
	printf("IRLSAFETY+ pipeline stub tests passed.\n");
	return 0;
}