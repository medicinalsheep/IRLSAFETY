/*
 * IRLSAFETY+ — OCR engine for text PII detection.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "../custom_pii.h"
#include "../irlsafety_types.h"
#include "ocr_text.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ocr_engine_context ocr_engine_context;

ocr_engine_context *ocr_engine_create(void);
void ocr_engine_destroy(ocr_engine_context *ctx);

/* Run OCR once; hit coordinates are in OCR space — multiply by scale_x/y for frame pixels. */
int ocr_recognize_hits(ocr_engine_context *ctx, const irlsafety_frame_view *frame, irlsafety_ocr_hit_list *out_hits,
		       float *scale_x, float *scale_y);

int ocr_submit_hits(ocr_engine_context *ctx, const irlsafety_frame_view *frame, uint32_t max_width, uint64_t *job_id,
		    float *scale_x, float *scale_y, bool cache_source);

/* Second pass from cached full-res frame (Maximum / dual-scan mode). */
int ocr_submit_cached_hits(ocr_engine_context *ctx, uint32_t max_width, uint32_t source_width, uint32_t source_height,
			   uint64_t *job_id, float *scale_x, float *scale_y);

void ocr_clear_cached_source(ocr_engine_context *ctx);

/* 1 = ready, 0 = pending, -1 = error. */
int ocr_poll_hits(uint64_t job_id, irlsafety_ocr_hit_list *out_hits);

/* Run OCR and return blur regions for custom PII matches (case-insensitive). */
int ocr_regions_for_custom_pii(ocr_engine_context *ctx, const irlsafety_frame_view *frame,
			       const irlsafety_custom_pii_list *custom_pii, irlsafety_region_list *out_regions);

/* General OCR path (screen text category — returns all text boxes in v0.1). */
int ocr_regions(ocr_engine_context *ctx, const irlsafety_frame_view *frame,
		const irlsafety_region_list *hint_regions, irlsafety_region_list *out_regions);

#ifdef __cplusplus
}
#endif