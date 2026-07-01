/*
 * IRLSAFETY+ — in-place box blur compositor.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "blur_compositor.h"

#include <stdlib.h>
#include <string.h>

struct blur_compositor_context {
	float default_strength;
};

static int radius_from_strength(float blur_strength)
{
	int radius = (int)(blur_strength * 0.75f);
	if (radius < 3)
		radius = 3;
	if (radius > 24)
		radius = 24;
	return radius;
}

static void clamp_rect(irlsafety_rect *rect, uint32_t frame_w, uint32_t frame_h)
{
	if (rect->x < 0.0f)
		rect->x = 0.0f;
	if (rect->y < 0.0f)
		rect->y = 0.0f;
	if (rect->x + rect->width > (float)frame_w)
		rect->width = (float)frame_w - rect->x;
	if (rect->y + rect->height > (float)frame_h)
		rect->height = (float)frame_h - rect->y;
	if (rect->width < 1.0f)
		rect->width = 1.0f;
	if (rect->height < 1.0f)
		rect->height = 1.0f;
}

static void box_blur_plane(uint8_t *plane, uint32_t linesize, uint32_t frame_w, uint32_t frame_h, uint32_t x, uint32_t y,
			   uint32_t w, uint32_t h, int radius)
{
	uint32_t x0 = x;
	uint32_t y0 = y;
	uint32_t x1 = x + w;
	uint32_t y1 = y + h;
	uint8_t *tmp;

	if (x1 > frame_w)
		x1 = frame_w;
	if (y1 > frame_h)
		y1 = frame_h;
	if (x0 >= x1 || y0 >= y1)
		return;

	tmp = malloc((size_t)(x1 - x0) * (size_t)(y1 - y0));
	if (!tmp)
		return;

	for (uint32_t row = y0; row < y1; row++) {
		for (uint32_t col = x0; col < x1; col++) {
			int sum = 0;
			int count = 0;

			for (int ky = -(int)radius; ky <= radius; ky++) {
				int sy = (int)row + ky;
				if (sy < 0 || sy >= (int)frame_h)
					continue;
				for (int kx = -(int)radius; kx <= radius; kx++) {
					int sx = (int)col + kx;
					if (sx < 0 || sx >= (int)frame_w)
						continue;
					sum += plane[sy * linesize + sx];
					count++;
				}
			}

			tmp[(row - y0) * (x1 - x0) + (col - x0)] = (uint8_t)(sum / (count > 0 ? count : 1));
		}
	}

	for (uint32_t row = y0; row < y1; row++)
		memcpy(plane + row * linesize + x0, tmp + (row - y0) * (x1 - x0), x1 - x0);

	free(tmp);
}

static void blur_rect_bgra(irlsafety_frame_view *frame, const irlsafety_rect *rect, int radius)
{
	uint32_t x = (uint32_t)rect->x;
	uint32_t y = (uint32_t)rect->y;
	uint32_t w = (uint32_t)rect->width;
	uint32_t h = (uint32_t)rect->height;
	uint8_t *plane = frame->planes[0];
	uint32_t linesize = frame->linesize[0];

	for (uint32_t row = y; row < y + h && row < frame->height; row++) {
		for (int pass = 0; pass < 3; pass++) {
			for (uint32_t col = x; col < x + w && col < frame->width; col++) {
				int sum = 0;
				int count = 0;
				for (int ky = -radius; ky <= radius; ky++) {
					int sy = (int)row + ky;
					if (sy < 0 || sy >= (int)frame->height)
						continue;
					for (int kx = -radius; kx <= radius; kx++) {
						int sx = (int)col + kx;
						if (sx < 0 || sx >= (int)frame->width)
							continue;
						sum += plane[sy * linesize + sx * 4 + pass];
						count++;
					}
				}
				plane[row * linesize + col * 4 + pass] = (uint8_t)(sum / (count > 0 ? count : 1));
			}
		}
	}
}

static void blur_region(irlsafety_frame_view *frame, const irlsafety_rect *region, int radius)
{
	irlsafety_rect rect = *region;

	clamp_rect(&rect, frame->width, frame->height);

	switch (frame->format) {
	case IRLSAFETY_FORMAT_BGRA:
	case IRLSAFETY_FORMAT_BGRX:
		blur_rect_bgra(frame, &rect, radius);
		break;
	case IRLSAFETY_FORMAT_I420:
		box_blur_plane(frame->planes[0], frame->linesize[0], frame->width, frame->height, (uint32_t)rect.x,
			       (uint32_t)rect.y, (uint32_t)rect.width, (uint32_t)rect.height, radius);
		if (frame->plane_count >= 3) {
			box_blur_plane(frame->planes[1], frame->linesize[1], frame->width / 2, frame->height / 2,
				       (uint32_t)rect.x / 2, (uint32_t)rect.y / 2, (uint32_t)rect.width / 2,
				       (uint32_t)rect.height / 2, radius / 2);
			box_blur_plane(frame->planes[2], frame->linesize[2], frame->width / 2, frame->height / 2,
				       (uint32_t)rect.x / 2, (uint32_t)rect.y / 2, (uint32_t)rect.width / 2,
				       (uint32_t)rect.height / 2, radius / 2);
		}
		break;
	case IRLSAFETY_FORMAT_NV12:
		box_blur_plane(frame->planes[0], frame->linesize[0], frame->width, frame->height, (uint32_t)rect.x,
			       (uint32_t)rect.y, (uint32_t)rect.width, (uint32_t)rect.height, radius);
		if (frame->plane_count >= 2) {
			box_blur_plane(frame->planes[1], frame->linesize[1], frame->width, frame->height / 2,
				       (uint32_t)rect.x, (uint32_t)rect.y / 2, (uint32_t)rect.width,
				       (uint32_t)rect.height / 2, radius / 2);
		}
		break;
	default:
		box_blur_plane(frame->planes[0], frame->linesize[0], frame->width, frame->height, (uint32_t)rect.x,
			       (uint32_t)rect.y, (uint32_t)rect.width, (uint32_t)rect.height, radius);
		break;
	}
}

blur_compositor_context *blur_compositor_create(void)
{
	blur_compositor_context *ctx = calloc(1, sizeof(*ctx));
	if (ctx)
		ctx->default_strength = 8.0f;
	return ctx;
}

void blur_compositor_destroy(blur_compositor_context *ctx)
{
	free(ctx);
}

int apply_blur(blur_compositor_context *ctx, irlsafety_frame_view *frame, const irlsafety_region_list *regions,
	       float blur_strength)
{
	int radius;

	(void)ctx;

	if (!frame || !regions)
		return -1;

	if (regions->count == 0)
		return 0;

	radius = radius_from_strength(blur_strength);

	for (size_t i = 0; i < regions->count; i++)
		blur_region(frame, &regions->regions[i], radius);

	return 0;
}