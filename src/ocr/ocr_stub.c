/*
 * IRLSAFETY+ — OCR backend stub (non-Windows / unit tests).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "ocr_backend.h"

void ocr_backend_shutdown(void)
{
}

uint32_t ocr_backend_last_error(void)
{
	return 0;
}

bool ocr_backend_available(void)
{
	return false;
}

int ocr_backend_submit(const uint8_t *bgra, uint32_t width, uint32_t height, uint32_t stride, uint64_t *out_job_id)
{
	(void)bgra;
	(void)width;
	(void)height;
	(void)stride;

	if (out_job_id)
		*out_job_id = 0;
	return -1;
}

int ocr_backend_poll(uint64_t job_id, irlsafety_ocr_hit_list *out_hits)
{
	(void)job_id;

	if (!out_hits)
		return -1;

	out_hits->count = 0;
	return -1;
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