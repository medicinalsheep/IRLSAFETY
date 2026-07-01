/*
 * IRLSAFETY+ — OCR engine for text PII detection.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "ocr_engine.h"

#include "ocr_backend.h"
#include "ocr_frame_util.h"
#include "pii_match.h"

#include <stdlib.h>
#include <string.h>

struct ocr_engine_context {
	uint8_t *bgra_buffer;
	size_t bgra_capacity;
};

static int ensure_bgra_buffer(ocr_engine_context *ctx, uint32_t width, uint32_t height)
{
	size_t needed = (size_t)width * (size_t)height * 4;

	if (!ctx)
		return -1;

	if (ctx->bgra_capacity < needed) {
		uint8_t *resized = realloc(ctx->bgra_buffer, needed);
		if (!resized)
			return -1;
		ctx->bgra_buffer = resized;
		ctx->bgra_capacity = needed;
	}

	return 0;
}

static int ocr_run_on_frame(ocr_engine_context *ctx, const irlsafety_frame_view *frame,
			    irlsafety_ocr_hit_list *out_hits, float *scale_x, float *scale_y)
{
	uint32_t ocr_w = 0;
	uint32_t ocr_h = 0;

	if (!ctx || !frame || !out_hits)
		return -1;

	if (!ocr_backend_available())
		return -1;

	irlsafety_compute_ocr_size(frame->width, frame->height, &ocr_w, &ocr_h, scale_x, scale_y);
	if (ensure_bgra_buffer(ctx, ocr_w, ocr_h) != 0)
		return -1;

	if (irlsafety_frame_to_bgra_scaled(frame, ctx->bgra_buffer, ocr_w, ocr_h) != 0)
		return -1;

	out_hits->count = 0;
	return ocr_backend_recognize(ctx->bgra_buffer, ocr_w, ocr_h, ocr_w * 4, out_hits);
}

ocr_engine_context *ocr_engine_create(void)
{
	return calloc(1, sizeof(ocr_engine_context));
}

void ocr_engine_destroy(ocr_engine_context *ctx)
{
	if (!ctx)
		return;
	free(ctx->bgra_buffer);
	free(ctx);
}

int ocr_regions_for_custom_pii(ocr_engine_context *ctx, const irlsafety_frame_view *frame,
			       const irlsafety_custom_pii_list *custom_pii, irlsafety_region_list *out_regions)
{
	irlsafety_ocr_hit_list hits;
	float scale_x = 1.0f;
	float scale_y = 1.0f;

	if (!out_regions)
		return -1;

	out_regions->count = 0;

	if (!custom_pii || custom_pii->count == 0)
		return 0;

	if (ocr_run_on_frame(ctx, frame, &hits, &scale_x, &scale_y) != 0)
		return 0;

	return irlsafety_match_custom_pii_hits(&hits, custom_pii, scale_x, scale_y, out_regions);
}

int ocr_regions(ocr_engine_context *ctx, const irlsafety_frame_view *frame,
		const irlsafety_region_list *hint_regions, irlsafety_region_list *out_regions)
{
	irlsafety_ocr_hit_list hits;
	float scale_x = 1.0f;
	float scale_y = 1.0f;

	(void)hint_regions;

	if (!out_regions)
		return -1;

	out_regions->count = 0;

	if (ocr_run_on_frame(ctx, frame, &hits, &scale_x, &scale_y) != 0)
		return 0;

	for (size_t i = 0; i < hits.count && out_regions->count < IRLSAFETY_MAX_REGIONS; i++) {
		irlsafety_rect rect = {
			.x = hits.hits[i].x * scale_x,
			.y = hits.hits[i].y * scale_y,
			.width = hits.hits[i].width * scale_x,
			.height = hits.hits[i].height * scale_y,
			.confidence = 1.0f,
		};
		out_regions->regions[out_regions->count++] = rect;
	}

	return 0;
}