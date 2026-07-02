/*
 * IRLSAFETY+ — in-place censor compositor (blur / box / ellipse).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "blur_compositor.h"

#include <math.h>
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

/* OBS color: AARRGGBB in hex, stored as little-endian bytes (vec4_from_rgba). */
static void color_bytes_from_obs(uint32_t color, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a)
{
	uint8_t bytes[4];

	memcpy(bytes, &color, sizeof(bytes));
	*r = bytes[0];
	*g = bytes[1];
	*b = bytes[2];
	*a = bytes[3];
	/* color picker without alpha stores 0xRRGGBB — force opaque for censor fills. */
	if (*a == 0)
		*a = 255;
}

static void rgb_to_yuv(uint8_t r, uint8_t g, uint8_t b, uint8_t *y, uint8_t *u, uint8_t *v)
{
	int yi = (int)(0.299f * (float)r + 0.587f * (float)g + 0.114f * (float)b);
	int ui = (int)(-0.169f * (float)r - 0.331f * (float)g + 0.500f * (float)b + 128.0f);
	int vi = (int)(0.500f * (float)r - 0.419f * (float)g - 0.081f * (float)b + 128.0f);

	if (yi < 0)
		yi = 0;
	if (yi > 255)
		yi = 255;
	if (ui < 0)
		ui = 0;
	if (ui > 255)
		ui = 255;
	if (vi < 0)
		vi = 0;
	if (vi > 255)
		vi = 255;

	*y = (uint8_t)yi;
	*u = (uint8_t)ui;
	*v = (uint8_t)vi;
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

static void fill_pixel_bgra(uint8_t *plane, uint32_t linesize, uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b,
			    uint8_t a)
{
	uint8_t *px = plane + y * linesize + x * 4;
	px[0] = b;
	px[1] = g;
	px[2] = r;
	px[3] = a;
}

static void fill_pixel_rgba(uint8_t *plane, uint32_t linesize, uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b,
			    uint8_t a)
{
	uint8_t *px = plane + y * linesize + x * 4;
	px[0] = r;
	px[1] = g;
	px[2] = b;
	px[3] = a;
}

static void blur_rect_rgba(irlsafety_frame_view *frame, const irlsafety_rect *rect, int radius)
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

typedef enum irlsafety_fill_shape {
	IRLSAFETY_FILL_RECT = 0,
	IRLSAFETY_FILL_ELLIPSE = 1,
	IRLSAFETY_FILL_CLOUD = 2,
} irlsafety_fill_shape;

typedef struct cloud_blob {
	float cx;
	float cy;
	float radius;
} cloud_blob;

static const cloud_blob k_cloud_blobs[] = {
	{0.34f, 0.58f, 0.30f}, {0.52f, 0.50f, 0.36f}, {0.70f, 0.56f, 0.28f},
	{0.46f, 0.70f, 0.30f}, {0.26f, 0.64f, 0.22f}, {0.60f, 0.34f, 0.24f},
};

static bool point_in_cloud(float nx, float ny)
{
	size_t i;

	for (i = 0; i < sizeof(k_cloud_blobs) / sizeof(k_cloud_blobs[0]); i++) {
		float dx = nx - k_cloud_blobs[i].cx;
		float dy = ny - k_cloud_blobs[i].cy;
		float r = k_cloud_blobs[i].radius;

		if ((dx * dx + dy * dy) <= (r * r))
			return true;
	}

	return false;
}

static bool point_in_shape(float fx, float fy, const irlsafety_rect *rect, irlsafety_fill_shape shape)
{
	float cx = rect->x + rect->width * 0.5f;
	float cy = rect->y + rect->height * 0.5f;
	float rx = rect->width * 0.5f;
	float ry = rect->height * 0.5f;

	if (shape == IRLSAFETY_FILL_RECT)
		return true;

	if (rx < 1.0f)
		rx = 1.0f;
	if (ry < 1.0f)
		ry = 1.0f;

	if (shape == IRLSAFETY_FILL_ELLIPSE) {
		float dx = (fx - cx) / rx;
		float dy = (fy - cy) / ry;
		return (dx * dx + dy * dy) <= 1.0f;
	}

	if (rect->width > 0.0f && rect->height > 0.0f) {
		float nx = (fx - rect->x) / rect->width;
		float ny = (fy - rect->y) / rect->height;
		return point_in_cloud(nx, ny);
	}

	return false;
}

static void fill_rect_packed(irlsafety_frame_view *frame, const irlsafety_rect *rect, uint32_t color,
			     irlsafety_fill_shape shape, bool rgba_order)
{
	uint8_t r, g, b, a;
	uint32_t x0, y0, x1, y1;

	color_bytes_from_obs(color, &r, &g, &b, &a);

	x0 = (uint32_t)rect->x;
	y0 = (uint32_t)rect->y;
	x1 = x0 + (uint32_t)rect->width;
	y1 = y0 + (uint32_t)rect->height;
	if (x1 > frame->width)
		x1 = frame->width;
	if (y1 > frame->height)
		y1 = frame->height;

	for (uint32_t row = y0; row < y1; row++) {
		for (uint32_t col = x0; col < x1; col++) {
			if (!point_in_shape((float)col + 0.5f, (float)row + 0.5f, rect, shape))
				continue;
			if (rgba_order)
				fill_pixel_rgba(frame->planes[0], frame->linesize[0], col, row, r, g, b, a);
			else
				fill_pixel_bgra(frame->planes[0], frame->linesize[0], col, row, r, g, b, a);
		}
	}
}

static void fill_rect_bgra(irlsafety_frame_view *frame, const irlsafety_rect *rect, uint32_t color,
			   irlsafety_fill_shape shape)
{
	fill_rect_packed(frame, rect, color, shape, false);
}

static void fill_rect_rgba(irlsafety_frame_view *frame, const irlsafety_rect *rect, uint32_t color,
			   irlsafety_fill_shape shape)
{
	fill_rect_packed(frame, rect, color, shape, true);
}

static void fill_rect_yuv_plane(uint8_t *plane, uint32_t linesize, uint32_t plane_w, uint32_t plane_h, uint32_t x,
				uint32_t y, uint32_t w, uint32_t h, uint8_t value, const irlsafety_rect *region,
				irlsafety_fill_shape shape, float scale_x, float scale_y)
{
	uint32_t x0 = x;
	uint32_t y0 = y;
	uint32_t x1 = x + w;
	uint32_t y1 = y + h;

	if (x1 > plane_w)
		x1 = plane_w;
	if (y1 > plane_h)
		y1 = plane_h;

	for (uint32_t row = y0; row < y1; row++) {
		for (uint32_t col = x0; col < x1; col++) {
			float fx = ((float)col + 0.5f) * scale_x;
			float fy = ((float)row + 0.5f) * scale_y;

			if (!point_in_shape(fx, fy, region, shape))
				continue;
			plane[row * linesize + col] = value;
		}
	}
}

static void fill_rect_i420(irlsafety_frame_view *frame, const irlsafety_rect *rect, uint32_t color,
			   irlsafety_fill_shape shape)
{
	uint8_t b, g, r, a, y, u, v;

	color_bytes_from_obs(color, &r, &g, &b, &a);
	rgb_to_yuv(r, g, b, &y, &u, &v);

	fill_rect_yuv_plane(frame->planes[0], frame->linesize[0], frame->width, frame->height, (uint32_t)rect->x,
			    (uint32_t)rect->y, (uint32_t)rect->width, (uint32_t)rect->height, y, rect, shape, 1.0f,
			    1.0f);
	if (frame->plane_count >= 3) {
		fill_rect_yuv_plane(frame->planes[1], frame->linesize[1], frame->width / 2, frame->height / 2,
				    (uint32_t)rect->x / 2, (uint32_t)rect->y / 2, (uint32_t)rect->width / 2,
				    (uint32_t)rect->height / 2, u, rect, shape, 2.0f, 2.0f);
		fill_rect_yuv_plane(frame->planes[2], frame->linesize[2], frame->width / 2, frame->height / 2,
				    (uint32_t)rect->x / 2, (uint32_t)rect->y / 2, (uint32_t)rect->width / 2,
				    (uint32_t)rect->height / 2, v, rect, shape, 2.0f, 2.0f);
	}
}

static void fill_rect_nv12(irlsafety_frame_view *frame, const irlsafety_rect *rect, uint32_t color,
			   irlsafety_fill_shape shape)
{
	uint8_t b, g, r, a, y, u, v;

	color_bytes_from_obs(color, &r, &g, &b, &a);
	rgb_to_yuv(r, g, b, &y, &u, &v);

	fill_rect_yuv_plane(frame->planes[0], frame->linesize[0], frame->width, frame->height, (uint32_t)rect->x,
			    (uint32_t)rect->y, (uint32_t)rect->width, (uint32_t)rect->height, y, rect, shape, 1.0f,
			    1.0f);
	if (frame->plane_count >= 2) {
		uint32_t x0 = (uint32_t)rect->x & ~1u;
		uint32_t y0 = (uint32_t)rect->y & ~1u;
		uint32_t x1 = (x0 + (uint32_t)rect->width + 1u) & ~1u;
		uint32_t y1 = (y0 + (uint32_t)rect->height + 1u) & ~1u;

		for (uint32_t row = y0 / 2; row < y1 / 2 && row < frame->height / 2; row++) {
			for (uint32_t col = x0 / 2; col < x1 / 2 && col < frame->width / 2; col++) {
				float fx = (float)col * 2.0f + 1.0f;
				float fy = (float)row * 2.0f + 1.0f;

				if (!point_in_shape(fx, fy, rect, shape))
					continue;
				frame->planes[1][row * frame->linesize[1] + col * 2] = u;
				frame->planes[1][row * frame->linesize[1] + col * 2 + 1] = v;
			}
		}
	}
}

static void draw_preview_outline(irlsafety_frame_view *frame, const irlsafety_rect *rect)
{
	uint32_t x0 = (uint32_t)rect->x;
	uint32_t y0 = (uint32_t)rect->y;
	uint32_t x1 = x0 + (uint32_t)rect->width;
	uint32_t y1 = y0 + (uint32_t)rect->height;
	const uint8_t outline_bgra[4] = {0, 255, 0, 255};
	const uint8_t outline_rgba[4] = {0, 255, 0, 255};
	const uint8_t *outline = (frame->format == IRLSAFETY_FORMAT_RGBA) ? outline_rgba : outline_bgra;

	if (x1 > frame->width)
		x1 = frame->width;
	if (y1 > frame->height)
		y1 = frame->height;
	if (x0 >= x1 || y0 >= y1)
		return;

	for (uint32_t t = 0; t < 2; t++) {
		uint32_t top = y0 + t;
		uint32_t bottom = (y1 > t) ? y1 - 1 - t : y0;
		if (top < frame->height) {
			for (uint32_t col = x0; col < x1; col++)
				memcpy(frame->planes[0] + top * frame->linesize[0] + col * 4, outline, 4);
		}
		if (bottom < frame->height && bottom != top) {
			for (uint32_t col = x0; col < x1; col++)
				memcpy(frame->planes[0] + bottom * frame->linesize[0] + col * 4, outline, 4);
		}
		for (uint32_t row = y0; row < y1; row++) {
			if (x0 + t < x1)
				memcpy(frame->planes[0] + row * frame->linesize[0] + (x0 + t) * 4, outline, 4);
			if (x1 > x0 + t + 1)
				memcpy(frame->planes[0] + row * frame->linesize[0] + (x1 - 1 - t) * 4, outline, 4);
		}
	}
}

static void blur_region(irlsafety_frame_view *frame, const irlsafety_rect *region, int radius)
{
	irlsafety_rect rect = *region;

	clamp_rect(&rect, frame->width, frame->height);

	switch (frame->format) {
	case IRLSAFETY_FORMAT_RGBA:
		blur_rect_rgba(frame, &rect, radius);
		break;
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

static irlsafety_fill_shape shape_for_mode(irlsafety_censor_mode mode)
{
	switch (mode) {
	case IRLSAFETY_CENSOR_ELLIPSE:
		return IRLSAFETY_FILL_ELLIPSE;
	case IRLSAFETY_CENSOR_CLOUD:
		return IRLSAFETY_FILL_CLOUD;
	default:
		return IRLSAFETY_FILL_RECT;
	}
}

static void fill_region(irlsafety_frame_view *frame, const irlsafety_rect *region, uint32_t color,
			irlsafety_fill_shape shape)
{
	irlsafety_rect rect = *region;

	clamp_rect(&rect, frame->width, frame->height);

	switch (frame->format) {
	case IRLSAFETY_FORMAT_RGBA:
		fill_rect_rgba(frame, &rect, color, shape);
		break;
	case IRLSAFETY_FORMAT_BGRA:
	case IRLSAFETY_FORMAT_BGRX:
		fill_rect_bgra(frame, &rect, color, shape);
		break;
	case IRLSAFETY_FORMAT_I420:
		fill_rect_i420(frame, &rect, color, shape);
		break;
	case IRLSAFETY_FORMAT_NV12:
		fill_rect_nv12(frame, &rect, color, shape);
		break;
	default:
		fill_rect_bgra(frame, &rect, color, shape);
		break;
	}
}

static void censor_region(irlsafety_frame_view *frame, const irlsafety_rect *region, const irlsafety_censor_options *options,
			  int radius)
{
	if (options->mode == IRLSAFETY_CENSOR_BLUR)
		blur_region(frame, region, radius);
	else
		fill_region(frame, region, options->color, shape_for_mode(options->mode));
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

int apply_censor(blur_compositor_context *ctx, irlsafety_frame_view *frame, const irlsafety_region_list *regions,
		 const irlsafety_censor_options *options)
{
	int radius;

	(void)ctx;

	if (!frame || !regions || !options)
		return -1;

	if (regions->count == 0)
		return 0;

	radius = radius_from_strength(options->blur_strength);

	for (size_t i = 0; i < regions->count; i++)
		censor_region(frame, &regions->regions[i], options, radius);

	if (options->show_preview) {
		for (size_t i = 0; i < regions->count; i++) {
			if (frame->format == IRLSAFETY_FORMAT_RGBA || frame->format == IRLSAFETY_FORMAT_BGRA ||
			    frame->format == IRLSAFETY_FORMAT_BGRX)
				draw_preview_outline(frame, &regions->regions[i]);
		}
	}

	return 0;
}

int apply_blur(blur_compositor_context *ctx, irlsafety_frame_view *frame, const irlsafety_region_list *regions,
	       float blur_strength)
{
	irlsafety_censor_options options = {
		.mode = IRLSAFETY_CENSOR_BLUR,
		.blur_strength = blur_strength,
		.color = 0xFF000000,
		.show_preview = false,
	};

	return apply_censor(ctx, frame, regions, &options);
}