/*
 * IRLSAFETY+ — frame conversion helpers for OCR input.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "../irlsafety_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IRLSAFETY_OCR_WIDTH_STANDARD 1280
#define IRLSAFETY_OCR_WIDTH_DETAILED 1920
#define IRLSAFETY_OCR_WIDTH_MAXIMUM 2560

uint32_t irlsafety_ocr_max_width_for_detail(int ocr_detail);

void irlsafety_compute_ocr_size(uint32_t frame_width, uint32_t frame_height, uint32_t max_width, uint32_t *ocr_width,
				uint32_t *ocr_height, float *scale_x, float *scale_y);

/* Convert any supported frame into a tightly packed BGRA buffer (ocr_w x ocr_h). */
int irlsafety_frame_to_bgra_scaled(const irlsafety_frame_view *frame, uint8_t *bgra, uint32_t ocr_width,
				   uint32_t ocr_height);

#ifdef __cplusplus
}
#endif