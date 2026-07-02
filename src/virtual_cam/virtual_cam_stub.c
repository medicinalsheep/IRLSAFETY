/*
 * IRLSAFETY+ — virtual camera stub (not yet shipped).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "virtual_cam.h"

#include <stdio.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

static irlsafety_virtual_cam_state g_state = IRLSAFETY_VCAM_STOPPED;
static char g_status[256] = "Virtual camera not yet available — see models/VIRTUAL_CAMERA.txt";

static bool obs_virtual_cam_module_present(void)
{
#ifdef _WIN32
	static const char *candidates[] = {
		"C:\\Program Files\\obs-studio\\data\\obs-plugins\\win-dshow\\obs-virtualcam-module64.dll",
		"C:\\Program Files (x86)\\obs-studio\\data\\obs-plugins\\win-dshow\\obs-virtualcam-module64.dll",
		NULL,
	};

	for (int i = 0; candidates[i]; i++) {
		DWORD attrs = GetFileAttributesA(candidates[i]);
		if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY))
			return true;
	}
#endif
	return false;
}

bool irlsafety_virtual_cam_supported(void)
{
	return false;
}

irlsafety_virtual_cam_state irlsafety_virtual_cam_get_state(void)
{
	return g_state;
}

const char *irlsafety_virtual_cam_status_message(void)
{
	return g_status;
}

int irlsafety_virtual_cam_start(const irlsafety_virtual_cam_config *config)
{
	(void)config;
	g_state = IRLSAFETY_VCAM_ERROR;
	return -1;
}

void irlsafety_virtual_cam_stop(void)
{
	g_state = IRLSAFETY_VCAM_STOPPED;
}

int irlsafety_virtual_cam_submit_frame(const irlsafety_frame_view *frame)
{
	(void)frame;
	return -1;
}

/* Called once at plugin load to refine the dock / log status line. */
void irlsafety_virtual_cam_refresh_status(void)
{
	if (obs_virtual_cam_module_present()) {
		snprintf(g_status, sizeof(g_status),
			 "IRLSAFETY direct virtual cam coming in v0.8 — today use OBS Tools → Start Virtual Camera on censored output");
	} else {
		snprintf(g_status, sizeof(g_status),
			 "Virtual camera not yet available — install OBS Virtual Camera (Tools menu) or see models/VIRTUAL_CAMERA.txt");
	}
}