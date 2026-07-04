/*
 * IRLSAFETY+ — detection → OCR → blur orchestration.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "censor_log.h"
#include "custom_pii.h"
#include "filter_settings.h"
#include "irlsafety_types.h"

struct irlsafety_runtime_status;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct irlsafety_pipeline irlsafety_pipeline;

irlsafety_pipeline *irlsafety_pipeline_create(void);
void irlsafety_pipeline_shutdown(irlsafety_pipeline *pipeline);
void irlsafety_pipeline_destroy(irlsafety_pipeline *pipeline);

/* Reload custom PII keywords from current filter settings. */
int irlsafety_pipeline_update_settings(irlsafety_pipeline *pipeline, const irlsafety_filter_settings *settings);

/*
 * Hot-swap runtime toggles (categories, confidence, frame skip) without clearing
 * the overlay tracker. Reloads the detector only when model path or prefer_gpu changes.
 */
int irlsafety_pipeline_apply_runtime_settings(irlsafety_pipeline *pipeline,
					      const irlsafety_filter_settings *settings);

/* Load or reload the YOLO ONNX detector model. */
int irlsafety_pipeline_set_detector_model(irlsafety_pipeline *pipeline, const char *model_path, bool prefer_gpu);

/* Snapshot runtime status for the control dock. */
void irlsafety_pipeline_get_runtime_status(const irlsafety_pipeline *pipeline, struct irlsafety_runtime_status *out,
					   uint64_t frame_count);

/* Poll for a completed OCR job and update the overlay tracker (non-blocking). */
void irlsafety_pipeline_poll_detection(irlsafety_pipeline *pipeline, const irlsafety_filter_settings *settings,
				       uint32_t output_width, uint32_t output_height);

/* Queue OCR / detection on a CPU frame when the worker is idle. */
int irlsafety_pipeline_submit_detection(irlsafety_pipeline *pipeline, irlsafety_frame_view *frame,
					  const irlsafety_filter_settings *settings, uint64_t frame_index,
					  uint32_t output_width, uint32_t output_height);

bool irlsafety_pipeline_ocr_busy(const irlsafety_pipeline *pipeline);

/* True when OCR would run (screen text, sensitive patterns, or custom PII keywords). */
bool irlsafety_pipeline_needs_ocr(const irlsafety_pipeline *pipeline, const irlsafety_filter_settings *settings);

/*
 * Convenience: poll then optionally submit (non-blocking).
 * output_width/height are the full display size; frame may be a downscaled OCR buffer.
 */
int irlsafety_pipeline_detect_frame(irlsafety_pipeline *pipeline, irlsafety_frame_view *frame,
				    const irlsafety_filter_settings *settings, uint64_t frame_index,
				    uint32_t output_width, uint32_t output_height, bool heavy_source);

/* Return tracked overlay regions (+ optional test box) for GPU/CPU compositing. */
void irlsafety_pipeline_get_overlays(irlsafety_pipeline *pipeline, uint32_t frame_width, uint32_t frame_height,
				     const irlsafety_filter_settings *settings, irlsafety_region_list *out_regions);

/* Apply CPU censor (blur or ellipse) directly into a frame buffer. */
int irlsafety_pipeline_apply_cpu_censor(irlsafety_pipeline *pipeline, irlsafety_frame_view *frame,
					const irlsafety_filter_settings *settings, uint32_t frame_width,
					uint32_t frame_height);

/* Advance motion prediction every video frame (call from filter video_tick). */
void irlsafety_pipeline_tick(irlsafety_pipeline *pipeline, uint64_t frame_index,
			       const irlsafety_filter_settings *settings);

/* True while escalated secure mode needs OCR every frame (bypass frame skip). */
bool irlsafety_pipeline_needs_urgent_scan(const irlsafety_pipeline *pipeline);

/*
 * True when secure mode should drop/hide the current frame (OCR in flight or overlays lost mid-motion).
 * GPU path draws a full-screen censor; CPU async path may return NULL from filter_video.
 */
bool irlsafety_pipeline_should_drop_frame(const irlsafety_pipeline *pipeline,
					  const irlsafety_filter_settings *settings);

size_t irlsafety_pipeline_copy_censor_log(const irlsafety_pipeline *pipeline, irlsafety_censor_log_entry *out,
					  size_t max_entries);
void irlsafety_pipeline_clear_censor_log(irlsafety_pipeline *pipeline);

#ifdef __cplusplus
}
#endif