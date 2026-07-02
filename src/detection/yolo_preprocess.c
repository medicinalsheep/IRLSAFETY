/*
 * IRLSAFETY+ — YOLO letterbox preprocessing.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "yolo_preprocess.h"

#include <string.h>

static inline float clampf(float v, float lo, float hi)
{
	if (v < lo)
		return lo;
	if (v > hi)
		return hi;
	return v;
}

static void sample_bgra(const irlsafety_frame_view *frame, float fx, float fy, float *r, float *g, float *b)
{
	uint32_t x0 = (uint32_t)fx;
	uint32_t y0 = (uint32_t)fy;
	uint32_t x1 = x0 + 1;
	uint32_t y1 = y0 + 1;
	float dx = fx - (float)x0;
	float dy = fy - (float)y0;
	const uint8_t *p00;
	const uint8_t *p10;
	const uint8_t *p01;
	const uint8_t *p11;
	float b00, b10, b01, b11;
	float g00, g10, g01, g11;
	float r00, r10, r01, r11;

	if (x0 >= frame->width)
		x0 = frame->width - 1;
	if (y0 >= frame->height)
		y0 = frame->height - 1;
	if (x1 >= frame->width)
		x1 = frame->width - 1;
	if (y1 >= frame->height)
		y1 = frame->height - 1;

	p00 = frame->planes[0] + y0 * frame->linesize[0] + x0 * 4;
	p10 = frame->planes[0] + y0 * frame->linesize[0] + x1 * 4;
	p01 = frame->planes[0] + y1 * frame->linesize[0] + x0 * 4;
	p11 = frame->planes[0] + y1 * frame->linesize[0] + x1 * 4;

	b00 = (float)p00[0];
	g00 = (float)p00[1];
	r00 = (float)p00[2];
	b10 = (float)p10[0];
	g10 = (float)p10[1];
	r10 = (float)p10[2];
	b01 = (float)p01[0];
	g01 = (float)p01[1];
	r01 = (float)p01[2];
	b11 = (float)p11[0];
	g11 = (float)p11[1];
	r11 = (float)p11[2];

	*b = ((1.0f - dx) * (1.0f - dy) * b00 + dx * (1.0f - dy) * b10 + (1.0f - dx) * dy * b01 + dx * dy * b11) /
	     255.0f;
	*g = ((1.0f - dx) * (1.0f - dy) * g00 + dx * (1.0f - dy) * g10 + (1.0f - dx) * dy * g01 + dx * dy * g11) /
	     255.0f;
	*r = ((1.0f - dx) * (1.0f - dy) * r00 + dx * (1.0f - dy) * r10 + (1.0f - dx) * dy * r01 + dx * dy * r11) /
	     255.0f;
}

static void sample_i420(const irlsafety_frame_view *frame, float fx, float fy, float *r, float *g, float *b)
{
	uint32_t x = (uint32_t)clampf(fx, 0.0f, (float)(frame->width - 1));
	uint32_t y = (uint32_t)clampf(fy, 0.0f, (float)(frame->height - 1));
	uint32_t uv_x = x / 2;
	uint32_t uv_y = y / 2;
	uint8_t y_val = frame->planes[0][y * frame->linesize[0] + x];
	uint8_t u_val = frame->planes[1][uv_y * frame->linesize[1] + uv_x];
	uint8_t v_val = frame->planes[2][uv_y * frame->linesize[2] + uv_x];
	int c = (int)y_val - 16;
	int d = (int)u_val - 128;
	int e = (int)v_val - 128;
	int ri = (298 * c + 409 * e + 128) >> 8;
	int gi = (298 * c - 100 * d - 208 * e + 128) >> 8;
	int bi = (298 * c + 516 * d + 128) >> 8;

	*r = (float)(ri < 0 ? 0 : ri > 255 ? 255 : ri) / 255.0f;
	*g = (float)(gi < 0 ? 0 : gi > 255 ? 255 : gi) / 255.0f;
	*b = (float)(bi < 0 ? 0 : bi > 255 ? 255 : bi) / 255.0f;
}

static void sample_nv12(const irlsafety_frame_view *frame, float fx, float fy, float *r, float *g, float *b)
{
	uint32_t x = (uint32_t)clampf(fx, 0.0f, (float)(frame->width - 1));
	uint32_t y = (uint32_t)clampf(fy, 0.0f, (float)(frame->height - 1));
	uint32_t uv_x = (x / 2) * 2;
	uint32_t uv_y = y / 2;
	uint8_t y_val = frame->planes[0][y * frame->linesize[0] + x];
	uint8_t u_val = frame->planes[1][uv_y * frame->linesize[1] + uv_x];
	uint8_t v_val = frame->planes[1][uv_y * frame->linesize[1] + uv_x + 1];
	int c = (int)y_val - 16;
	int d = (int)u_val - 128;
	int e = (int)v_val - 128;
	int ri = (298 * c + 409 * e + 128) >> 8;
	int gi = (298 * c - 100 * d - 208 * e + 128) >> 8;
	int bi = (298 * c + 516 * d + 128) >> 8;

	*r = (float)(ri < 0 ? 0 : ri > 255 ? 255 : ri) / 255.0f;
	*g = (float)(gi < 0 ? 0 : gi > 255 ? 255 : gi) / 255.0f;
	*b = (float)(bi < 0 ? 0 : bi > 255 ? 255 : bi) / 255.0f;
}

static void sample_pixel(const irlsafety_frame_view *frame, float fx, float fy, float *r, float *g, float *b)
{
	if (frame->format == IRLSAFETY_FORMAT_I420) {
		sample_i420(frame, fx, fy, r, g, b);
		return;
	}

	if (frame->format == IRLSAFETY_FORMAT_NV12) {
		sample_nv12(frame, fx, fy, r, g, b);
		return;
	}

	if (frame->format == IRLSAFETY_FORMAT_BGRA || frame->format == IRLSAFETY_FORMAT_BGRX) {
		sample_bgra(frame, fx, fy, r, g, b);
		return;
	}

	*r = *g = *b = 0.0f;
}

int yolo_frame_to_tensor(const irlsafety_frame_view *frame, float *tensor_nchw, uint32_t tensor_w, uint32_t tensor_h,
			 yolo_letterbox *out_letterbox)
{
	float scale;
	float pad_x;
	float pad_y;
	size_t plane = (size_t)tensor_w * (size_t)tensor_h;

	if (!frame || !tensor_nchw || tensor_w == 0 || tensor_h == 0 || !frame->planes[0])
		return -1;

	scale = (float)tensor_w / (float)frame->width;
	if ((float)tensor_h / (float)frame->height < scale)
		scale = (float)tensor_h / (float)frame->height;

	pad_x = ((float)tensor_w - (float)frame->width * scale) * 0.5f;
	pad_y = ((float)tensor_h - (float)frame->height * scale) * 0.5f;

	if (out_letterbox) {
		out_letterbox->scale = scale;
		out_letterbox->pad_x = pad_x;
		out_letterbox->pad_y = pad_y;
		out_letterbox->src_width = frame->width;
		out_letterbox->src_height = frame->height;
	}

	for (uint32_t y = 0; y < tensor_h; y++) {
		for (uint32_t x = 0; x < tensor_w; x++) {
			float src_x = ((float)x - pad_x) / scale;
			float src_y = ((float)y - pad_y) / scale;
			float r = 0.0f;
			float g = 0.0f;
			float b = 0.0f;
			size_t idx = (size_t)y * tensor_w + x;

			if (src_x >= 0.0f && src_y >= 0.0f && src_x < (float)frame->width && src_y < (float)frame->height)
				sample_pixel(frame, src_x, src_y, &r, &g, &b);

			tensor_nchw[idx] = r;
			tensor_nchw[plane + idx] = g;
			tensor_nchw[plane * 2 + idx] = b;
		}
	}

	return 0;
}

void yolo_unmap_box(float cx, float cy, float w, float h, const yolo_letterbox *letterbox, irlsafety_rect *out_rect)
{
	float x1;
	float y1;
	float x2;
	float y2;

	if (!letterbox || !out_rect)
		return;

	x1 = (cx - w * 0.5f - letterbox->pad_x) / letterbox->scale;
	y1 = (cy - h * 0.5f - letterbox->pad_y) / letterbox->scale;
	x2 = (cx + w * 0.5f - letterbox->pad_x) / letterbox->scale;
	y2 = (cy + h * 0.5f - letterbox->pad_y) / letterbox->scale;

	if (x1 < 0.0f)
		x1 = 0.0f;
	if (y1 < 0.0f)
		y1 = 0.0f;
	if (x2 > (float)letterbox->src_width)
		x2 = (float)letterbox->src_width;
	if (y2 > (float)letterbox->src_height)
		y2 = (float)letterbox->src_height;

	out_rect->x = x1;
	out_rect->y = y1;
	out_rect->width = x2 - x1;
	out_rect->height = y2 - y1;
	if (out_rect->width < 1.0f)
		out_rect->width = 1.0f;
	if (out_rect->height < 1.0f)
		out_rect->height = 1.0f;
}