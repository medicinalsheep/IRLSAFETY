/*
 * IRLSAFETY+ — YOLO/ONNX object detection.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "../irlsafety_types.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Expected class indices in IRLSAFETY+ detection ONNX models. */
#define IRLSAFETY_YOLO_CLASS_LICENSE_PLATE 0
#define IRLSAFETY_YOLO_CLASS_STREET_SIGN 1
#define IRLSAFETY_YOLO_CLASS_DOCUMENT 2
#define IRLSAFETY_YOLO_CLASS_FACE 3

typedef struct yolo_onnx_context yolo_onnx_context;

typedef struct irlsafety_detection_config {
	bool license_plates;
	bool street_signs;
	bool documents;
	bool faces;
	float confidence_threshold;
} irlsafety_detection_config;

yolo_onnx_context *yolo_onnx_create(void);
void yolo_onnx_destroy(yolo_onnx_context *ctx);

/* Load or reload an ONNX model. prefer_gpu uses DirectML on Windows when available. */
int yolo_onnx_load_model(yolo_onnx_context *ctx, const char *model_path, bool prefer_gpu);

bool yolo_onnx_is_ready(const yolo_onnx_context *ctx);
const char *yolo_onnx_status_message(const yolo_onnx_context *ctx);

int detect_regions(yolo_onnx_context *ctx, const irlsafety_frame_view *frame,
		   const irlsafety_detection_config *config, irlsafety_region_list *out_regions);

#ifdef __cplusplus
}
#endif