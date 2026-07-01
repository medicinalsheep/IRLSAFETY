/*
 * IRLSAFETY+ — detection → OCR → blur orchestration.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "custom_pii.h"
#include "filter_settings.h"
#include "irlsafety_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct irlsafety_pipeline irlsafety_pipeline;

irlsafety_pipeline *irlsafety_pipeline_create(const char *yolo_model_path);
void irlsafety_pipeline_destroy(irlsafety_pipeline *pipeline);

/* Reload custom PII keywords from current filter settings. */
int irlsafety_pipeline_update_settings(irlsafety_pipeline *pipeline, const irlsafety_filter_settings *settings);

/* Run the full PII pipeline on a CPU frame view (respects category toggles). */
int irlsafety_pipeline_process_frame(irlsafety_pipeline *pipeline, irlsafety_frame_view *frame,
				     const irlsafety_filter_settings *settings);

#ifdef __cplusplus
}
#endif