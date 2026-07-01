/*
 * IRLSAFETY+ — OBS frame to pipeline frame conversion.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "frame_convert.h"

#include <string.h>

irlsafety_video_format irlsafety_format_from_obs(enum video_format format)
{
	switch (format) {
	case VIDEO_FORMAT_I420:
		return IRLSAFETY_FORMAT_I420;
	case VIDEO_FORMAT_NV12:
		return IRLSAFETY_FORMAT_NV12;
	case VIDEO_FORMAT_BGRA:
		return IRLSAFETY_FORMAT_BGRA;
	case VIDEO_FORMAT_BGRX:
		return IRLSAFETY_FORMAT_BGRX;
	case VIDEO_FORMAT_YUY2:
		return IRLSAFETY_FORMAT_YUY2;
	case VIDEO_FORMAT_UYVY:
		return IRLSAFETY_FORMAT_UYVY;
	default:
		return IRLSAFETY_FORMAT_UNKNOWN;
	}
}

int irlsafety_frame_view_from_obs(const struct obs_source_frame *frame, irlsafety_frame_view *view)
{
	if (!frame || !view)
		return -1;

	memset(view, 0, sizeof(*view));

	view->width = frame->width;
	view->height = frame->height;
	view->format = irlsafety_format_from_obs(frame->format);
	view->plane_count = irlsafety_plane_count_for_format(view->format);

	for (uint8_t i = 0; i < view->plane_count; i++) {
		view->planes[i] = frame->data[i];
		view->linesize[i] = frame->linesize[i];
	}

	return 0;
}