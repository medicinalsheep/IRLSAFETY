/*
 * IRLSAFETY+ — OBS filter settings (loaded from obs_data_t).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

struct obs_data;

#ifdef __cplusplus
extern "C" {
#endif

/* OBS settings key names — keep stable for scene collection compatibility. */
#define IRLSAFETY_SET_ENABLE_ALL "enable_all"
#define IRLSAFETY_SET_CAT_STREET_SIGNS "cat_street_signs"
#define IRLSAFETY_SET_CAT_LICENSE_PLATES "cat_license_plates"
#define IRLSAFETY_SET_CAT_DOCUMENTS "cat_documents"
#define IRLSAFETY_SET_CAT_FACES "cat_faces"
#define IRLSAFETY_SET_CAT_SCREEN_TEXT "cat_screen_text"
#define IRLSAFETY_SET_CAT_CUSTOM_PII "cat_custom_pii"
#define IRLSAFETY_SET_CONFIDENCE "confidence_threshold"
#define IRLSAFETY_SET_FRAME_SKIP "frame_skip"
#define IRLSAFETY_SET_BLUR_STRENGTH "blur_strength"
#define IRLSAFETY_SET_SHOW_PREVIEW "show_preview"
#define IRLSAFETY_SET_ENABLE_LOGGING "enable_logging"
#define IRLSAFETY_SET_PREFER_GPU "prefer_gpu"
#define IRLSAFETY_SET_CUSTOM_PII_INLINE "custom_pii_inline"
#define IRLSAFETY_SET_CUSTOM_PII_FILE "custom_pii_file"
#define IRLSAFETY_SET_MODEL_PATH "model_path"

typedef struct irlsafety_filter_settings {
	bool enable_all;
	bool cat_street_signs;
	bool cat_license_plates;
	bool cat_documents;
	bool cat_faces;
	bool cat_screen_text;
	bool cat_custom_pii;
	float confidence_threshold;
	int frame_skip;
	float blur_strength;
	bool show_preview;
	bool enable_logging;
	bool prefer_gpu;
	char custom_pii_inline[4096];
	char custom_pii_file[1024];
	char model_path[1024];
} irlsafety_filter_settings;

void irlsafety_filter_settings_set_defaults(struct obs_data *settings);
void irlsafety_filter_settings_load(struct obs_data *settings, irlsafety_filter_settings *out);

/* Returns true when the pipeline should run on this frame index (0-based). */
bool irlsafety_filter_should_process_frame(const irlsafety_filter_settings *settings, uint64_t frame_index);

#ifdef __cplusplus
}
#endif