/*
 * IRLSAFETY+ — OCR engine stub for text PII detection.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "../irlsafety_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ocr_engine_context ocr_engine_context;

ocr_engine_context *ocr_engine_create(void);
void ocr_engine_destroy(ocr_engine_context *ctx);

/* Planned: extract text regions and classify PII (names, emails, IDs, etc.). */
int ocr_regions(ocr_engine_context *ctx, const irlsafety_frame_view *frame,
		const irlsafety_region_list *hint_regions, irlsafety_region_list *out_regions);

#ifdef __cplusplus
}
#endif