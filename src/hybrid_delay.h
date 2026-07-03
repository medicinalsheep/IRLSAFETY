/*
 * IRLSAFETY+ — hybrid stream delay + overlay timeline buffer.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "filter_settings.h"
#include "irlsafety_types.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IRLSAFETY_OVERLAY_HISTORY_CAP 150
#define IRLSAFETY_HYBRID_DELAY_FPS_ESTIMATE 30.0

typedef struct irlsafety_overlay_history {
	irlsafety_region_list frames[IRLSAFETY_OVERLAY_HISTORY_CAP];
	uint64_t frame_indices[IRLSAFETY_OVERLAY_HISTORY_CAP];
	size_t head;
	size_t count;
} irlsafety_overlay_history;

typedef struct irlsafety_hybrid_delay_runtime {
	irlsafety_overlay_history history;
	double effective_delay_sec;
	uint64_t last_protection_frame;
	uint64_t last_history_frame;
	bool protection_latched;
} irlsafety_hybrid_delay_runtime;

void irlsafety_hybrid_delay_runtime_init(irlsafety_hybrid_delay_runtime *runtime);
void irlsafety_hybrid_delay_runtime_reset(irlsafety_hybrid_delay_runtime *runtime);

typedef void (*irlsafety_stream_delay_fn)(double total_sec, void *userdata);

void irlsafety_hybrid_delay_set_stream_hook(irlsafety_stream_delay_fn fn, void *userdata);
void irlsafety_hybrid_delay_on_stream_started(void);
void irlsafety_hybrid_delay_on_stream_stopped(void);

void irlsafety_hybrid_delay_update_global(const irlsafety_filter_settings *settings, bool any_overlays,
					  uint64_t frame_index);

double irlsafety_hybrid_delay_target_sec(const irlsafety_filter_settings *settings, bool any_overlays,
					 uint64_t frame_index);

void irlsafety_hybrid_delay_tick(irlsafety_hybrid_delay_runtime *runtime,
				 const irlsafety_filter_settings *settings, uint64_t frame_index,
				 int frames_elapsed, bool any_overlays);

void irlsafety_hybrid_delay_record(irlsafety_hybrid_delay_runtime *runtime, uint64_t frame_index,
				   const irlsafety_region_list *regions, bool force);

bool irlsafety_hybrid_delay_lookup(const irlsafety_hybrid_delay_runtime *runtime, uint64_t frame_index,
				   double effective_delay_sec, irlsafety_region_list *out_regions);

bool irlsafety_hybrid_delay_is_protecting(void);

#ifdef __cplusplus
}
#endif