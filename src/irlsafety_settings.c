/*
 * IRLSAFETY+ — portable filter settings helpers.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "irlsafety_settings.h"

#include "hybrid_delay.h"

#include <string.h>

void irlsafety_settings_apply_defaults(irlsafety_filter_settings *out)
{
	if (!out)
		return;

	memset(out, 0, sizeof(*out));
	out->enable_all = true;
	out->cat_license_plates = true;
	out->cat_street_signs = true;
	out->cat_shipping_labels = true;
	out->cat_id_documents = true;
	out->cat_screen_text = false;
	out->cat_custom_pii = true;
	out->cat_sensitive_patterns = false;
	out->confidence_threshold = 0.35f;
	out->frame_skip = 8;
	out->ocr_detail = 0;
	out->blur_strength = 24.0f;
	out->censor_mode = IRLSAFETY_CENSOR_BOX;
	out->censor_color = 0xFF000000;
	out->angled_cover = true;
	out->hybrid_delay_enable = true;
	out->global_delay_sec = 1.5f;
	out->auto_delay_sec = 1.0f;
	out->auto_delay_hold_sec = 4.0f;
	out->partial_pii_threshold = 0.50f;
	out->secure_mode_enable = true;
	out->secure_drop_frames = true;
	out->cover_while_typing = false;
	out->test_effect = false;
	out->prefer_gpu = true;
	out->use_child_ocr = false;
}

float irlsafety_settings_effective_partial_threshold(const irlsafety_filter_settings *settings)
{
	if (!settings)
		return 0.50f;

	if (settings->cover_while_typing && settings->cat_custom_pii)
		return IRLSAFETY_TYPING_COVER_THRESHOLD;

	return settings->partial_pii_threshold;
}

bool irlsafety_settings_should_run_detection(const irlsafety_filter_settings *settings, uint64_t frame_index,
					     bool heavy_source)
{
	int interval;

	if (!settings || !settings->enable_all)
		return false;

	interval = settings->frame_skip;
	if (interval < 1)
		interval = 1;

	if (heavy_source) {
		interval *= 3;
		if (interval < 6)
			interval = 6;
	}

	if (!heavy_source && settings->cover_while_typing && settings->cat_custom_pii)
		return true;

	if (!heavy_source && frame_index <= 2)
		return true;

	if (settings->hybrid_delay_enable && !irlsafety_hybrid_delay_is_protecting() && interval < 30)
		interval *= 2;

	return (frame_index % (uint64_t)interval) == 0;
}

bool irlsafety_settings_detection_enabled(const irlsafety_filter_settings *settings)
{
	if (!settings || !settings->enable_all)
		return false;

	return settings->cat_license_plates || settings->cat_street_signs || settings->cat_shipping_labels ||
	       settings->cat_id_documents;
}