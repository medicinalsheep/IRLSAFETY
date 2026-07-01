/*
 * IRLSAFETY+ — OCR engine for text PII detection.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "../custom_pii.h"
#include "../irlsafety_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ocr_engine_context ocr_engine_context;

ocr_engine_context *ocr_engine_create(void);
void ocr_engine_destroy(ocr_engine_context *ctx);

/* Run OCR and return blur regions for custom PII matches (case-insensitive). */
int ocr_regions_for_custom_pii(ocr_engine_context *ctx, const irlsafety_frame_view *frame,
			       const irlsafety_custom_pii_list *custom_pii, irlsafety_region_list *out_regions);

/* General OCR path (screen text category — returns all text boxes in v0.1). */
int ocr_regions(ocr_engine_context *ctx, const irlsafety_frame_view *frame,
		const irlsafety_region_list *hint_regions, irlsafety_region_list *out_regions);

#ifdef __cplusplus
}
#endif