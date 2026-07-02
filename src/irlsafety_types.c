/*
 * IRLSAFETY+ — shared pipeline types.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "irlsafety_types.h"

uint8_t irlsafety_plane_count_for_format(irlsafety_video_format format)
{
	switch (format) {
	case IRLSAFETY_FORMAT_I420:
		return 3;
	case IRLSAFETY_FORMAT_NV12:
		return 2;
	case IRLSAFETY_FORMAT_BGRA:
	case IRLSAFETY_FORMAT_BGRX:
	case IRLSAFETY_FORMAT_RGBA:
	case IRLSAFETY_FORMAT_YUY2:
	case IRLSAFETY_FORMAT_UYVY:
		return 1;
	default:
		return 1;
	}
}