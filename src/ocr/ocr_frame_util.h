/*
 * IRLSAFETY+ — frame conversion helpers for OCR input.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "../irlsafety_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Downscale width cap for OCR to keep real-time performance acceptable. */
#define IRLSAFETY_OCR_MAX_WIDTH 960

void irlsafety_compute_ocr_size(uint32_t frame_width, uint32_t frame_height, uint32_t *ocr_width, uint32_t *ocr_height,
				float *scale_x, float *scale_y);

/* Convert any supported frame into a tightly packed BGRA buffer (ocr_w x ocr_h). */
int irlsafety_frame_to_bgra_scaled(const irlsafety_frame_view *frame, uint8_t *bgra, uint32_t ocr_width,
				   uint32_t ocr_height);

#ifdef __cplusplus
}
#endif