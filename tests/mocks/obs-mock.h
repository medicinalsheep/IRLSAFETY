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

enum obs_text_type {
	OBS_TEXT_DEFAULT,
	OBS_TEXT_PASSWORD,
	OBS_TEXT_MULTILINE,
	OBS_TEXT_INFO,
};

typedef struct obs_data obs_data_t;
typedef struct obs_properties obs_properties_t;
typedef struct obs_source obs_source_t;
typedef struct obs_module obs_module_t;

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
	struct obs_source_frame *(*filter_video)(void *data, struct obs_source_frame *frame);
};

#define OBS_DECLARE_MODULE()

const char *obs_module_text(const char *val);

void obs_mock_reset(void);
size_t obs_mock_registered_source_count(void);
const struct obs_source_info *obs_mock_get_registered_source(size_t index);
const char *obs_mock_last_property_label(void);
int obs_mock_module_load_calls(void);
int obs_mock_filter_video_calls(void);

void obs_register_source_s(const struct obs_source_info *info, size_t size);
#define obs_register_source(info) obs_register_source_s(info, sizeof(struct obs_source_info))

void blogva(int log_level, const char *format, va_list args);
void *bzalloc(size_t size);
void bfree(void *ptr);

obs_data_t *obs_data_create(void);
void obs_data_release(obs_data_t *data);
double obs_data_get_double(obs_data_t *data, const char *name);
void obs_data_set_default_double(obs_data_t *data, const char *name, double val);

obs_properties_t *obs_properties_create(void);
void obs_properties_destroy(obs_properties_t *props);
obs_properties_t *obs_properties_add_text(obs_properties_t *props, const char *name, const char *desc,
					    enum obs_text_type type);
obs_properties_t *obs_properties_add_float(obs_properties_t *props, const char *name, const char *desc, double min,
					   double max, double step);