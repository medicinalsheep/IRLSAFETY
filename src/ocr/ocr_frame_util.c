/*
 * IRLSAFETY+ — frame conversion helpers for OCR input.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "ocr_frame_util.h"

#include "../frame_sample.h"

#include <string.h>

uint32_t irlsafety_ocr_max_width_for_detail(int ocr_detail)
{
	switch (ocr_detail) {
	case 0:
		return IRLSAFETY_OCR_WIDTH_STANDARD;
	case 2:
		return IRLSAFETY_OCR_WIDTH_MAXIMUM;
	default:
		return IRLSAFETY_OCR_WIDTH_DETAILED;
	}
}

void irlsafety_compute_ocr_size(uint32_t frame_width, uint32_t frame_height, uint32_t max_width, uint32_t *ocr_width,
				uint32_t *ocr_height, float *scale_x, float *scale_y)
{
	float scale = 1.0f;

	if (!ocr_width || !ocr_height)
		return;

	if (max_width == 0) {
		*ocr_width = frame_width;
		*ocr_height = frame_height;
		if (scale_x)
			*scale_x = 1.0f;
		if (scale_y)
			*scale_y = 1.0f;
		return;
	}

	if (frame_width > max_width) {
		scale = (float)max_width / (float)frame_width;
	}

	*ocr_width = (uint32_t)((float)frame_width * scale);
	*ocr_height = (uint32_t)((float)frame_height * scale);
	if (*ocr_width < 1)
		*ocr_width = 1;
	if (*ocr_height < 1)
		*ocr_height = 1;

	if (scale_x)
		*scale_x = (float)frame_width / (float)*ocr_width;
	if (scale_y)
		*scale_y = (float)frame_height / (float)*ocr_height;
}

int irlsafety_frame_to_bgra_scaled(const irlsafety_frame_view *frame, uint8_t *bgra, uint32_t ocr_width,
				   uint32_t ocr_height)
{
	if (!frame || !bgra || ocr_width == 0 || ocr_height == 0)
		return -1;

	for (uint32_t y = 0; y < ocr_height; y++) {
		uint32_t src_y = (uint32_t)((float)y * (float)frame->height / (float)ocr_height);
		for (uint32_t x = 0; x < ocr_width; x++) {
			uint32_t src_x = (uint32_t)((float)x * (float)frame->width / (float)ocr_width);
			uint8_t *dst = bgra + (y * ocr_width + x) * 4;
			uint8_t r = 0;
			uint8_t g = 0;
			uint8_t b = 0;

			irlsafety_frame_read_rgb(frame, src_x, src_y, &r, &g, &b);
			dst[0] = b;
			dst[1] = g;
			dst[2] = r;
			dst[3] = 255;
		}
	}

	return 0;
}