/*
 * IRLSAFETY+ — OCR backend stub (non-Windows / unit tests).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "ocr_backend.h"

bool ocr_backend_available(void)
{
	return false;
}

int ocr_backend_recognize(const uint8_t *bgra, uint32_t width, uint32_t height, uint32_t stride,
			  irlsafety_ocr_hit_list *out_hits)
{
	(void)bgra;
	(void)width;
	(void)height;
	(void)stride;

	if (!out_hits)
		return -1;

	out_hits->count = 0;
	return 0;
}