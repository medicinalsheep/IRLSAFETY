/*
 * IRLSAFETY+ — detection → OCR → blur orchestration.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "irlsafety_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct irlsafety_pipeline irlsafety_pipeline;

irlsafety_pipeline *irlsafety_pipeline_create(const char *yolo_model_path);
void irlsafety_pipeline_destroy(irlsafety_pipeline *pipeline);

/* Run the full PII pipeline on a CPU frame view. */
int irlsafety_pipeline_process_frame(irlsafety_pipeline *pipeline, irlsafety_frame_view *frame,
				     float blur_strength);

#ifdef __cplusplus
}
#endif