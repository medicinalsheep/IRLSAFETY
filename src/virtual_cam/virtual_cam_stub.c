/*
 * IRLSAFETY+ — virtual camera core (host hooks via OBS adapter on Windows).
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

static irlsafety_virtual_cam_start_fn g_start_hook;
static irlsafety_virtual_cam_stop_fn g_stop_hook;
static irlsafety_virtual_cam_active_fn g_active_hook;
static void *g_hook_userdata;

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

void irlsafety_virtual_cam_set_hooks(irlsafety_virtual_cam_start_fn start, irlsafety_virtual_cam_stop_fn stop,
				     irlsafety_virtual_cam_active_fn active, void *userdata)
{
	g_start_hook = start;
	g_stop_hook = stop;
	g_active_hook = active;
	g_hook_userdata = userdata;

	if (start && stop) {
		snprintf(g_status, sizeof(g_status),
			 "Protected Virtual Camera ready — start from tray panel or OBS Tools");
	} else {
		irlsafety_virtual_cam_refresh_status();
	}
}

void irlsafety_virtual_cam_clear_hooks(void)
{
	g_start_hook = NULL;
	g_stop_hook = NULL;
	g_active_hook = NULL;
	g_hook_userdata = NULL;
	g_state = IRLSAFETY_VCAM_STOPPED;
	irlsafety_virtual_cam_refresh_status();
}

bool irlsafety_virtual_cam_supported(void)
{
	return g_start_hook != NULL && g_stop_hook != NULL;
}

irlsafety_virtual_cam_state irlsafety_virtual_cam_get_state(void)
{
	if (g_active_hook && g_active_hook(g_hook_userdata))
		return IRLSAFETY_VCAM_RUNNING;
	if (g_state == IRLSAFETY_VCAM_STARTING)
		return IRLSAFETY_VCAM_STARTING;
	return g_state;
}

const char *irlsafety_virtual_cam_status_message(void)
{
	return g_status;
}

int irlsafety_virtual_cam_start(const irlsafety_virtual_cam_config *config)
{
	if (!g_start_hook)
		return -1;

	g_state = IRLSAFETY_VCAM_STARTING;
	if (g_start_hook(config, g_hook_userdata) != 0) {
		g_state = IRLSAFETY_VCAM_ERROR;
		snprintf(g_status, sizeof(g_status), "Failed to start protected virtual camera");
		return -1;
	}

	g_state = IRLSAFETY_VCAM_RUNNING;
	snprintf(g_status, sizeof(g_status), "Protected Virtual Camera active — pick it in Discord, Zoom, or browser");
	return 0;
}

void irlsafety_virtual_cam_stop(void)
{
	if (g_stop_hook)
		g_stop_hook(g_hook_userdata);
	g_state = IRLSAFETY_VCAM_STOPPED;
	if (irlsafety_virtual_cam_supported())
		snprintf(g_status, sizeof(g_status), "Protected Virtual Camera stopped");
	else
		irlsafety_virtual_cam_refresh_status();
}

int irlsafety_virtual_cam_submit_frame(const irlsafety_frame_view *frame)
{
	(void)frame;
	/* OBS virtual cam taps the program output after filters — no manual submit yet. */
	return irlsafety_virtual_cam_get_state() == IRLSAFETY_VCAM_RUNNING ? 0 : -1;
}

void irlsafety_virtual_cam_refresh_status(void)
{
	if (g_start_hook && g_stop_hook) {
		snprintf(g_status, sizeof(g_status),
			 "Protected Virtual Camera ready — start from tray panel (apply filter to sources first)");
		return;
	}

	if (obs_virtual_cam_module_present()) {
		snprintf(g_status, sizeof(g_status),
			 "OBS Virtual Camera driver found — IRLSAFETY tray panel can start protected output");
	} else {
		snprintf(g_status, sizeof(g_status),
			 "Install OBS Virtual Camera (OBS Tools menu) for protected output to other apps");
	}
}