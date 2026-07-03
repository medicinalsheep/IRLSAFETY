/*
 * IRLSAFETY+ — OBS platform hooks for libirlsafety (plugin adapter).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "irlsafety_obs_adapter.h"

#include "hybrid_delay.h"
#include "irlsafety_log.h"
#include "irlsafety_paths.h"
#include "irlsafety_shutdown.h"
#include "virtual_cam/virtual_cam.h"

#include <math.h>
#include <obs-module.h>
#include <plugin-support.h>
#include <util/config-file.h>

#ifdef IRLSAFETY_HAS_FRONTEND_API
#include <obs-frontend-api.h>
#endif

static void obs_log_bridge(int level, const char *message, void *userdata)
{
	(void)userdata;
	obs_log(level, "%s", message);
}

static char *obs_bundled_path_resolver(const char *relative, void *userdata)
{
	(void)userdata;
	return obs_module_file(relative);
}

#ifdef IRLSAFETY_HAS_FRONTEND_API
static struct {
	bool user_delay_saved;
	bool saved_delay_enable;
	uint32_t saved_delay_sec;
} g_obs_hybrid;

static uint32_t delay_seconds_from_double(double seconds)
{
	if (seconds < 0.0)
		seconds = 0.0;
	return (uint32_t)ceil(seconds);
}

static void obs_stream_delay_hook(double total_sec, void *userdata)
{
	obs_output_t *output;
	config_t *profile;

	(void)userdata;

	if (irlsafety_is_shutting_down())
		return;

	output = obs_frontend_get_streaming_output();
	profile = obs_frontend_get_profile_config();

	if (!output)
		return;

	if (profile) {
		config_set_bool(profile, "Output", "DelayEnable", total_sec > 0.0);
		config_set_int(profile, "Output", "DelaySec", (int)delay_seconds_from_double(total_sec));
	}

	obs_output_set_delay(output, delay_seconds_from_double(total_sec), OBS_OUTPUT_DELAY_PRESERVE);
}

static int obs_virtual_cam_start_hook(const irlsafety_virtual_cam_config *config, void *userdata)
{
	(void)config;
	(void)userdata;

	if (irlsafety_is_shutting_down())
		return -1;

	if (obs_frontend_virtualcam_active())
		return 0;

	obs_frontend_start_virtualcam();
	return obs_frontend_virtualcam_active() ? 0 : -1;
}

static void obs_virtual_cam_stop_hook(void *userdata)
{
	(void)userdata;

	if (irlsafety_is_shutting_down())
		return;

	if (obs_frontend_virtualcam_active())
		obs_frontend_stop_virtualcam();
}

static bool obs_virtual_cam_active_hook(void *userdata)
{
	(void)userdata;
	return obs_frontend_virtualcam_active();
}
#endif

void irlsafety_obs_adapter_register(void)
{
	irlsafety_log_set_callback(obs_log_bridge, NULL);
	irlsafety_paths_set_resolver(obs_bundled_path_resolver, NULL);

#ifdef IRLSAFETY_HAS_FRONTEND_API
	irlsafety_hybrid_delay_set_stream_hook(obs_stream_delay_hook, NULL);
	irlsafety_virtual_cam_set_hooks(obs_virtual_cam_start_hook, obs_virtual_cam_stop_hook,
					obs_virtual_cam_active_hook, NULL);
#endif
}

void irlsafety_obs_adapter_unregister(void)
{
	irlsafety_log_set_callback(NULL, NULL);
	irlsafety_paths_set_resolver(NULL, NULL);
	irlsafety_hybrid_delay_set_stream_hook(NULL, NULL);
	irlsafety_virtual_cam_clear_hooks();
}

void irlsafety_obs_on_stream_started(void)
{
#ifdef IRLSAFETY_HAS_FRONTEND_API
	config_t *profile = obs_frontend_get_profile_config();

	if (!g_obs_hybrid.user_delay_saved && profile) {
		g_obs_hybrid.saved_delay_enable = config_get_bool(profile, "Output", "DelayEnable");
		g_obs_hybrid.saved_delay_sec = (uint32_t)config_get_int(profile, "Output", "DelaySec");
		g_obs_hybrid.user_delay_saved = true;
	}
#endif

	irlsafety_hybrid_delay_on_stream_started();
}

void irlsafety_obs_on_stream_stopped(void)
{
#ifdef IRLSAFETY_HAS_FRONTEND_API
	if (!irlsafety_is_shutting_down() || g_obs_hybrid.user_delay_saved) {
		config_t *profile = obs_frontend_get_profile_config();
		obs_output_t *output = obs_frontend_get_streaming_output();

		if (g_obs_hybrid.user_delay_saved && profile) {
			config_set_bool(profile, "Output", "DelayEnable", g_obs_hybrid.saved_delay_enable);
			config_set_int(profile, "Output", "DelaySec", (int)g_obs_hybrid.saved_delay_sec);
			if (output)
				obs_output_set_delay(output, g_obs_hybrid.saved_delay_enable ? g_obs_hybrid.saved_delay_sec : 0,
						     OBS_OUTPUT_DELAY_PRESERVE);
		}
	}
	g_obs_hybrid.user_delay_saved = false;
#endif

	irlsafety_hybrid_delay_on_stream_stopped();
}