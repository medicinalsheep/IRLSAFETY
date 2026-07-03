/*
 * IRLSAFETY+ — virtual camera output (planned adapter).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 *
 * Feeds censored frames to a local virtual webcam device so Discord, Zoom, browsers,
 * and any "camera" picker can receive protected video without OBS in the middle.
 * See data/models/VIRTUAL_CAMERA.txt for user-facing roadmap.
 */

#pragma once

#include "../irlsafety_types.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum irlsafety_virtual_cam_state {
	IRLSAFETY_VCAM_STOPPED = 0,
	IRLSAFETY_VCAM_STARTING,
	IRLSAFETY_VCAM_RUNNING,
	IRLSAFETY_VCAM_ERROR,
} irlsafety_virtual_cam_state;

typedef struct irlsafety_virtual_cam_config {
	uint32_t width;
	uint32_t height;
	uint32_t fps;
	bool prefer_nv12;
} irlsafety_virtual_cam_config;

bool irlsafety_virtual_cam_supported(void);
irlsafety_virtual_cam_state irlsafety_virtual_cam_get_state(void);
const char *irlsafety_virtual_cam_status_message(void);

int irlsafety_virtual_cam_start(const irlsafety_virtual_cam_config *config);
void irlsafety_virtual_cam_stop(void);

/* Submit one censored frame (any supported format — converted internally). */
int irlsafety_virtual_cam_submit_frame(const irlsafety_frame_view *frame);

/* Refine status message after probing the host environment (OBS virtual cam module, etc.). */
void irlsafety_virtual_cam_refresh_status(void);

typedef int (*irlsafety_virtual_cam_start_fn)(const irlsafety_virtual_cam_config *config, void *userdata);
typedef void (*irlsafety_virtual_cam_stop_fn)(void *userdata);
typedef bool (*irlsafety_virtual_cam_active_fn)(void *userdata);

void irlsafety_virtual_cam_set_hooks(irlsafety_virtual_cam_start_fn start, irlsafety_virtual_cam_stop_fn stop,
				     irlsafety_virtual_cam_active_fn active, void *userdata);
void irlsafety_virtual_cam_clear_hooks(void);

#ifdef __cplusplus
}
#endif