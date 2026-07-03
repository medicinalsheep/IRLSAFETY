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
#define IRLSAFETY_SET_CAT_SHIPPING_LABELS "cat_shipping_labels"
#define IRLSAFETY_SET_CAT_ID_DOCUMENTS "cat_id_documents"
/* Legacy keys — migrated on load; hidden from UI. */
#define IRLSAFETY_SET_CAT_DOCUMENTS "cat_documents"
#define IRLSAFETY_SET_CAT_FACES "cat_faces"
#define IRLSAFETY_SET_CAT_SCREEN_TEXT "cat_screen_text"
#define IRLSAFETY_SET_CAT_SENSITIVE_PATTERNS "cat_sensitive_patterns"
#define IRLSAFETY_SET_CAT_CUSTOM_PII "cat_custom_pii"
#define IRLSAFETY_SET_CONFIDENCE "confidence_threshold"
#define IRLSAFETY_SET_FRAME_SKIP "frame_skip"
#define IRLSAFETY_SET_OCR_DETAIL "ocr_detail"
#define IRLSAFETY_SET_BLUR_STRENGTH "blur_strength"
#define IRLSAFETY_SET_CENSOR_MODE "censor_mode"
#define IRLSAFETY_SET_CENSOR_COLOR "censor_color"
#define IRLSAFETY_SET_CENSOR_OVERLAY "censor_overlay_file"
#define IRLSAFETY_SET_TEST_EFFECT "test_effect"
#define IRLSAFETY_SET_SHOW_PREVIEW "show_preview"
#define IRLSAFETY_SET_ENABLE_LOGGING "enable_logging"
#define IRLSAFETY_SET_PREFER_GPU "prefer_gpu"
#define IRLSAFETY_SET_USE_CHILD_OCR "use_child_ocr"
#define IRLSAFETY_SET_CUSTOM_PII_INLINE "custom_pii_inline"
#define IRLSAFETY_SET_CUSTOM_PII_FILE "custom_pii_file"
#define IRLSAFETY_SET_MODEL_PATH "model_path"
#define IRLSAFETY_SET_ANGLED_COVER "angled_cover"
#define IRLSAFETY_SET_HYBRID_DELAY_ENABLE "hybrid_delay_enable"
#define IRLSAFETY_SET_GLOBAL_DELAY_SEC "global_delay_sec"
#define IRLSAFETY_SET_AUTO_DELAY_SEC "auto_delay_sec"
#define IRLSAFETY_SET_AUTO_DELAY_HOLD_SEC "auto_delay_hold_sec"
#define IRLSAFETY_SET_PARTIAL_PII_THRESHOLD "partial_pii_threshold"
#define IRLSAFETY_SET_SECURE_MODE_ENABLE "secure_mode_enable"
#define IRLSAFETY_SET_SECURE_DROP_FRAMES "secure_drop_frames"
#define IRLSAFETY_SET_COVER_WHILE_TYPING "cover_while_typing"

#define IRLSAFETY_TYPING_COVER_THRESHOLD 0.01f

typedef enum irlsafety_censor_mode {
	IRLSAFETY_CENSOR_BLUR = 0,
	IRLSAFETY_CENSOR_BOX = 1,
	IRLSAFETY_CENSOR_ELLIPSE = 2,
	IRLSAFETY_CENSOR_CLOUD = 3,
	IRLSAFETY_CENSOR_OVERLAY = 4,
} irlsafety_censor_mode;

typedef struct irlsafety_filter_settings {
	bool enable_all;
	bool cat_street_signs;
	bool cat_license_plates;
	bool cat_shipping_labels;
	bool cat_id_documents;
	bool cat_screen_text;
	bool cat_sensitive_patterns;
	bool cat_custom_pii;
	float confidence_threshold;
	int frame_skip;
	int ocr_detail;
	float blur_strength;
	irlsafety_censor_mode censor_mode;
	uint32_t censor_color;
	char censor_overlay_file[1024];
	bool test_effect;
	bool show_preview;
	bool enable_logging;
	bool prefer_gpu;
	bool use_child_ocr;
	char custom_pii_inline[4096];
	char custom_pii_file[1024];
	char model_path[1024];
	bool angled_cover;
	bool hybrid_delay_enable;
	float global_delay_sec;
	float auto_delay_sec;
	float auto_delay_hold_sec;
	float partial_pii_threshold;
	bool secure_mode_enable;
	bool secure_drop_frames;
	bool cover_while_typing;
} irlsafety_filter_settings;

void irlsafety_filter_settings_set_defaults(struct obs_data *settings);
void irlsafety_filter_settings_load(struct obs_data *settings, irlsafety_filter_settings *out);

/*
 * Returns true when OCR / detection should run on this frame index (0-based).
 * heavy_source: async camera / NDI — scans less often to avoid stalling the video thread.
 */
bool irlsafety_filter_should_run_detection(const irlsafety_filter_settings *settings, uint64_t frame_index,
					   bool heavy_source);

/* Partial PII threshold used for custom keyword matching (typing mode overrides to near-zero). */
float irlsafety_filter_effective_partial_threshold(const irlsafety_filter_settings *settings);

bool irlsafety_filter_detection_enabled(const irlsafety_filter_settings *settings);

#ifdef __cplusplus
}
#endif