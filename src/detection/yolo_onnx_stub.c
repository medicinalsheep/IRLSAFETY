/*
 * IRLSAFETY+ — YOLO/ONNX stub (unit tests / builds without ONNX Runtime).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "yolo_onnx.h"

#include <stdlib.h>
#include <string.h>

struct yolo_onnx_context {
	char status[256];
	char model_path[1024];
	bool prefer_gpu;
	bool loaded;
	int load_count;
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
	if (!ctx)
		return -1;

	/* Mirror production skip: same path + prefer_gpu → no-op. */
	if (ctx->loaded && model_path && model_path[0] != '\0' && ctx->model_path[0] != '\0' &&
	    strcmp(ctx->model_path, model_path) == 0 && ctx->prefer_gpu == prefer_gpu) {
		return 0;
	}

	ctx->prefer_gpu = prefer_gpu;
	ctx->load_count++;

	if (!model_path || model_path[0] == '\0') {
		ctx->model_path[0] = '\0';
		ctx->loaded = false;
		strncpy(ctx->status, "No ONNX model configured", sizeof(ctx->status) - 1);
		return 0;
	}

	/* Stub cannot run ORT; record path so skip-reload tests work. */
	strncpy(ctx->model_path, model_path, sizeof(ctx->model_path) - 1);
	ctx->model_path[sizeof(ctx->model_path) - 1] = '\0';
	ctx->loaded = true;
	strncpy(ctx->status, "ONNX detection stub (no Runtime in test build)", sizeof(ctx->status) - 1);
	return 0;
}

bool yolo_onnx_is_ready(const yolo_onnx_context *ctx)
{
	return ctx && ctx->loaded;
}

const char *yolo_onnx_status_message(const yolo_onnx_context *ctx)
{
	if (!ctx)
		return "Detector unavailable";
	return ctx->status[0] ? ctx->status : "ONNX detection not available";
}

float yolo_onnx_last_inference_ms(const yolo_onnx_context *ctx)
{
	(void)ctx;
	return 0.0f;
}

const char *yolo_onnx_active_ep(const yolo_onnx_context *ctx)
{
	(void)ctx;
	return "CPU";
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