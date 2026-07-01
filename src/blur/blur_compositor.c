/*
 * IRLSAFETY+ — blur compositing stub.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "blur_compositor.h"

#include <stdlib.h>

struct blur_compositor_context {
	float default_strength;
};

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

int apply_blur(blur_compositor_context *ctx, irlsafety_frame_view *frame,
	       const irlsafety_region_list *regions, float blur_strength)
{
	(void)ctx;
	(void)frame;
	(void)regions;
	(void)blur_strength;

	/* Stub: no-op until GPU/CPU blur kernel is implemented. */
	return 0;
}