/*
 * IRLSAFETY+ — OBS filter settings (loaded from obs_data_t).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "filter_settings.h"
#include "hybrid_delay.h"

#ifdef IRLSAFETY_TEST_BUILD
#include "obs-mock.h"
#else
#include <obs-module.h>
#endif

#include <string.h>

void irlsafety_filter_settings_set_defaults(struct obs_data *settings)
{
	if (!settings)
		return;

	obs_data_set_default_bool(settings, IRLSAFETY_SET_ENABLE_ALL, true);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_CAT_STREET_SIGNS, false);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_CAT_LICENSE_PLATES, false);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_CAT_DOCUMENTS, false);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_CAT_FACES, false);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_CAT_SCREEN_TEXT, true);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_CAT_CUSTOM_PII, true);

	obs_data_set_default_double(settings, IRLSAFETY_SET_CONFIDENCE, 0.45);
	obs_data_set_default_int(settings, IRLSAFETY_SET_FRAME_SKIP, 2);
	obs_data_set_default_int(settings, IRLSAFETY_SET_OCR_DETAIL, 1);
	obs_data_set_default_double(settings, IRLSAFETY_SET_BLUR_STRENGTH, 12.0);
	obs_data_set_default_int(settings, IRLSAFETY_SET_CENSOR_MODE, IRLSAFETY_CENSOR_BOX);
	/* Opaque dark gray (AARRGGBB) — visible on most sources; pick black in UI if preferred. */
	obs_data_set_default_int(settings, IRLSAFETY_SET_CENSOR_COLOR, 0xFF404040);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_TEST_EFFECT, false);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_SHOW_PREVIEW, false);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_ENABLE_LOGGING, false);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_PREFER_GPU, true);

	obs_data_set_default_string(settings, IRLSAFETY_SET_CUSTOM_PII_INLINE, "");
	obs_data_set_default_string(settings, IRLSAFETY_SET_CUSTOM_PII_FILE, "");
	obs_data_set_default_string(settings, IRLSAFETY_SET_MODEL_PATH, "");
	obs_data_set_default_double(settings, IRLSAFETY_SET_OVERLAY_OVERLAP, 0.0);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_HYBRID_DELAY_ENABLE, true);
	obs_data_set_default_double(settings, IRLSAFETY_SET_GLOBAL_DELAY_SEC, 0.5);
	obs_data_set_default_double(settings, IRLSAFETY_SET_AUTO_DELAY_SEC, 0.5);
	obs_data_set_default_double(settings, IRLSAFETY_SET_AUTO_DELAY_HOLD_SEC, 2.0);
	obs_data_set_default_double(settings, IRLSAFETY_SET_PARTIAL_PII_THRESHOLD, 0.50);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_SECURE_MODE_ENABLE, true);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_SECURE_DROP_FRAMES, true);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_COVER_WHILE_TYPING, false);
}

float irlsafety_filter_effective_partial_threshold(const irlsafety_filter_settings *settings)
{
	if (!settings)
		return 0.50f;

	if (settings->cover_while_typing && settings->cat_custom_pii)
		return IRLSAFETY_TYPING_COVER_THRESHOLD;

	return settings->partial_pii_threshold;
}

static void copy_string_field(char *dest, size_t dest_size, const char *src)
{
	if (!dest || dest_size == 0)
		return;
	if (!src) {
		dest[0] = '\0';
		return;
	}
	strncpy(dest, src, dest_size - 1);
	dest[dest_size - 1] = '\0';
}

