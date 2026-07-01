/*
 * IRLSAFETY+ — YOLO/ONNX object detection stub.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "../irlsafety_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct yolo_onnx_context yolo_onnx_context;

yolo_onnx_context *yolo_onnx_create(const char *model_path);
void yolo_onnx_destroy(yolo_onnx_context *ctx);

/* Planned: run ONNX Runtime inference on a normalized frame tensor. */
int detect_regions(yolo_onnx_context *ctx, const irlsafety_frame_view *frame,
		   irlsafety_region_list *out_regions);

#ifdef __cplusplus
}
#endif