/*
 * IRLSAFETY+ — PNG overlay images for custom censor patterns.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "overlay_image.h"

#include "../irlsafety_paths.h"

#include <stdlib.h>
#include <string.h>

#define STBI_WINDOWS_UTF8
#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb_image.h"

#ifndef IRLSAFETY_TEST_BUILD
#include <obs-module.h>
#include <util/bmem.h>
#endif

static irlsafety_overlay_image *g_overlay_cache = NULL;
static char g_overlay_cache_path[1024] = {0};

void irlsafety_overlay_resolve_path(const char *user_path, char *dest, size_t dest_size)
{
	if (!dest || dest_size == 0)
		return;

	dest[0] = '\0';

	if (user_path && user_path[0] != '\0') {
		strncpy(dest, user_path, dest_size - 1);
		dest[dest_size - 1] = '\0';
		return;
	}

#ifndef IRLSAFETY_TEST_BUILD
	irlsafety_resolve_model_path(IRLSAFETY_OVERLAY_BUNDLED_REL, NULL, dest, dest_size);
#else
	strncpy(dest, "data/overlays/sample-censored.png", dest_size - 1);
	dest[dest_size - 1] = '\0';
#endif
}

static void overlay_free(irlsafety_overlay_image *img)
{
	if (!img)
		return;
	stbi_image_free(img->rgba);
	free(img);
}

static irlsafety_overlay_image *overlay_load_file(const char *path)
{
	irlsafety_overlay_image *img;
	int w = 0;
	int h = 0;
	int comp = 0;
	uint8_t *pixels;

	if (!path || path[0] == '\0')
		return NULL;

	stbi_set_flip_vertically_on_load(1);
	pixels = stbi_load(path, &w, &h, &comp, 4);
	if (!pixels || w <= 0 || h <= 0)
		return NULL;

	img = calloc(1, sizeof(*img));
	if (!img) {
		stbi_image_free(pixels);
		return NULL;
	}

	img->width = (uint32_t)w;
	img->height = (uint32_t)h;
	img->rgba = pixels;
	return img;
}

irlsafety_overlay_image *irlsafety_overlay_acquire(const char *user_path)
{
	char resolved[1024];

	irlsafety_overlay_resolve_path(user_path, resolved, sizeof(resolved));
	if (resolved[0] == '\0')
		return NULL;

	if (g_overlay_cache && strcmp(g_overlay_cache_path, resolved) == 0)
		return g_overlay_cache;

	irlsafety_overlay_release_cache();

	g_overlay_cache = overlay_load_file(resolved);
	if (!g_overlay_cache)
		return NULL;

	strncpy(g_overlay_cache_path, resolved, sizeof(g_overlay_cache_path) - 1);
	g_overlay_cache_path[sizeof(g_overlay_cache_path) - 1] = '\0';
	return g_overlay_cache;
}

void irlsafety_overlay_release_cache(void)
{
	overlay_free(g_overlay_cache);
	g_overlay_cache = NULL;
	g_overlay_cache_path[0] = '\0';
}

static inline uint8_t blend_channel(uint8_t dst, uint8_t src, uint8_t alpha)
{
	return (uint8_t)(((uint16_t)dst * (255 - alpha) + (uint16_t)src * alpha) / 255);
}

void irlsafety_overlay_apply_region_bgra(const irlsafety_overlay_image *overlay, irlsafety_frame_view *frame,
					 const irlsafety_rect *region)
{
	uint32_t x0;
	uint32_t y0;
	uint32_t x1;
	uint32_t y1;

	if (!overlay || !overlay->rgba || overlay->width == 0 || overlay->height == 0 || !frame || !region ||
	    !frame->planes[0])
		return;

	if (frame->format != IRLSAFETY_FORMAT_BGRA && frame->format != IRLSAFETY_FORMAT_BGRX &&
	    frame->format != IRLSAFETY_FORMAT_RGBA)
		return;

	x0 = region->x < 0.0f ? 0u : (uint32_t)region->x;
	y0 = region->y < 0.0f ? 0u : (uint32_t)region->y;
	x1 = (uint32_t)(region->x + region->width);
	y1 = (uint32_t)(region->y + region->height);

	if (x1 > frame->width)
		x1 = frame->width;
	if (y1 > frame->height)
		y1 = frame->height;
	if (x0 >= x1 || y0 >= y1)
		return;

	for (uint32_t y = y0; y < y1; y++) {
		float v = ((float)y - region->y) / region->height;
		uint32_t sy = (uint32_t)(v * (float)overlay->height);
		if (sy >= overlay->height)
			sy = overlay->height - 1;

		for (uint32_t x = x0; x < x1; x++) {
			float u = ((float)x - region->x) / region->width;
			uint32_t sx = (uint32_t)(u * (float)overlay->width);
			uint8_t *dst;
			const uint8_t *src;
			uint8_t alpha;

			if (sx >= overlay->width)
				sx = overlay->width - 1;

			src = overlay->rgba + ((size_t)sy * overlay->width + sx) * 4;
			alpha = src[3];
			if (alpha == 0)
				continue;

			dst = frame->planes[0] + (size_t)y * frame->linesize[0] + (size_t)x * 4;

			if (frame->format == IRLSAFETY_FORMAT_RGBA) {
				dst[0] = blend_channel(dst[0], src[0], alpha);
				dst[1] = blend_channel(dst[1], src[1], alpha);
				dst[2] = blend_channel(dst[2], src[2], alpha);
			} else {
				dst[0] = blend_channel(dst[0], src[2], alpha);
				dst[1] = blend_channel(dst[1], src[1], alpha);
				dst[2] = blend_channel(dst[2], src[0], alpha);
			}
			dst[3] = 255;
		}
	}
}