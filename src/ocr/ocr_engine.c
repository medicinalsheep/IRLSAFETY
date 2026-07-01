/*
 * IRLSAFETY+ — OCR engine stub for text PII detection.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "ocr_engine.h"

#include <stdlib.h>

struct ocr_engine_context {
	int placeholder;
};

ocr_engine_context *ocr_engine_create(void)
{
	return calloc(1, sizeof(ocr_engine_context));
}

void ocr_engine_destroy(ocr_engine_context *ctx)
{
	free(ctx);
}

int ocr_regions(ocr_engine_context *ctx, const irlsafety_frame_view *frame,
		const irlsafety_region_list *hint_regions, irlsafety_region_list *out_regions)
{
	(void)ctx;
	(void)frame;
	(void)hint_regions;

	if (!out_regions)
		return -1;

	out_regions->count = 0;
	return 0;
}