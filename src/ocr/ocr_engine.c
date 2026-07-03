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
	uint8_t *source_bgra;
	size_t source_capacity;
	uint32_t source_width;
	uint32_t source_height;
	bool have_source_cache;
};

static int ensure_bgra_buffer(uint8_t **buffer, size_t *capacity, uint32_t width, uint32_t height)
{
	size_t needed = (size_t)width * (size_t)height * 4;
	uint8_t *resized;

	if (!buffer || !capacity)
		return -1;

	if (*capacity < needed) {
		resized = realloc(*buffer, needed);
		if (!resized)
			return -1;
		*buffer = resized;
		*capacity = needed;
	}

	return 0;
}

static int cache_source_frame(ocr_engine_context *ctx, const irlsafety_frame_view *frame)
{
	if (!ctx || !frame || frame->width == 0 || frame->height == 0)
		return -1;

	if (ensure_bgra_buffer(&ctx->source_bgra, &ctx->source_capacity, frame->width, frame->height) != 0)
		return -1;

	if ((frame->format == IRLSAFETY_FORMAT_BGRA || frame->format == IRLSAFETY_FORMAT_BGRX) &&
	    frame->planes[0] != NULL && frame->linesize[0] == frame->width * 4) {
		size_t needed = (size_t)frame->width * (size_t)frame->height * 4;
		memcpy(ctx->source_bgra, frame->planes[0], needed);
	} else if (irlsafety_frame_to_bgra_scaled(frame, ctx->source_bgra, frame->width, frame->height) != 0) {
		return -1;
	}

	ctx->source_width = frame->width;
	ctx->source_height = frame->height;
	ctx->have_source_cache = true;
	return 0;
}

static int prepare_ocr_frame(ocr_engine_context *ctx, const irlsafety_frame_view *frame, uint32_t max_width,
			     uint32_t *ocr_w, uint32_t *ocr_h, float *scale_x, float *scale_y, const uint8_t **pixels,
			     uint32_t *stride)
{
	if (!ctx || !frame || !ocr_w || !ocr_h || !pixels || !stride)
		return -1;

	if (!ocr_backend_available())
		return -1;

	irlsafety_compute_ocr_size(frame->width, frame->height, max_width, ocr_w, ocr_h, scale_x, scale_y);
	if (ensure_bgra_buffer(&ctx->bgra_buffer, &ctx->bgra_capacity, *ocr_w, *ocr_h) != 0)
		return -1;

	/* GPU readback already provides tightly packed BGRA at OCR dimensions. */
	if ((frame->format == IRLSAFETY_FORMAT_BGRA || frame->format == IRLSAFETY_FORMAT_BGRX) &&
	    frame->width == *ocr_w && frame->height == *ocr_h && frame->planes[0] != NULL &&
	    frame->linesize[0] == *ocr_w * 4) {
		*pixels = frame->planes[0];
		*stride = frame->linesize[0];
		return 0;
	}

	if (irlsafety_frame_to_bgra_scaled(frame, ctx->bgra_buffer, *ocr_w, *ocr_h) != 0)
		return -1;

	*pixels = ctx->bgra_buffer;
	*stride = *ocr_w * 4;
	return 0;
}

static int downscale_cached_source(ocr_engine_context *ctx, uint32_t max_width, uint32_t *ocr_w, uint32_t *ocr_h,
				   float *scale_x, float *scale_y, const uint8_t **pixels, uint32_t *stride)
{
	irlsafety_frame_view cached;

	if (!ctx || !ctx->have_source_cache || !ocr_w || !ocr_h || !pixels || !stride)
		return -1;

	memset(&cached, 0, sizeof(cached));
	cached.planes[0] = ctx->source_bgra;
	cached.linesize[0] = ctx->source_width * 4;
	cached.width = ctx->source_width;
	cached.height = ctx->source_height;
	cached.format = IRLSAFETY_FORMAT_BGRA;
	cached.plane_count = 1;

	return prepare_ocr_frame(ctx, &cached, max_width, ocr_w, ocr_h, scale_x, scale_y, pixels, stride);
}

int ocr_submit_hits(ocr_engine_context *ctx, const irlsafety_frame_view *frame, uint32_t max_width, uint64_t *job_id,
		    float *scale_x, float *scale_y, bool cache_source)
{
	uint32_t ocr_w = 0;
	uint32_t ocr_h = 0;
	const uint8_t *pixels = NULL;
	uint32_t stride = 0;

	if (!job_id)
		return -1;

	if (cache_source && cache_source_frame(ctx, frame) != 0)
		return -1;

	if (prepare_ocr_frame(ctx, frame, max_width, &ocr_w, &ocr_h, scale_x, scale_y, &pixels, &stride) != 0)
		return -1;

	return ocr_backend_submit(pixels, ocr_w, ocr_h, stride, job_id);
}

int ocr_submit_cached_hits(ocr_engine_context *ctx, uint32_t max_width, uint32_t source_width, uint32_t source_height,
			   uint64_t *job_id, float *scale_x, float *scale_y)
{
	uint32_t ocr_w = 0;
	uint32_t ocr_h = 0;
	const uint8_t *pixels = NULL;
	uint32_t stride = 0;

	if (!ctx || !job_id || !ctx->have_source_cache)
		return -1;

	if (ctx->source_width != source_width || ctx->source_height != source_height)
		return -1;

	if (downscale_cached_source(ctx, max_width, &ocr_w, &ocr_h, scale_x, scale_y, &pixels, &stride) != 0)
		return -1;

	return ocr_backend_submit(pixels, ocr_w, ocr_h, stride, job_id);
}

void ocr_clear_cached_source(ocr_engine_context *ctx)
{
	if (!ctx)
		return;

	ctx->have_source_cache = false;
	ctx->source_width = 0;
	ctx->source_height = 0;
}

int ocr_poll_hits(uint64_t job_id, irlsafety_ocr_hit_list *out_hits)
{
	if (!out_hits)
		return -1;

	return ocr_backend_poll(job_id, out_hits);
}

int ocr_recognize_hits(ocr_engine_context *ctx, const irlsafety_frame_view *frame, irlsafety_ocr_hit_list *out_hits,
		       float *scale_x, float *scale_y)
{
	uint32_t ocr_w = 0;
	uint32_t ocr_h = 0;
	const uint8_t *pixels = NULL;
	uint32_t stride = 0;

	if (!ctx || !frame || !out_hits)
		return -1;

	if (prepare_ocr_frame(ctx, frame, 0, &ocr_w, &ocr_h, scale_x, scale_y, &pixels, &stride) != 0)
		return -1;

	out_hits->count = 0;
	return ocr_backend_recognize(pixels, ocr_w, ocr_h, stride, out_hits);
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
	free(ctx->source_bgra);
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

	if (ocr_recognize_hits(ctx, frame, &hits, &scale_x, &scale_y) != 0)
		return 0;

	return irlsafety_match_custom_pii_hits(&hits, custom_pii, scale_x, scale_y, 0.50f, out_regions);
}

int ocr_regions(ocr_engine_context *ctx, const irlsafety_frame_view *frame, const irlsafety_region_list *hint_regions,
		irlsafety_region_list *out_regions)
{
	irlsafety_ocr_hit_list hits;
	float scale_x = 1.0f;
	float scale_y = 1.0f;

	(void)hint_regions;

	if (!out_regions)
		return -1;

	out_regions->count = 0;

	if (ocr_recognize_hits(ctx, frame, &hits, &scale_x, &scale_y) != 0)
		return 0;

	return irlsafety_regions_from_screen_text_hits(&hits, scale_x, scale_y, out_regions);
}