/*
 * IRLSAFETY+ — OBS filter settings (loaded from obs_data_t).
 * Copyright (c) 2026 medicinalsheep. MIT License.
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
	obs_data_set_default_bool(settings, IRLSAFETY_SET_CAT_LICENSE_PLATES, true);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_CAT_STREET_SIGNS, true);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_CAT_SHIPPING_LABELS, true);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_CAT_ID_DOCUMENTS, true);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_CAT_SCREEN_TEXT, true);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_CAT_CUSTOM_PII, true);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_CAT_SENSITIVE_PATTERNS, false);

	obs_data_set_default_double(settings, IRLSAFETY_SET_CONFIDENCE, 0.35);
	obs_data_set_default_int(settings, IRLSAFETY_SET_FRAME_SKIP, 3);
	obs_data_set_default_int(settings, IRLSAFETY_SET_CENSOR_MODE, IRLSAFETY_CENSOR_BOX);
	obs_data_set_default_int(settings, IRLSAFETY_SET_CENSOR_COLOR, 0xFF000000);
	obs_data_set_default_string(settings, IRLSAFETY_SET_CENSOR_OVERLAY, "");
	obs_data_set_default_bool(settings, IRLSAFETY_SET_ANGLED_COVER, true);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_HYBRID_DELAY_ENABLE, true);
	obs_data_set_default_double(settings, IRLSAFETY_SET_GLOBAL_DELAY_SEC, 1.5);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_SECURE_MODE_ENABLE, true);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_TEST_EFFECT, false);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_SHOW_PREVIEW, false);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_ENABLE_LOGGING, false);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_PREFER_GPU, true);

	obs_data_set_default_string(settings, IRLSAFETY_SET_CUSTOM_PII_INLINE, "");
	obs_data_set_default_string(settings, IRLSAFETY_SET_CUSTOM_PII_FILE, "");
	obs_data_set_default_string(settings, IRLSAFETY_SET_MODEL_PATH, "");

	/* Advanced — hidden defaults */
	obs_data_set_default_int(settings, IRLSAFETY_SET_OCR_DETAIL, 0);
	obs_data_set_default_double(settings, IRLSAFETY_SET_BLUR_STRENGTH, 24.0);
	obs_data_set_default_double(settings, IRLSAFETY_SET_AUTO_DELAY_SEC, 1.0);
	obs_data_set_default_double(settings, IRLSAFETY_SET_AUTO_DELAY_HOLD_SEC, 4.0);
	obs_data_set_default_double(settings, IRLSAFETY_SET_PARTIAL_PII_THRESHOLD, 0.50);
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

static bool settings_key_has_user_value(struct obs_data *settings, const char *key)
{
	if (!settings || !key)
		return false;
	return obs_data_has_user_value(settings, key);
}

static bool settings_get_bool(struct obs_data *settings, const char *key, bool default_val)
{
	if (!settings)
		return default_val;
	if (!settings_key_has_user_value(settings, key))
		return default_val;
	return obs_data_get_bool(settings, key);
}

static void irlsafety_filter_apply_defaults_struct(irlsafety_filter_settings *out)
{
	out->enable_all = true;
	out->cat_license_plates = true;
	out->cat_street_signs = true;
	out->cat_shipping_labels = true;
	out->cat_id_documents = true;
	out->cat_screen_text = true;
	out->cat_custom_pii = true;
	out->cat_sensitive_patterns = false;
	out->confidence_threshold = 0.35f;
	out->frame_skip = 3;
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
}

