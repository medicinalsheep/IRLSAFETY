/*
 * IRLSAFETY+ — YOLO/ONNX object detection stub.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "yolo_onnx.h"

#include <stdlib.h>
#include <string.h>

struct yolo_onnx_context {
	char model_path[512];
};

yolo_onnx_context *yolo_onnx_create(const char *model_path)
{
	yolo_onnx_context *ctx = calloc(1, sizeof(*ctx));
	if (!ctx)
		return NULL;

	if (model_path)
		strncpy(ctx->model_path, model_path, sizeof(ctx->model_path) - 1);

	return ctx;
}

void yolo_onnx_destroy(yolo_onnx_context *ctx)
{
	free(ctx);
}

int detect_regions(yolo_onnx_context *ctx, const irlsafety_frame_view *frame,
		   irlsafety_region_list *out_regions)
{
	(void)ctx;
	(void)frame;

	if (!out_regions)
		return -1;

	out_regions->count = 0;
	return 0;
}