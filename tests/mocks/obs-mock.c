/*
 * IRLSAFETY+ — minimal OBS API mock for unit tests.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "obs-mock.h"

#define OBS_MOCK_MAX_SOURCES 8

static struct obs_source_info g_registered_sources[OBS_MOCK_MAX_SOURCES];
static size_t g_registered_source_count;
static char g_last_property_label[256];
static int g_module_load_calls;
static int g_filter_video_calls;

void obs_mock_reset(void)
{
	g_registered_source_count = 0;
	g_last_property_label[0] = '\0';
	g_module_load_calls = 0;
	g_filter_video_calls = 0;
	memset(g_registered_sources, 0, sizeof(g_registered_sources));
}

size_t obs_mock_registered_source_count(void)
{
	return g_registered_source_count;
}

const struct obs_source_info *obs_mock_get_registered_source(size_t index)
{
	if (index >= g_registered_source_count)
		return NULL;
	return &g_registered_sources[index];
}

const char *obs_mock_last_property_label(void)
{
	return g_last_property_label;
}

int obs_mock_module_load_calls(void)
{
	return g_module_load_calls;
}

int obs_mock_filter_video_calls(void)
{
	return g_filter_video_calls;
}

const char *obs_module_text(const char *val)
{
	if (strcmp(val, "IRLSAFETYPlus.FilterName") == 0)
		return "IRLSAFETY+ PII Blur";
	if (strcmp(val, "IRLSAFETYPlus.FilterDescription") == 0)
		return "Real-time PII detection and blur filter";
	if (strcmp(val, "IRLSAFETYPlus.BlurStrength") == 0)
		return "Blur Strength";
	return val;
}

void obs_register_source_s(const struct obs_source_info *info, size_t size)
{
	(void)size;
	if (g_registered_source_count < OBS_MOCK_MAX_SOURCES)
		g_registered_sources[g_registered_source_count++] = *info;
}

void blogva(int log_level, const char *format, va_list args)
{
	(void)log_level;
	(void)format;
	(void)args;
}

void *bzalloc(size_t size)
{
	void *ptr = calloc(1, size);
	return ptr;
}

void bfree(void *ptr)
{
	free(ptr);
}

struct obs_data_entry {
	char name[64];
	double value;
	double default_value;
	bool has_default;
};

struct obs_data {
	struct obs_data_entry entries[16];
	size_t entry_count;
};

obs_data_t *obs_data_create(void)
{
	return calloc(1, sizeof(obs_data_t));
}

void obs_data_release(obs_data_t *data)
{
	free(data);
}

static struct obs_data_entry *obs_data_find_entry(obs_data_t *data, const char *name)
{
	for (size_t i = 0; i < data->entry_count; i++) {
		if (strcmp(data->entries[i].name, name) == 0)
			return &data->entries[i];
	}
	return NULL;
}

static struct obs_data_entry *obs_data_get_or_create_entry(obs_data_t *data, const char *name)
{
	struct obs_data_entry *entry = obs_data_find_entry(data, name);
	if (entry)
		return entry;

	if (data->entry_count >= 16)
		return NULL;

	entry = &data->entries[data->entry_count++];
	strncpy(entry->name, name, sizeof(entry->name) - 1);
	return entry;
}

double obs_data_get_double(obs_data_t *data, const char *name)
{
	struct obs_data_entry *entry = obs_data_find_entry(data, name);
	if (!entry)
		return 0.0;
	if (entry->value != 0.0)
		return entry->value;
	if (entry->has_default)
		return entry->default_value;
	return 0.0;
}

void obs_data_set_default_double(obs_data_t *data, const char *name, double val)
{
	struct obs_data_entry *entry = obs_data_get_or_create_entry(data, name);
	if (!entry)
		return;
	entry->default_value = val;
	entry->has_default = true;
	if (entry->value == 0.0)
		entry->value = val;
}

struct obs_properties {
	size_t property_count;
};

obs_properties_t *obs_properties_create(void)
{
	return calloc(1, sizeof(obs_properties_t));
}

void obs_properties_destroy(obs_properties_t *props)
{
	free(props);
}

obs_properties_t *obs_properties_add_text(obs_properties_t *props, const char *name, const char *desc,
					    enum obs_text_type type)
{
	(void)name;
	(void)type;
	if (desc)
		strncpy(g_last_property_label, desc, sizeof(g_last_property_label) - 1);
	if (props)
		props->property_count++;
	return props;
}

obs_properties_t *obs_properties_add_float(obs_properties_t *props, const char *name, const char *desc, double min,
					   double max, double step)
{
	(void)name;
	(void)min;
	(void)max;
	(void)step;
	if (desc)
		strncpy(g_last_property_label, desc, sizeof(g_last_property_label) - 1);
	if (props)
		props->property_count++;
	return props;
}