/*
 * IRLSAFETY+ — frame conversion helpers for OCR input.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "ocr_frame_util.h"

#include <string.h>

void irlsafety_compute_ocr_size(uint32_t frame_width, uint32_t frame_height, uint32_t *ocr_width, uint32_t *ocr_height,
				float *scale_x, float *scale_y)
{
	float scale = 1.0f;

	if (!ocr_width || !ocr_height)
		return;

	if (frame_width > IRLSAFETY_OCR_MAX_WIDTH) {
		scale = (float)IRLSAFETY_OCR_MAX_WIDTH / (float)frame_width;
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

static inline uint8_t clamp_u8(int v)
{
	if (v < 0)
		return 0;
	if (v > 255)
		return 255;
	return (uint8_t)v;
}

static void yuv_to_bgra(uint8_t y, uint8_t u, uint8_t v, uint8_t *b, uint8_t *g, uint8_t *r)
{
	int c = (int)y - 16;
	int d = (int)u - 128;
	int e = (int)v - 128;

	*r = clamp_u8((298 * c + 409 * e + 128) >> 8);
	*g = clamp_u8((298 * c - 100 * d - 208 * e + 128) >> 8);
	*b = clamp_u8((298 * c + 516 * d + 128) >> 8);
}

static void sample_i420(const irlsafety_frame_view *frame, uint32_t x, uint32_t y, uint8_t *bgra_out)
{
	uint32_t fx = x;
	uint32_t fy = y;
	uint8_t y_val;
	uint8_t u_val;
	uint8_t v_val;
	uint32_t uv_x = fx / 2;
	uint32_t uv_y = fy / 2;

	if (fx >= frame->width)
		fx = frame->width - 1;
	if (fy >= frame->height)
		fy = frame->height - 1;

	y_val = frame->planes[0][fy * frame->linesize[0] + fx];
	u_val = frame->planes[1][uv_y * frame->linesize[1] + uv_x];
	v_val = frame->planes[2][uv_y * frame->linesize[2] + uv_x];
	yuv_to_bgra(y_val, u_val, v_val, &bgra_out[0], &bgra_out[1], &bgra_out[2]);
	bgra_out[3] = 255;
}

static void sample_nv12(const irlsafety_frame_view *frame, uint32_t x, uint32_t y, uint8_t *bgra_out)
{
	uint32_t fx = x;
	uint32_t fy = y;
	uint8_t y_val;
	uint8_t u_val;
	uint8_t v_val;
	uint32_t uv_x = (fx / 2) * 2;
	uint32_t uv_y = fy / 2;

	if (fx >= frame->width)
		fx = frame->width - 1;
	if (fy >= frame->height)
		fy = frame->height - 1;

	y_val = frame->planes[0][fy * frame->linesize[0] + fx];
	u_val = frame->planes[1][uv_y * frame->linesize[1] + uv_x];
	v_val = frame->planes[1][uv_y * frame->linesize[1] + uv_x + 1];
	yuv_to_bgra(y_val, u_val, v_val, &bgra_out[0], &bgra_out[1], &bgra_out[2]);
	bgra_out[3] = 255;
}

static void sample_bgra(const irlsafety_frame_view *frame, uint32_t x, uint32_t y, uint8_t *bgra_out)
{
	uint32_t fx = x;
	uint32_t fy = y;
	const uint8_t *src;

	if (fx >= frame->width)
		fx = frame->width - 1;
	if (fy >= frame->height)
		fy = frame->height - 1;

	src = frame->planes[0] + fy * frame->linesize[0] + fx * 4;
	bgra_out[0] = src[0];
	bgra_out[1] = src[1];
	bgra_out[2] = src[2];
	bgra_out[3] = 255;
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

			switch (frame->format) {
			case IRLSAFETY_FORMAT_I420:
				sample_i420(frame, src_x, src_y, dst);
				break;
			case IRLSAFETY_FORMAT_NV12:
				sample_nv12(frame, src_x, src_y, dst);
				break;
			case IRLSAFETY_FORMAT_BGRA:
			case IRLSAFETY_FORMAT_BGRX:
				sample_bgra(frame, src_x, src_y, dst);
				break;
			default:
				memset(dst, 0, 4);
				break;
			}
		}
	}

	return 0;
}