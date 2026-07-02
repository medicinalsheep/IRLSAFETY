/*
 * IRLSAFETY+ — filter property tooltips (OBS long descriptions).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

struct obs_properties;

#ifdef __cplusplus
extern "C" {
#endif

void irlsafety_filter_apply_property_tooltips(struct obs_properties *props, struct obs_properties *categories,
					      struct obs_properties *custom, struct obs_properties *protection,
					      struct obs_properties *advanced);

#ifdef __cplusplus
}
#endif