void irlsafety_filter_settings_load(struct obs_data *settings, irlsafety_filter_settings *out)
{
	if (!out)
		return;

	memset(out, 0, sizeof(*out));

	if (!settings) {
		out->enable_all = true;
		out->cat_street_signs = false;
		out->cat_license_plates = false;
		out->cat_documents = false;
		out->cat_faces = false;
		out->cat_screen_text = true;
		out->cat_custom_pii = true;
		out->confidence_threshold = 0.45f;
		out->frame_skip = 2;
		out->ocr_detail = 1;
		out->blur_strength = 12.0f;
		out->censor_mode = IRLSAFETY_CENSOR_BOX;
		out->censor_color = 0xFF404040;
		out->test_effect = false;
		out->prefer_gpu = true;
		out->overlay_overlap = 0.0f;
		out->hybrid_delay_enable = true;
		out->global_delay_sec = 0.5f;
		out->auto_delay_sec = 0.5f;
		out->auto_delay_hold_sec = 2.0f;
		out->partial_pii_threshold = 0.50f;
		out->secure_mode_enable = true;
		out->secure_drop_frames = true;
		out->cover_while_typing = false;
		return;
	}

	out->enable_all = obs_data_get_bool(settings, IRLSAFETY_SET_ENABLE_ALL);
	out->cat_street_signs = obs_data_get_bool(settings, IRLSAFETY_SET_CAT_STREET_SIGNS);
	out->cat_license_plates = obs_data_get_bool(settings, IRLSAFETY_SET_CAT_LICENSE_PLATES);
	out->cat_documents = obs_data_get_bool(settings, IRLSAFETY_SET_CAT_DOCUMENTS);
	out->cat_faces = obs_data_get_bool(settings, IRLSAFETY_SET_CAT_FACES);
	out->cat_screen_text = obs_data_get_bool(settings, IRLSAFETY_SET_CAT_SCREEN_TEXT);
	out->cat_custom_pii = obs_data_get_bool(settings, IRLSAFETY_SET_CAT_CUSTOM_PII);

	out->confidence_threshold = (float)obs_data_get_double(settings, IRLSAFETY_SET_CONFIDENCE);
	out->frame_skip = (int)obs_data_get_int(settings, IRLSAFETY_SET_FRAME_SKIP);
	if (out->frame_skip < 1)
		out->frame_skip = 1;

	out->ocr_detail = (int)obs_data_get_int(settings, IRLSAFETY_SET_OCR_DETAIL);
	if (out->ocr_detail < 0 || out->ocr_detail > 2)
		out->ocr_detail = 1;

	out->blur_strength = (float)obs_data_get_double(settings, IRLSAFETY_SET_BLUR_STRENGTH);
	out->censor_mode = (irlsafety_censor_mode)obs_data_get_int(settings, IRLSAFETY_SET_CENSOR_MODE);
	if (out->censor_mode < IRLSAFETY_CENSOR_BLUR || out->censor_mode > IRLSAFETY_CENSOR_CLOUD)
		out->censor_mode = IRLSAFETY_CENSOR_BOX;
	out->censor_color = (uint32_t)obs_data_get_int(settings, IRLSAFETY_SET_CENSOR_COLOR);
	if (out->censor_color == 0)
		out->censor_color = 0xFF404040;
	out->test_effect = obs_data_get_bool(settings, IRLSAFETY_SET_TEST_EFFECT);
	out->show_preview = obs_data_get_bool(settings, IRLSAFETY_SET_SHOW_PREVIEW);
	out->enable_logging = obs_data_get_bool(settings, IRLSAFETY_SET_ENABLE_LOGGING);
	out->prefer_gpu = obs_data_get_bool(settings, IRLSAFETY_SET_PREFER_GPU);

	copy_string_field(out->custom_pii_inline, sizeof(out->custom_pii_inline),
			  obs_data_get_string(settings, IRLSAFETY_SET_CUSTOM_PII_INLINE));
	copy_string_field(out->custom_pii_file, sizeof(out->custom_pii_file),
			  obs_data_get_string(settings, IRLSAFETY_SET_CUSTOM_PII_FILE));
	copy_string_field(out->model_path, sizeof(out->model_path),
			  obs_data_get_string(settings, IRLSAFETY_SET_MODEL_PATH));

	out->overlay_overlap = (float)obs_data_get_double(settings, IRLSAFETY_SET_OVERLAY_OVERLAP);
	if (out->overlay_overlap < 0.0f)
		out->overlay_overlap = 0.0f;
	if (out->overlay_overlap > 0.95f)
		out->overlay_overlap = 0.95f;

	out->hybrid_delay_enable = obs_data_get_bool(settings, IRLSAFETY_SET_HYBRID_DELAY_ENABLE);
	out->global_delay_sec = (float)obs_data_get_double(settings, IRLSAFETY_SET_GLOBAL_DELAY_SEC);
	if (out->global_delay_sec < 0.0f)
		out->global_delay_sec = 0.0f;
	if (out->global_delay_sec > 10.0f)
		out->global_delay_sec = 10.0f;

	out->auto_delay_sec = (float)obs_data_get_double(settings, IRLSAFETY_SET_AUTO_DELAY_SEC);
	if (out->auto_delay_sec < 0.0f)
		out->auto_delay_sec = 0.0f;
	if (out->auto_delay_sec > 10.0f)
		out->auto_delay_sec = 10.0f;

	out->auto_delay_hold_sec = (float)obs_data_get_double(settings, IRLSAFETY_SET_AUTO_DELAY_HOLD_SEC);
	if (out->auto_delay_hold_sec < 0.0f)
		out->auto_delay_hold_sec = 0.0f;
	if (out->auto_delay_hold_sec > 30.0f)
		out->auto_delay_hold_sec = 30.0f;

	out->partial_pii_threshold = (float)obs_data_get_double(settings, IRLSAFETY_SET_PARTIAL_PII_THRESHOLD);
	if (out->partial_pii_threshold < 0.0f)
		out->partial_pii_threshold = 0.0f;
	if (out->partial_pii_threshold > 1.0f)
		out->partial_pii_threshold = 1.0f;

	out->secure_mode_enable = obs_data_get_bool(settings, IRLSAFETY_SET_SECURE_MODE_ENABLE);
	out->secure_drop_frames = obs_data_get_bool(settings, IRLSAFETY_SET_SECURE_DROP_FRAMES);
	out->cover_while_typing = obs_data_get_bool(settings, IRLSAFETY_SET_COVER_WHILE_TYPING);
}

bool irlsafety_filter_should_run_detection(const irlsafety_filter_settings *settings, uint64_t frame_index,
					   bool heavy_source)
{
	int interval;

	if (!settings || !settings->enable_all)
		return false;

	interval = settings->frame_skip;
	if (interval < 1)
		interval = 1;

	/* Cameras / async sources: scan less often — OCR runs off-thread but frame conversion still costs. */
	if (heavy_source) {
		interval *= 3;
		if (interval < 6)
			interval = 6;
	}

	/* Cover While Typing: scan every frame on display sources so keystrokes are caught immediately. */
	if (!heavy_source && settings->cover_while_typing && settings->cat_custom_pii)
		return true;

	/* Run a few early scans so overlays appear quickly after enabling the filter. */
	if (!heavy_source && frame_index <= 2)
		return true;

	/* When hybrid delay is idle (no recent overlays), scan less often to save CPU. */
	if (settings->hybrid_delay_enable && !irlsafety_hybrid_delay_is_protecting() && interval < 30)
		interval *= 2;

	return (frame_index % (uint64_t)interval) == 0;
}