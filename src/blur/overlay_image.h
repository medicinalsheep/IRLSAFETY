/*
 * IRLSAFETY+ — PNG overlay images for custom censor patterns.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "../irlsafety_types.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IRLSAFETY_OVERLAY_BUNDLED_REL "overlays/sample-censored.png"

typedef struct irlsafety_overlay_image {
	uint32_t width;
	uint32_t height;
	uint8_t *rgba;
} irlsafety_overlay_image;

void irlsafety_overlay_resolve_path(const char *user_path, char *dest, size_t dest_size);

/* Returns cached image; reloads when resolved path changes. NULL on failure. */
irlsafety_overlay_image *irlsafety_overlay_acquire(const char *user_path);

void irlsafety_overlay_release_cache(void);

/* Alpha-composite overlay stretched to fit region (RGBA source → BGRA/BGRX frame). */
void irlsafety_overlay_apply_region_bgra(const irlsafety_overlay_image *overlay, irlsafety_frame_view *frame,
					 const irlsafety_rect *region);

#ifdef __cplusplus
}
#endif