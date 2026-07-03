/*
 * IRLSAFETY+ — hybrid stream delay overlay timeline (portable core).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "hybrid_delay.h"

#include <math.h>
#include <string.h>

#define IRLSAFETY_DELAY_RAMP_SEC 0.35

static struct {
	bool streaming;
	bool global_protecting;
	uint64_t global_last_overlay_frame;
	double configured_global_sec;
	double configured_auto_sec;
	double configured_hold_sec;
	bool hybrid_enabled;
} g_hybrid = {0};

static irlsafety_stream_delay_fn g_stream_hook;
static void *g_stream_hook_userdata;

static void copy_region_list(irlsafety_region_list *dest, const irlsafety_region_list *src)
{
	if (!dest || !src)
		return;

	*dest = *src;
}

static void hybrid_apply_stream_delay(double total_sec)
{
	if (g_stream_hook)
		g_stream_hook(total_sec, g_stream_hook_userdata);
}

void irlsafety_hybrid_delay_set_stream_hook(irlsafety_stream_delay_fn fn, void *userdata)
{
	g_stream_hook = fn;
	g_stream_hook_userdata = userdata;
}

void irlsafety_hybrid_delay_runtime_init(irlsafety_hybrid_delay_runtime *runtime)
{
	if (!runtime)
		return;

	memset(runtime, 0, sizeof(*runtime));
}

void irlsafety_hybrid_delay_runtime_reset(irlsafety_hybrid_delay_runtime *runtime)
{
	if (!runtime)
		return;

	memset(&runtime->history, 0, sizeof(runtime->history));
	runtime->effective_delay_sec = 0.0;
	runtime->last_protection_frame = 0;
	runtime->last_history_frame = 0;
	runtime->protection_latched = false;
}

void irlsafety_hybrid_delay_on_stream_started(void)
{
	g_hybrid.streaming = true;

	if (g_hybrid.hybrid_enabled)
		hybrid_apply_stream_delay(g_hybrid.configured_global_sec);
}

void irlsafety_hybrid_delay_on_stream_stopped(void)
{
	g_hybrid.streaming = false;
	g_hybrid.global_protecting = false;
}

static void hybrid_refresh_config(const irlsafety_filter_settings *settings)
{
	if (!settings)
		return;

	g_hybrid.hybrid_enabled = settings->hybrid_delay_enable;
	g_hybrid.configured_global_sec = settings->global_delay_sec;
	g_hybrid.configured_auto_sec = settings->auto_delay_sec;
	g_hybrid.configured_hold_sec = settings->auto_delay_hold_sec;
}

void irlsafety_hybrid_delay_update_global(const irlsafety_filter_settings *settings, bool any_overlays,
					  uint64_t frame_index)
{
	double target_delay;
	bool want_protect;

	hybrid_refresh_config(settings);
	if (!settings || !settings->hybrid_delay_enable)
		return;

	if (any_overlays) {
		g_hybrid.global_last_overlay_frame = frame_index;
		g_hybrid.global_protecting = true;
	}

	want_protect = g_hybrid.global_protecting;
	if (want_protect && settings->auto_delay_hold_sec > 0.0f && g_hybrid.global_last_overlay_frame > 0 &&
	    frame_index > g_hybrid.global_last_overlay_frame) {
		double hold_frames = settings->auto_delay_hold_sec * IRLSAFETY_HYBRID_DELAY_FPS_ESTIMATE;
		if ((double)(frame_index - g_hybrid.global_last_overlay_frame) > hold_frames)
			g_hybrid.global_protecting = false;
	}

	target_delay = settings->global_delay_sec;
	if (g_hybrid.global_protecting)
		target_delay += settings->auto_delay_sec;

	if (g_hybrid.streaming)
		hybrid_apply_stream_delay(target_delay);
}

double irlsafety_hybrid_delay_target_sec(const irlsafety_filter_settings *settings, bool any_overlays,
					 uint64_t frame_index)
{
	if (!settings || !settings->hybrid_delay_enable)
		return 0.0;

	if (any_overlays)
		return settings->global_delay_sec + settings->auto_delay_sec;

	if (g_hybrid.global_last_overlay_frame > 0 && frame_index >= g_hybrid.global_last_overlay_frame) {
		double hold_frames = settings->auto_delay_hold_sec * IRLSAFETY_HYBRID_DELAY_FPS_ESTIMATE;
		if ((double)(frame_index - g_hybrid.global_last_overlay_frame) <= hold_frames)
			return settings->global_delay_sec + settings->auto_delay_sec;
	}

	return settings->global_delay_sec;
}

void irlsafety_hybrid_delay_tick(irlsafety_hybrid_delay_runtime *runtime, const irlsafety_filter_settings *settings,
				 uint64_t frame_index, int frames_elapsed, bool any_overlays)
{
	double target;
	double step;
	int steps;

	if (!runtime || !settings || frames_elapsed < 1)
		return;

	if (!settings->hybrid_delay_enable) {
		runtime->effective_delay_sec = 0.0;
		return;
	}

	if (any_overlays)
		runtime->last_protection_frame = frame_index;

	target = irlsafety_hybrid_delay_target_sec(settings, any_overlays, frame_index);
	runtime->protection_latched = target > settings->global_delay_sec + 0.001;

	step = (settings->auto_delay_sec > 0.0 ? settings->auto_delay_sec : 0.5) /
	       (IRLSAFETY_DELAY_RAMP_SEC * IRLSAFETY_HYBRID_DELAY_FPS_ESTIMATE);
	steps = frames_elapsed;

	while (steps-- > 0) {
		if (runtime->effective_delay_sec < target)
			runtime->effective_delay_sec += step;
		else if (runtime->effective_delay_sec > target)
			runtime->effective_delay_sec -= step;

		if (fabs(runtime->effective_delay_sec - target) < step)
			runtime->effective_delay_sec = target;
	}

	irlsafety_hybrid_delay_update_global(settings, any_overlays, frame_index);
}

void irlsafety_hybrid_delay_record(irlsafety_hybrid_delay_runtime *runtime, uint64_t frame_index,
				   const irlsafety_region_list *regions, bool force)
{
	size_t slot;

	if (!runtime || !regions)
		return;

	if (!force && runtime->last_history_frame > 0 && frame_index <= runtime->last_history_frame + 3 &&
	    regions->count == 0)
		return;

	slot = runtime->history.head % IRLSAFETY_OVERLAY_HISTORY_CAP;
	copy_region_list(&runtime->history.frames[slot], regions);
	runtime->history.frame_indices[slot] = frame_index;
	runtime->history.head = (runtime->history.head + 1) % IRLSAFETY_OVERLAY_HISTORY_CAP;
	if (runtime->history.count < IRLSAFETY_OVERLAY_HISTORY_CAP)
		runtime->history.count++;
	runtime->last_history_frame = frame_index;
}

bool irlsafety_hybrid_delay_lookup(const irlsafety_hybrid_delay_runtime *runtime, uint64_t frame_index,
				   double effective_delay_sec, irlsafety_region_list *out_regions)
{
	uint64_t lookup_frame;
	size_t best_slot = 0;
	uint64_t best_delta = UINT64_MAX;
	size_t i;

	if (!runtime || !out_regions)
		return false;

	out_regions->count = 0;
	if (effective_delay_sec <= 0.0 || runtime->history.count == 0)
		return false;

	lookup_frame = frame_index;
	{
		uint64_t delay_frames = (uint64_t)(effective_delay_sec * IRLSAFETY_HYBRID_DELAY_FPS_ESTIMATE);
		if (lookup_frame > delay_frames)
			lookup_frame -= delay_frames;
	}

	for (i = 0; i < runtime->history.count; i++) {
		size_t slot = (runtime->history.head + IRLSAFETY_OVERLAY_HISTORY_CAP - 1 - i) %
			      IRLSAFETY_OVERLAY_HISTORY_CAP;
		uint64_t stored = runtime->history.frame_indices[slot];
		uint64_t delta = stored > lookup_frame ? stored - lookup_frame : lookup_frame - stored;

		if (delta < best_delta) {
			best_delta = delta;
			best_slot = slot;
		}
	}

	copy_region_list(out_regions, &runtime->history.frames[best_slot]);
	return out_regions->count > 0;
}

bool irlsafety_hybrid_delay_is_protecting(void)
{
	return g_hybrid.global_protecting;
}