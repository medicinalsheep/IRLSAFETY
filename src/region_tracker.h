/*
 * IRLSAFETY+ — persistent overlay region tracking across detection cycles.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "irlsafety_types.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IRLSAFETY_TRACKER_MISS_EVICT 5
#define IRLSAFETY_TRACKER_MISS_EVICT_MOVING 10
#define IRLSAFETY_TRACKER_IOU_MATCH 0.12f
#define IRLSAFETY_TRACKER_IOU_MATCH_FAST 0.04f
#define IRLSAFETY_TRACKER_VELOCITY_BLEND 0.62f
#define IRLSAFETY_TRACKER_MIN_SPEED 0.35f

typedef struct irlsafety_region_tracker {
	irlsafety_rect regions[IRLSAFETY_MAX_REGIONS];
	float velocity_x[IRLSAFETY_MAX_REGIONS];
	float velocity_y[IRLSAFETY_MAX_REGIONS];
	int miss_cycles[IRLSAFETY_MAX_REGIONS];
	size_t count;
	uint32_t frame_width;
	uint32_t frame_height;
} irlsafety_region_tracker;

void irlsafety_region_tracker_clear(irlsafety_region_tracker *tracker);

void irlsafety_region_tracker_scale(irlsafety_region_tracker *tracker, uint32_t new_width, uint32_t new_height);

/*
 * Merge fresh detections; unmatched tracks age by one cycle and evict after miss threshold.
 * frames_since_update: video frames since the last detection merge (for velocity smoothing).
 */
void irlsafety_region_tracker_update(irlsafety_region_tracker *tracker, const irlsafety_region_list *detected,
				     uint32_t frame_width, uint32_t frame_height, int frames_since_update);

/*
 * Advance tracked regions by velocity without evicting — used during secure-mode hold
 * when OCR misses during motion but overlays must stay until re-secured.
 */
void irlsafety_region_tracker_hold_predict(irlsafety_region_tracker *tracker, int frames_since_update);

void irlsafety_region_tracker_copy_regions(const irlsafety_region_tracker *tracker, irlsafety_region_list *out);

#ifdef __cplusplus
}
#endif