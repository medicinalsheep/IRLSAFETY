/*
 * IRLSAFETY+ — YOLO letterbox preprocessing.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "../irlsafety_types.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IRLSAFETY_YOLO_INPUT_SIZE 640

typedef struct yolo_letterbox {
	float scale;
	float pad_x;
	float pad_y;
	uint32_t src_width;
	uint32_t src_height;
} yolo_letterbox;

/* Fill NCHW float tensor (RGB, 0..1) with letterboxed resize. */
int yolo_frame_to_tensor(const irlsafety_frame_view *frame, float *tensor_nchw, uint32_t tensor_w, uint32_t tensor_h,
			 yolo_letterbox *out_letterbox);

/* Map a network-space box back to source frame pixel coordinates. */
void yolo_unmap_box(float cx, float cy, float w, float h, const yolo_letterbox *letterbox, irlsafety_rect *out_rect);

#ifdef __cplusplus
}
#endif