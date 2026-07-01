/*
 * IRLSAFETY+ — OBS frame to pipeline frame conversion.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "irlsafety_types.h"

#ifdef IRLSAFETY_TEST_BUILD
#include "obs-mock.h"
#else
#include <obs.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

irlsafety_video_format irlsafety_format_from_obs(enum video_format format);
int irlsafety_frame_view_from_obs(const struct obs_source_frame *frame, irlsafety_frame_view *view);

#ifdef __cplusplus
}
#endif