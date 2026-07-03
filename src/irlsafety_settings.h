/*
 * IRLSAFETY+ — portable filter settings (no OBS types).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "filter_settings.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void irlsafety_settings_apply_defaults(irlsafety_filter_settings *out);

float irlsafety_settings_effective_partial_threshold(const irlsafety_filter_settings *settings);

bool irlsafety_settings_should_run_detection(const irlsafety_filter_settings *settings, uint64_t frame_index,
					     bool heavy_source);

bool irlsafety_settings_detection_enabled(const irlsafety_filter_settings *settings);

#ifdef __cplusplus
}
#endif