/*
 * IRLSAFETY+ — shared pixel sampling for all video formats.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "frame_sample.h"

static inline uint8_t clamp_u8(int v)
{
	if (v < 0)
		return 0;
	if (v > 255)
		return 255;
	return (uint8_t)v;
}

void irlsafety_yuv601_to_rgb(uint8_t y, uint8_t u, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
	int c = (int)y - 16;
	int d = (int)u - 128;
	int e = (int)v - 128;

	if (r)
		*r = clamp_u8((298 * c + 409 * e + 128) >> 8);
	if (g)
		*g = clamp_u8((298 * c - 100 * d - 208 * e + 128) >> 8);
	if (b)
		*b = clamp_u8((298 * c + 516 * d + 128) >> 8);
}

void irlsafety_rgb_to_yuv601(uint8_t r, uint8_t g, uint8_t b, uint8_t *y, uint8_t *u, uint8_t *v)
{
	int yi = (int)(0.299f * (float)r + 0.587f * (float)g + 0.114f * (float)b);
	int ui = (int)(-0.169f * (float)r - 0.331f * (float)g + 0.500f * (float)b + 128.0f);
	int vi = (int)(0.500f * (float)r - 0.419f * (float)g - 0.081f * (float)b + 128.0f);

	if (y)
		*y = clamp_u8(yi);
	if (u)
		*u = clamp_u8(ui);
	if (v)
		*v = clamp_u8(vi);
}

static void read_yuy2(const uint8_t *row, uint32_t x, uint8_t *y_out, uint8_t *u_out, uint8_t *v_out)
{
	uint32_t pair = x & ~1u;
	uint8_t y_val = row[x * 2];
	uint8_t u_val;
	uint8_t v_val;

	if ((x & 1u) == 0) {
		u_val = row[pair * 2 + 1];
		v_val = row[pair * 2 + 3];
	} else {
		u_val = row[pair * 2 + 1];
		v_val = row[pair * 2 + 3];
	}

	if (y_out)
		*y_out = y_val;
	if (u_out)
		*u_out = u_val;
	if (v_out)
		*v_out = v_val;
}

static void read_uyvy(const uint8_t *row, uint32_t x, uint8_t *y_out, uint8_t *u_out, uint8_t *v_out)
{
	uint32_t pair = x & ~1u;
	uint8_t y_val = row[pair * 2 + 1 + (x & 1u)];
	uint8_t u_val = row[pair * 2];
	uint8_t v_val = row[pair * 2 + 2];

	if (y_out)
		*y_out = y_val;
	if (u_out)
		*u_out = u_val;
	if (v_out)
		*v_out = v_val;
}

void irlsafety_yuy2_write_pixel(uint8_t *row, uint32_t x, uint8_t y, uint8_t u, uint8_t v)
{
	uint32_t pair = x & ~1u;

	row[x * 2] = y;
	row[pair * 2 + 1] = u;
	row[pair * 2 + 3] = v;
}

void irlsafety_uyvy_write_pixel(uint8_t *row, uint32_t x, uint8_t y, uint8_t u, uint8_t v)
{
	uint32_t pair = x & ~1u;

	row[pair * 2] = u;
	row[pair * 2 + 1 + (x & 1u)] = y;
	row[pair * 2 + 2] = v;
}

uint8_t irlsafety_yuy2_read_y(const uint8_t *row, uint32_t x)
{
	return row[x * 2];
}

uint8_t irlsafety_uyvy_read_y(const uint8_t *row, uint32_t x)
{
	return row[(x & ~1u) * 2 + 1 + (x & 1u)];
}

static void read_i420(const irlsafety_frame_view *frame, uint32_t x, uint32_t y, uint8_t *r, uint8_t *g, uint8_t *b)
{
	uint32_t uv_x = x / 2;
	uint32_t uv_y = y / 2;
	uint8_t y_val = frame->planes[0][y * frame->linesize[0] + x];
	uint8_t u_val = frame->planes[1][uv_y * frame->linesize[1] + uv_x];
	uint8_t v_val = frame->planes[2][uv_y * frame->linesize[2] + uv_x];

	irlsafety_yuv601_to_rgb(y_val, u_val, v_val, r, g, b);
}

static void read_nv12(const irlsafety_frame_view *frame, uint32_t x, uint32_t y, uint8_t *r, uint8_t *g, uint8_t *b)
{
	uint32_t uv_x = (x / 2) * 2;
	uint32_t uv_y = y / 2;
	uint8_t y_val = frame->planes[0][y * frame->linesize[0] + x];
	uint8_t u_val = frame->planes[1][uv_y * frame->linesize[1] + uv_x];
	uint8_t v_val = frame->planes[1][uv_y * frame->linesize[1] + uv_x + 1];

	irlsafety_yuv601_to_rgb(y_val, u_val, v_val, r, g, b);
}

static void read_bgra(const irlsafety_frame_view *frame, uint32_t x, uint32_t y, uint8_t *r, uint8_t *g, uint8_t *b)
{
	const uint8_t *src = frame->planes[0] + y * frame->linesize[0] + x * 4;

	if (b)
		*b = src[0];
	if (g)
		*g = src[1];
	if (r)
		*r = src[2];
}

static void read_rgba(const irlsafety_frame_view *frame, uint32_t x, uint32_t y, uint8_t *r, uint8_t *g, uint8_t *b)
{
	const uint8_t *src = frame->planes[0] + y * frame->linesize[0] + x * 4;

	if (r)
		*r = src[0];
	if (g)
		*g = src[1];
	if (b)
		*b = src[2];
}

void irlsafety_frame_read_rgb(const irlsafety_frame_view *frame, uint32_t x, uint32_t y, uint8_t *r, uint8_t *g,
			      uint8_t *b)
{
	uint32_t fx = x;
	uint32_t fy = y;
	uint8_t y_val;
	uint8_t u_val;
	uint8_t v_val;

	if (!frame || !frame->planes[0] || frame->width == 0 || frame->height == 0)
		return;

	if (fx >= frame->width)
		fx = frame->width - 1;
	if (fy >= frame->height)
		fy = frame->height - 1;

	switch (frame->format) {
	case IRLSAFETY_FORMAT_I420:
		read_i420(frame, fx, fy, r, g, b);
		break;
	case IRLSAFETY_FORMAT_NV12:
		read_nv12(frame, fx, fy, r, g, b);
		break;
	case IRLSAFETY_FORMAT_BGRA:
	case IRLSAFETY_FORMAT_BGRX:
		read_bgra(frame, fx, fy, r, g, b);
		break;
	case IRLSAFETY_FORMAT_RGBA:
		read_rgba(frame, fx, fy, r, g, b);
		break;
	case IRLSAFETY_FORMAT_YUY2:
		read_yuy2(frame->planes[0] + fy * frame->linesize[0], fx, &y_val, &u_val, &v_val);
		irlsafety_yuv601_to_rgb(y_val, u_val, v_val, r, g, b);
		break;
	case IRLSAFETY_FORMAT_UYVY:
		read_uyvy(frame->planes[0] + fy * frame->linesize[0], fx, &y_val, &u_val, &v_val);
		irlsafety_yuv601_to_rgb(y_val, u_val, v_val, r, g, b);
		break;
	default:
		if (r)
			*r = 0;
		if (g)
			*g = 0;
		if (b)
			*b = 0;
		break;
	}
}

void irlsafety_frame_sample_rgb_f(const irlsafety_frame_view *frame, float fx, float fy, float *r, float *g, float *b)
{
	uint32_t x;
	uint32_t y;
	uint8_t ri = 0;
	uint8_t gi = 0;
	uint8_t bi = 0;

	if (!frame)
		return;

	if (fx < 0.0f)
		fx = 0.0f;
	if (fy < 0.0f)
		fy = 0.0f;
	if (fx >= (float)frame->width)
		fx = (float)(frame->width - 1);
	if (fy >= (float)frame->height)
		fy = (float)(frame->height - 1);

	x = (uint32_t)fx;
	y = (uint32_t)fy;
	irlsafety_frame_read_rgb(frame, x, y, &ri, &gi, &bi);

	if (r)
		*r = (float)ri / 255.0f;
	if (g)
		*g = (float)gi / 255.0f;
	if (b)
		*b = (float)bi / 255.0f;
}