/*
 * IRLSAFETY+ — GPU texture capture, CPU pipeline, and redraw.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "filter_settings.h"
#include "irlsafety_types.h"
#include "pipeline.h"

#include <stdbool.h>
#include <stdint.h>

struct obs_source;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct irlsafety_gpu_frame irlsafety_gpu_frame;

irlsafety_gpu_frame *irlsafety_gpu_frame_create(struct obs_source *filter);
void irlsafety_gpu_frame_destroy(irlsafety_gpu_frame *gpu);

/* Capture the filter target once (call per frame before draw / OCR). */
bool irlsafety_gpu_frame_begin_frame(irlsafety_gpu_frame *gpu, uint32_t *out_width, uint32_t *out_height);

/*
 * Downscaled OCR readback from the last capture (does not re-capture).
 * output_width/height are the full display size used to scale OCR boxes.
 */
int irlsafety_gpu_frame_readback_ocr(irlsafety_gpu_frame *gpu, irlsafety_frame_view *out_view,
				     uint32_t output_width, uint32_t output_height, uint32_t ocr_max_width);

/* Draw the last captured texture to the current framebuffer. */
bool irlsafety_gpu_frame_draw_captured(irlsafety_gpu_frame *gpu);

/* Draw solid censorship boxes on top of the current framebuffer. */
void irlsafety_gpu_frame_draw_overlays(struct obs_source *filter, const irlsafety_region_list *regions,
				       const irlsafety_filter_settings *settings);

/* Opaque full-frame censor — used when secure mode drops frames on the GPU path. */
void irlsafety_gpu_frame_draw_fullscreen_censor(struct obs_source *filter, const irlsafety_filter_settings *settings,
						uint32_t width, uint32_t height);

/* Capture → CPU censor (blur/ellipse) → draw modified texture. */
int irlsafety_gpu_frame_render_cpu_censor(irlsafety_gpu_frame *gpu, irlsafety_pipeline *pipeline,
					  const irlsafety_filter_settings *settings, uint32_t frame_width,
					  uint32_t frame_height);

#ifdef __cplusplus
}
#endif