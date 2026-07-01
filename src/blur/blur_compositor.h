/*
 * IRLSAFETY+ — blur compositing stub.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "../irlsafety_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blur_compositor_context blur_compositor_context;

blur_compositor_context *blur_compositor_create(void);
void blur_compositor_destroy(blur_compositor_context *ctx);

/* Planned: expand region masks and apply Gaussian/pixelate blur in-place. */
int apply_blur(blur_compositor_context *ctx, irlsafety_frame_view *frame,
	       const irlsafety_region_list *regions, float blur_strength);

#ifdef __cplusplus
}
#endif