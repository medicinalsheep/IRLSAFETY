/*
 * IRLSAFETY+ — censor compositing (blur, solid box, solid ellipse).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "../filter_settings.h"
#include "../irlsafety_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blur_compositor_context blur_compositor_context;

typedef struct irlsafety_overlay_image irlsafety_overlay_image;

typedef struct irlsafety_censor_options {
	irlsafety_censor_mode mode;
	float blur_strength;
	uint32_t color;
	const irlsafety_overlay_image *overlay;
	bool show_preview;
} irlsafety_censor_options;

blur_compositor_context *blur_compositor_create(void);
void blur_compositor_destroy(blur_compositor_context *ctx);

int apply_censor(blur_compositor_context *ctx, irlsafety_frame_view *frame, const irlsafety_region_list *regions,
		 const irlsafety_censor_options *options);

/* Back-compat wrapper used by older tests. */
int apply_blur(blur_compositor_context *ctx, irlsafety_frame_view *frame, const irlsafety_region_list *regions,
	       float blur_strength);

#ifdef __cplusplus
}
#endif