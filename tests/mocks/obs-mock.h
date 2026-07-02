/*
 * IRLSAFETY+ — minimal OBS API mock for unit tests.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_AV_PLANES 4
#define UNUSED_PARAMETER(x) (void)(x)
#define LOG_WARNING 2
#define LOG_INFO 3

enum video_format {
	VIDEO_FORMAT_NONE,
	VIDEO_FORMAT_I420,
	VIDEO_FORMAT_NV12,
	VIDEO_FORMAT_YVYU,
	VIDEO_FORMAT_YUY2,
	VIDEO_FORMAT_UYVY,
	VIDEO_FORMAT_RGBA,
	VIDEO_FORMAT_BGRA,
	VIDEO_FORMAT_BGRX,
};

enum obs_source_type {
	OBS_SOURCE_TYPE_INPUT,
	OBS_SOURCE_TYPE_FILTER,
	OBS_SOURCE_TYPE_TRANSITION,
	OBS_SOURCE_TYPE_SCENE,
};

#define OBS_SOURCE_VIDEO (1 << 0)
#define OBS_SOURCE_SRGB (1 << 15)

enum obs_text_type {
	OBS_TEXT_DEFAULT,
	OBS_TEXT_PASSWORD,
	OBS_TEXT_MULTILINE,
	OBS_TEXT_INFO,
};

enum obs_path_type {
	OBS_PATH_FILE,
	OBS_PATH_DIRECTORY,
};

enum obs_group_type {
	OBS_GROUP_NORMAL,
	OBS_GROUP_CHECKABLE,
};

enum obs_combo_type {
	OBS_COMBO_TYPE_INVALID,
	OBS_COMBO_TYPE_EDITABLE,
	OBS_COMBO_TYPE_LIST,
};

enum obs_combo_format {
	OBS_COMBO_FORMAT_INT,
	OBS_COMBO_FORMAT_FLOAT,
	OBS_COMBO_FORMAT_STRING,
};

struct obs_property {
	int unused;
};

typedef struct obs_data obs_data_t;
typedef struct obs_properties obs_properties_t;
typedef struct obs_property obs_property_t;
typedef struct obs_source obs_source_t;
typedef struct obs_module obs_module_t;
typedef struct gs_effect gs_effect_t;

typedef bool (*obs_property_modified_t)(obs_properties_t *props, obs_property_t *property, obs_data_t *settings);
typedef bool (*obs_property_modified2_t)(void *priv, obs_properties_t *props, obs_property_t *property,
					 obs_data_t *settings);

struct obs_source_frame {
	uint8_t *data[MAX_AV_PLANES];
	uint32_t linesize[MAX_AV_PLANES];
	uint32_t width;
	uint32_t height;
	uint64_t timestamp;
	enum video_format format;
};

struct obs_source_info {
	const char *id;
	enum obs_source_type type;
	uint32_t output_flags;
	const char *(*get_name)(void *type_data);
	void *(*create)(obs_data_t *settings, obs_source_t *source);
	void (*destroy)(void *data);
	void (*get_defaults)(obs_data_t *settings);
	obs_properties_t *(*get_properties)(void *data);
	void (*update)(void *data, obs_data_t *settings);
	void (*video_tick)(void *data, float seconds);
	void (*video_render)(void *data, gs_effect_t *effect);
	struct obs_source_frame *(*filter_video)(void *data, struct obs_source_frame *frame);
};

#define OBS_DECLARE_MODULE()

const char *obs_module_text(const char *val);

void obs_mock_reset(void);
size_t obs_mock_registered_source_count(void);
const struct obs_source_info *obs_mock_get_registered_source(size_t index);
const char *obs_mock_last_property_label(void);
size_t obs_mock_property_count(void);
int obs_mock_module_load_calls(void);

void obs_register_source_s(const struct obs_source_info *info, size_t size);
#define obs_register_source(info) obs_register_source_s(info, sizeof(struct obs_source_info))

void blogva(int log_level, const char *format, va_list args);
void *bzalloc(size_t size);
void bfree(void *ptr);

obs_data_t *obs_data_create(void);
void obs_data_release(obs_data_t *data);
bool obs_data_get_bool(obs_data_t *data, const char *name);
void obs_data_set_default_bool(obs_data_t *data, const char *name, bool val);
int obs_data_get_int(obs_data_t *data, const char *name);
void obs_data_set_default_int(obs_data_t *data, const char *name, int val);
double obs_data_get_double(obs_data_t *data, const char *name);
void obs_data_set_default_double(obs_data_t *data, const char *name, double val);
const char *obs_data_get_string(obs_data_t *data, const char *name);
void obs_data_set_default_string(obs_data_t *data, const char *name, const char *val);

obs_properties_t *obs_properties_create(void);
obs_properties_t *obs_properties_add_group(obs_properties_t *props, const char *name, const char *desc,
					   enum obs_group_type type, obs_properties_t *group);
obs_property_t *obs_properties_add_text(obs_properties_t *props, const char *name, const char *desc,
					enum obs_text_type type);
obs_properties_t *obs_properties_add_bool(obs_properties_t *props, const char *name, const char *desc);
obs_properties_t *obs_properties_add_float(obs_properties_t *props, const char *name, const char *desc, double min,
					   double max, double step);
obs_properties_t *obs_properties_add_float_slider(obs_properties_t *props, const char *name, const char *desc,
						  double min, double max, double step);
obs_properties_t *obs_properties_add_int_slider(obs_properties_t *props, const char *name, const char *desc, int min,
						  int max, int step);
obs_properties_t *obs_properties_add_path(obs_properties_t *props, const char *name, const char *desc,
					    enum obs_path_type type, const char *filter, const char *default_path);
obs_property_t *obs_properties_add_list(obs_properties_t *props, const char *name, const char *desc,
					enum obs_combo_type type, enum obs_combo_format format);
obs_property_t *obs_properties_add_color(obs_properties_t *props, const char *name, const char *desc);
obs_property_t *obs_properties_add_color_alpha(obs_properties_t *props, const char *name, const char *desc);
obs_property_t *obs_properties_get(obs_properties_t *props, const char *name);
void obs_property_set_visible(obs_property_t *prop, bool visible);
void obs_property_set_modified_callback(obs_property_t *prop, obs_property_modified_t callback);
void obs_property_set_modified_callback2(obs_property_t *prop, obs_property_modified2_t callback, void *priv);
void obs_property_list_add_int(obs_property_t *prop, const char *name, long long val);

void obs_enter_graphics(void);
void obs_leave_graphics(void);