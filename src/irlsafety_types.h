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

#define IRLSAFETY_MAX_REGIONS 256
#define IRLSAFETY_MAX_PLANES 4

typedef enum irlsafety_video_format {
	IRLSAFETY_FORMAT_UNKNOWN = 0,
	IRLSAFETY_FORMAT_I420,
	IRLSAFETY_FORMAT_NV12,
	IRLSAFETY_FORMAT_BGRA,
	IRLSAFETY_FORMAT_BGRX,
	IRLSAFETY_FORMAT_RGBA,
	IRLSAFETY_FORMAT_YUY2,
	IRLSAFETY_FORMAT_UYVY,
} irlsafety_video_format;

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
	uint8_t *planes[IRLSAFETY_MAX_PLANES];
	uint32_t linesize[IRLSAFETY_MAX_PLANES];
	uint32_t width;
	uint32_t height;
	irlsafety_video_format format;
	uint8_t plane_count;
} irlsafety_frame_view;

typedef struct irlsafety_mask {
	uint8_t *data;
	uint32_t width;
	uint32_t height;
	uint32_t linesize;
} irlsafety_mask;

uint8_t irlsafety_plane_count_for_format(irlsafety_video_format format);

#ifdef __cplusplus
}
#endif