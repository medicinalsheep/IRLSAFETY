/*
 * IRLSAFETY+ — OBS filter settings (loaded from obs_data_t).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "filter_settings.h"

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
	obs_data_set_default_bool(settings, IRLSAFETY_SET_CAT_STREET_SIGNS, true);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_CAT_LICENSE_PLATES, true);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_CAT_DOCUMENTS, true);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_CAT_FACES, true);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_CAT_SCREEN_TEXT, true);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_CAT_CUSTOM_PII, true);

	obs_data_set_default_double(settings, IRLSAFETY_SET_CONFIDENCE, 0.45);
	obs_data_set_default_int(settings, IRLSAFETY_SET_FRAME_SKIP, 1);
	obs_data_set_default_double(settings, IRLSAFETY_SET_BLUR_STRENGTH, 12.0);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_SHOW_PREVIEW, false);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_ENABLE_LOGGING, false);
	obs_data_set_default_bool(settings, IRLSAFETY_SET_PREFER_GPU, true);

	obs_data_set_default_string(settings, IRLSAFETY_SET_CUSTOM_PII_INLINE, "");
	obs_data_set_default_string(settings, IRLSAFETY_SET_CUSTOM_PII_FILE, "");
	obs_data_set_default_string(settings, IRLSAFETY_SET_MODEL_PATH, "");
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
		out->cat_street_signs = true;
		out->cat_license_plates = true;
		out->cat_documents = true;
		out->cat_faces = true;
		out->cat_screen_text = true;
		out->cat_custom_pii = true;
		out->confidence_threshold = 0.45f;
		out->frame_skip = 1;
		out->blur_strength = 12.0f;
		out->prefer_gpu = true;
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

	out->blur_strength = (float)obs_data_get_double(settings, IRLSAFETY_SET_BLUR_STRENGTH);
	out->show_preview = obs_data_get_bool(settings, IRLSAFETY_SET_SHOW_PREVIEW);
	out->enable_logging = obs_data_get_bool(settings, IRLSAFETY_SET_ENABLE_LOGGING);
	out->prefer_gpu = obs_data_get_bool(settings, IRLSAFETY_SET_PREFER_GPU);

	copy_string_field(out->custom_pii_inline, sizeof(out->custom_pii_inline),
			  obs_data_get_string(settings, IRLSAFETY_SET_CUSTOM_PII_INLINE));
	copy_string_field(out->custom_pii_file, sizeof(out->custom_pii_file),
			  obs_data_get_string(settings, IRLSAFETY_SET_CUSTOM_PII_FILE));
	copy_string_field(out->model_path, sizeof(out->model_path),
			  obs_data_get_string(settings, IRLSAFETY_SET_MODEL_PATH));
}

bool irlsafety_filter_should_process_frame(const irlsafety_filter_settings *settings, uint64_t frame_index)
{
	int interval;

	if (!settings || !settings->enable_all)
		return false;

	interval = settings->frame_skip;
	if (interval < 1)
		interval = 1;

	return (frame_index % (uint64_t)interval) == 0;
}