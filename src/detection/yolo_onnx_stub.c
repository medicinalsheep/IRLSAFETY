/*
 * IRLSAFETY+ — YOLO/ONNX stub (unit tests / builds without ONNX Runtime).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "yolo_onnx.h"

#include <stdlib.h>
#include <string.h>

struct yolo_onnx_context {
	char status[256];
};

yolo_onnx_context *yolo_onnx_create(void)
{
	return calloc(1, sizeof(yolo_onnx_context));
}

void yolo_onnx_destroy(yolo_onnx_context *ctx)
{
	free(ctx);
}

int yolo_onnx_load_model(yolo_onnx_context *ctx, const char *model_path, bool prefer_gpu)
{
	(void)prefer_gpu;

	if (!ctx)
		return -1;

	if (!model_path || model_path[0] == '\0') {
		strncpy(ctx->status, "No ONNX model configured", sizeof(ctx->status) - 1);
		return 0;
	}

	strncpy(ctx->status, "ONNX detection not compiled in this build", sizeof(ctx->status) - 1);
	return -1;
}

bool yolo_onnx_is_ready(const yolo_onnx_context *ctx)
{
	(void)ctx;
	return false;
}

const char *yolo_onnx_status_message(const yolo_onnx_context *ctx)
{
	if (!ctx)
		return "Detector unavailable";
	return ctx->status[0] ? ctx->status : "ONNX detection not available";
}

int detect_regions(yolo_onnx_context *ctx, const irlsafety_frame_view *frame,
		   const irlsafety_detection_config *config, irlsafety_region_list *out_regions)
{
	(void)ctx;
	(void)frame;
	(void)config;

	if (!out_regions)
		return -1;

	out_regions->count = 0;
	return 0;
}