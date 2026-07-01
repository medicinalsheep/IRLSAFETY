/*
 * IRLSAFETY+ — shared pipeline types (no OBS dependency).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IRLSAFETY_MAX_REGIONS 64

typedef struct irlsafety_rect {
	float x;
	float y;
	float width;
	float height;
	float confidence;
} irlsafety_rect;

typedef struct irlsafety_region_list {
	irlsafety_rect regions[IRLSAFETY_MAX_REGIONS];
	size_t count;
} irlsafety_region_list;

typedef struct irlsafety_frame_view {
	const uint8_t *data;
	uint32_t width;
	uint32_t height;
	uint32_t linesize;
} irlsafety_frame_view;

typedef struct irlsafety_mask {
	uint8_t *data;
	uint32_t width;
	uint32_t height;
	uint32_t linesize;
} irlsafety_mask;

#ifdef __cplusplus
}
#endif