void irlsafety_filter_settings_load(struct obs_data *settings, irlsafety_filter_settings *out)
{
	bool legacy_documents;
	bool legacy_faces;

	if (!out)
		return;

	memset(out, 0, sizeof(*out));

	if (!settings) {
		irlsafety_filter_apply_defaults_struct(out);
		return;
	}

	out->enable_all = settings_get_bool(settings, IRLSAFETY_SET_ENABLE_ALL, true);
	out->cat_street_signs = settings_get_bool(settings, IRLSAFETY_SET_CAT_STREET_SIGNS, true);
	out->cat_license_plates = settings_get_bool(settings, IRLSAFETY_SET_CAT_LICENSE_PLATES, true);
	out->cat_shipping_labels = settings_get_bool(settings, IRLSAFETY_SET_CAT_SHIPPING_LABELS, true);
	out->cat_id_documents = settings_get_bool(settings, IRLSAFETY_SET_CAT_ID_DOCUMENTS, true);
	out->cat_screen_text = settings_get_bool(settings, IRLSAFETY_SET_CAT_SCREEN_TEXT, true);
	out->cat_sensitive_patterns = settings_get_bool(settings, IRLSAFETY_SET_CAT_SENSITIVE_PATTERNS, false);
	out->cat_custom_pii = settings_get_bool(settings, IRLSAFETY_SET_CAT_CUSTOM_PII, true);

	legacy_documents = settings_key_has_user_value(settings, IRLSAFETY_SET_CAT_DOCUMENTS) &&
			   obs_data_get_bool(settings, IRLSAFETY_SET_CAT_DOCUMENTS);
	legacy_faces = settings_key_has_user_value(settings, IRLSAFETY_SET_CAT_FACES) &&
		       obs_data_get_bool(settings, IRLSAFETY_SET_CAT_FACES);
	if (legacy_documents || legacy_faces)
		out->cat_id_documents = true;
	if (legacy_documents)
		out->cat_shipping_labels = true;

	out->confidence_threshold = (float)obs_data_get_double(settings, IRLSAFETY_SET_CONFIDENCE);
	out->frame_skip = (int)obs_data_get_int(settings, IRLSAFETY_SET_FRAME_SKIP);
	if (out->frame_skip < 1)
		out->frame_skip = 1;

	out->ocr_detail = (int)obs_data_get_int(settings, IRLSAFETY_SET_OCR_DETAIL);
	if (out->ocr_detail < 0 || out->ocr_detail > 2)
		out->ocr_detail = 0;

	out->blur_strength = (float)obs_data_get_double(settings, IRLSAFETY_SET_BLUR_STRENGTH);
	out->censor_mode = (irlsafety_censor_mode)obs_data_get_int(settings, IRLSAFETY_SET_CENSOR_MODE);
	if (out->censor_mode < IRLSAFETY_CENSOR_BLUR || out->censor_mode > IRLSAFETY_CENSOR_OVERLAY)
		out->censor_mode = IRLSAFETY_CENSOR_BOX;
	out->censor_color = (uint32_t)obs_data_get_int(settings, IRLSAFETY_SET_CENSOR_COLOR);
	copy_string_field(out->censor_overlay_file, sizeof(out->censor_overlay_file),
			  obs_data_get_string(settings, IRLSAFETY_SET_CENSOR_OVERLAY));
	if (out->censor_color == 0)
		out->censor_color = 0xFF000000;
	out->angled_cover = settings_get_bool(settings, IRLSAFETY_SET_ANGLED_COVER, true);
	out->test_effect = settings_get_bool(settings, IRLSAFETY_SET_TEST_EFFECT, false);
	out->show_preview = settings_get_bool(settings, IRLSAFETY_SET_SHOW_PREVIEW, false);
	out->enable_logging = settings_get_bool(settings, IRLSAFETY_SET_ENABLE_LOGGING, false);
	out->prefer_gpu = settings_get_bool(settings, IRLSAFETY_SET_PREFER_GPU, true);

	copy_string_field(out->custom_pii_inline, sizeof(out->custom_pii_inline),
			  obs_data_get_string(settings, IRLSAFETY_SET_CUSTOM_PII_INLINE));
	copy_string_field(out->custom_pii_file, sizeof(out->custom_pii_file),
			  obs_data_get_string(settings, IRLSAFETY_SET_CUSTOM_PII_FILE));
	copy_string_field(out->model_path, sizeof(out->model_path),
			  obs_data_get_string(settings, IRLSAFETY_SET_MODEL_PATH));

	out->hybrid_delay_enable = settings_get_bool(settings, IRLSAFETY_SET_HYBRID_DELAY_ENABLE, true);
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

	out->secure_mode_enable = settings_get_bool(settings, IRLSAFETY_SET_SECURE_MODE_ENABLE, true);
	out->secure_drop_frames = settings_get_bool(settings, IRLSAFETY_SET_SECURE_DROP_FRAMES, true);
	out->cover_while_typing = settings_get_bool(settings, IRLSAFETY_SET_COVER_WHILE_TYPING, false);
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

bool irlsafety_filter_detection_enabled(const irlsafety_filter_settings *settings)
{
	if (!settings || !settings->enable_all)
		return false;

	return settings->cat_license_plates || settings->cat_street_signs || settings->cat_shipping_labels ||
	       settings->cat_id_documents;
}