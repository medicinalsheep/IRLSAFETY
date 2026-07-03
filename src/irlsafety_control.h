/*
 * IRLSAFETY+ — control panel API (dock + filter management).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct obs_source;

#ifdef __cplusplus
extern "C" {
#endif

#define IRLSAFETY_MAX_FILTER_ENTRIES 32

typedef struct irlsafety_runtime_status {
	bool protection_enabled;
	bool detector_ready;
	bool ocr_available;
	bool ocr_busy;
	char ocr_backend[96];
	char ocr_backend_status[192];
	bool model_file_exists;
	char model_path[1024];
	char detector_message[256];
	uint32_t overlay_count;
	uint64_t frame_count;
	bool cat_license_plates;
	bool cat_street_signs;
	bool cat_screen_text;
	bool cat_sensitive_patterns;
	bool cat_shipping_labels;
	bool cat_id_documents;
	bool cat_custom_pii;
} irlsafety_runtime_status;

typedef struct irlsafety_filter_entry {
	char display_name[320];
	struct obs_source *source;
} irlsafety_filter_entry;

typedef struct irlsafety_filter_list {
	size_t count;
	irlsafety_filter_entry entries[IRLSAFETY_MAX_FILTER_ENTRIES];
} irlsafety_filter_list;

void irlsafety_control_refresh_filters(irlsafety_filter_list *list);
void irlsafety_control_release_filters(irlsafety_filter_list *list);
int irlsafety_control_get_status(struct obs_source *filter, irlsafety_runtime_status *out);
int irlsafety_control_set_bool_setting(struct obs_source *filter, const char *key, bool value);
int irlsafety_control_reload_model(struct obs_source *filter);
int irlsafety_control_set_model_path(struct obs_source *filter, const char *path);

void irlsafety_control_resolve_default_model_path(char *dest, size_t dest_size);
void irlsafety_control_get_models_folder(char *dest, size_t dest_size);
void irlsafety_control_get_training_folder(char *dest, size_t dest_size);

bool irlsafety_control_ensure_dir(const char *path);
bool irlsafety_control_open_path(const char *path);
bool irlsafety_control_open_models_guide(const char *filename);
int irlsafety_control_save_training_screenshot(void);

/* Launch a bundled PowerShell script from data/scripts/ (training pipeline). */
int irlsafety_control_run_script(const char *script_relative_path);

#ifdef __cplusplus
}
#endif