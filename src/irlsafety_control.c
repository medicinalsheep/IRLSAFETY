/*
 * IRLSAFETY+ — control panel API (dock + filter management).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "irlsafety_control.h"

#include "filter_settings.h"
#include "pii-filter.h"

#ifndef IRLSAFETY_TEST_BUILD
#include <obs-frontend-api.h>
#endif

#include <obs-module.h>
#include <util/platform.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#endif

typedef struct irlsafety_setting_patch {
	const char *key;
	bool value;
} irlsafety_setting_patch;

typedef struct irlsafety_path_patch {
	char path[1024];
} irlsafety_path_patch;

static void irlsafety_control_apply_bool_patch(obs_data_t *settings, void *param)
{
	irlsafety_setting_patch *patch = param;

	if (!settings || !patch || !patch->key)
		return;

	obs_data_set_bool(settings, patch->key, patch->value);
}

static void irlsafety_control_apply_model_path_patch(obs_data_t *settings, void *param)
{
	irlsafety_path_patch *patch = param;

	if (!settings || !patch)
		return;

	obs_data_set_string(settings, IRLSAFETY_SET_MODEL_PATH, patch->path);
}

static int irlsafety_control_update_setting(obs_source_t *filter, void (*apply)(obs_data_t *, void *), void *param)
{
	obs_data_t *settings;

	if (!filter || !apply)
		return -1;

	settings = obs_source_get_settings(filter);
	if (!settings)
		return -1;

	apply(settings, param);
	obs_source_update(filter, settings);
	obs_data_release(settings);
	return 0;
}

static bool irlsafety_control_enum_source(void *param, obs_source_t *source)
{
	irlsafety_filter_list *list = param;
	obs_source_t *parent;
	const char *id;
	const char *name;
	const char *parent_name;
	char display[sizeof(list->entries[0].display_name)];

	if (!list || !source || list->count >= IRLSAFETY_MAX_FILTER_ENTRIES)
		return true;

	id = obs_source_get_id(source);
	if (!id || strcmp(id, IRLSAFETY_PII_FILTER_ID) != 0)
		return true;

	name = obs_source_get_name(source);
	parent = obs_filter_get_parent(source);
	parent_name = parent ? obs_source_get_name(parent) : NULL;

	if (parent_name && name)
		snprintf(display, sizeof(display), "%s → %s", parent_name, name);
	else if (name)
		snprintf(display, sizeof(display), "%s", name);
	else
		snprintf(display, sizeof(display), "IRLSAFETY+");

	irlsafety_filter_entry *entry = &list->entries[list->count++];
	strncpy(entry->display_name, display, sizeof(entry->display_name) - 1);
	entry->display_name[sizeof(entry->display_name) - 1] = '\0';
	entry->source = source;
	obs_source_get_ref(source);
	return true;
}

void irlsafety_control_refresh_filters(irlsafety_filter_list *list)
{
	size_t i;

	if (!list)
		return;

	for (i = 0; i < list->count; i++) {
		if (list->entries[i].source)
			obs_source_release(list->entries[i].source);
	}

	memset(list, 0, sizeof(*list));
	obs_enum_sources(irlsafety_control_enum_source, list);
}

int irlsafety_control_get_status(obs_source_t *filter, irlsafety_runtime_status *out)
{
	if (!filter || !out)
		return -1;

	return irlsafety_filter_get_runtime_status(filter, out);
}

int irlsafety_control_set_bool_setting(obs_source_t *filter, const char *key, bool value)
{
	irlsafety_setting_patch patch = {.key = key, .value = value};

	if (!key)
		return -1;

	return irlsafety_control_update_setting(filter, irlsafety_control_apply_bool_patch, &patch);
}

int irlsafety_control_reload_model(obs_source_t *filter)
{
	return irlsafety_filter_reload_model(filter);
}

int irlsafety_control_set_model_path(obs_source_t *filter, const char *path)
{
	irlsafety_path_patch patch;

	if (!path)
		return -1;

	strncpy(patch.path, path, sizeof(patch.path) - 1);
	patch.path[sizeof(patch.path) - 1] = '\0';
	return irlsafety_control_update_setting(filter, irlsafety_control_apply_model_path_patch, &patch);
}

void irlsafety_control_resolve_default_model_path(char *dest, size_t dest_size)
{
	if (!dest || dest_size == 0)
		return;

	dest[0] = '\0';

#ifndef IRLSAFETY_TEST_BUILD
	{
		char *bundled = obs_module_file("models/irlsafety-detect.onnx");

		if (bundled) {
			strncpy(dest, bundled, dest_size - 1);
			dest[dest_size - 1] = '\0';
			bfree(bundled);
		}
	}
#endif
}

void irlsafety_control_get_models_folder(char *dest, size_t dest_size)
{
	if (!dest || dest_size == 0)
		return;

	dest[0] = '\0';

#ifndef IRLSAFETY_TEST_BUILD
	{
		char *models = obs_module_file("models");

		if (models) {
			strncpy(dest, models, dest_size - 1);
			dest[dest_size - 1] = '\0';
			bfree(models);
		}
	}
#endif
}

void irlsafety_control_get_training_folder(char *dest, size_t dest_size)
{
	if (!dest || dest_size == 0)
		return;

	dest[0] = '\0';

#ifndef IRLSAFETY_TEST_BUILD
	{
		char *training = obs_module_config_path("training");

		if (training) {
			strncpy(dest, training, dest_size - 1);
			dest[dest_size - 1] = '\0';
			bfree(training);
		}
	}
#endif
}

bool irlsafety_control_ensure_dir(const char *path)
{
	if (!path || path[0] == '\0')
		return false;

	if (os_file_exists(path))
		return true;

	return os_mkdirs(path) == 0;
}

bool irlsafety_control_open_path(const char *path)
{
	if (!path || path[0] == '\0')
		return false;

#ifdef _WIN32
	HINSTANCE result = ShellExecuteA(NULL, "open", path, NULL, NULL, SW_SHOWNORMAL);
	return (INT_PTR)result > 32;
#else
	UNUSED_PARAMETER(path);
	return false;
#endif
}

int irlsafety_control_save_training_screenshot(void)
{
#ifndef IRLSAFETY_TEST_BUILD
	char training_root[1024];
	char train_dir[1200];
	char dest_path[1400];
	char *screenshot = NULL;
	time_t now;
	struct tm tm_now;
	FILE *in;
	FILE *out;
	unsigned char buffer[8192];
	size_t nread;

	irlsafety_control_get_training_folder(training_root, sizeof(training_root));
	if (training_root[0] == '\0')
		return -1;

	if (!irlsafety_control_ensure_dir(training_root))
		return -1;

	snprintf(train_dir, sizeof(train_dir), "%s/images/train", training_root);
	if (!irlsafety_control_ensure_dir(train_dir))
		return -1;

	obs_frontend_take_screenshot();
	screenshot = obs_frontend_get_last_screenshot();
	if (!screenshot || screenshot[0] == '\0') {
		bfree(screenshot);
		return -1;
	}

	now = time(NULL);
#ifdef _WIN32
	localtime_s(&tm_now, &now);
#else
	struct tm *tm_ptr = localtime(&now);
	if (!tm_ptr) {
		bfree(screenshot);
		return -1;
	}
	tm_now = *tm_ptr;
#endif
	snprintf(dest_path, sizeof(dest_path), "%s/irlsafety-frame-%04d%02d%02d-%02d%02d%02d.png", train_dir,
		 tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);

	in = fopen(screenshot, "rb");
	if (!in) {
		bfree(screenshot);
		return -1;
	}

	out = fopen(dest_path, "wb");
	if (!out) {
		fclose(in);
		bfree(screenshot);
		return -1;
	}

	while ((nread = fread(buffer, 1, sizeof(buffer), in)) > 0)
		fwrite(buffer, 1, nread, out);

	fclose(in);
	fclose(out);
	bfree(screenshot);
	return 0;
#else
	return -1;
#endif
}