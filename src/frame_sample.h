/*
 * IRLSAFETY+ — shared pixel sampling for all video formats.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "irlsafety_types.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void irlsafety_yuv601_to_rgb(uint8_t y, uint8_t u, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b);
void irlsafety_rgb_to_yuv601(uint8_t r, uint8_t g, uint8_t b, uint8_t *y, uint8_t *u, uint8_t *v);

/* Read one pixel from a frame (integer coordinates). */
void irlsafety_frame_read_rgb(const irlsafety_frame_view *frame, uint32_t x, uint32_t y, uint8_t *r, uint8_t *g,
			      uint8_t *b);

/* Float-coordinate sample for detection preprocessing (returns 0..1). */
void irlsafety_frame_sample_rgb_f(const irlsafety_frame_view *frame, float fx, float fy, float *r, float *g, float *b);

/* Write solid YUV into packed camera buffers (used by censor fills). */
void irlsafety_yuy2_write_pixel(uint8_t *row, uint32_t x, uint8_t y, uint8_t u, uint8_t v);
void irlsafety_uyvy_write_pixel(uint8_t *row, uint32_t x, uint8_t y, uint8_t u, uint8_t v);

/* Read Y from packed formats (used by in-place blur). */
uint8_t irlsafety_yuy2_read_y(const uint8_t *row, uint32_t x);
uint8_t irlsafety_uyvy_read_y(const uint8_t *row, uint32_t x);

#ifdef __cplusplus
}
#endif