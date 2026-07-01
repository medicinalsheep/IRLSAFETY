/*
 * IRLSAFETY+ — minimal OBS API mock for unit tests.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "obs-mock.h"

#define OBS_MOCK_MAX_SOURCES 8
#define OBS_MOCK_MAX_BOOLS 32
#define OBS_MOCK_MAX_INTS 32
#define OBS_MOCK_MAX_DOUBLES 32
#define OBS_MOCK_MAX_STRINGS 16

static struct obs_source_info g_registered_sources[OBS_MOCK_MAX_SOURCES];
static size_t g_registered_source_count;
static char g_last_property_label[256];
static size_t g_property_count;
static int g_module_load_calls;

struct obs_bool_entry {
	char name[64];
	bool value;
	bool default_value;
	bool has_default;
};

struct obs_int_entry {
	char name[64];
	int value;
	int default_value;
	bool has_default;
	bool value_set;
};

struct obs_double_entry {
	char name[64];
	double value;
	double default_value;
	bool has_default;
	bool value_set;
};

struct obs_string_entry {
	char name[64];
	char value[4096];
	char default_value[4096];
	bool has_default;
	bool value_set;
};

struct obs_data {
	struct obs_bool_entry bools[OBS_MOCK_MAX_BOOLS];
	size_t bool_count;
	struct obs_int_entry ints[OBS_MOCK_MAX_INTS];
	size_t int_count;
	struct obs_double_entry doubles[OBS_MOCK_MAX_DOUBLES];
	size_t double_count;
	struct obs_string_entry strings[OBS_MOCK_MAX_STRINGS];
	size_t string_count;
};

struct obs_properties {
	size_t property_count;
};

void obs_mock_reset(void)
{
	g_registered_source_count = 0;
	g_last_property_label[0] = '\0';
	g_property_count = 0;
	g_module_load_calls = 0;
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

size_t obs_mock_property_count(void)
{
	return g_property_count;
}

int obs_mock_module_load_calls(void)
{
	return g_module_load_calls;
}

const char *obs_module_text(const char *val)
{
	if (strcmp(val, "IRLSAFETYPlus.FilterName") == 0)
		return "IRLSAFETY+ PII Blur";
	if (strcmp(val, "IRLSAFETYPlus.FilterDescription") == 0)
		return "Real-time PII detection and blur filter";
	if (strcmp(val, "IRLSAFETYPlus.BlurStrength") == 0)
		return "Blur Intensity";
	if (strcmp(val, "IRLSAFETYPlus.EnableAll") == 0)
		return "Enable All Protection";
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
	return calloc(1, size);
}

void bfree(void *ptr)
{
	free(ptr);
}

obs_data_t *obs_data_create(void)
{
	return calloc(1, sizeof(obs_data_t));
}

void obs_data_release(obs_data_t *data)
{
	free(data);
}

static void track_property_label(const char *desc, obs_properties_t *props)
{
	if (desc)
		strncpy(g_last_property_label, desc, sizeof(g_last_property_label) - 1);
	if (props)
		props->property_count++;
	g_property_count++;
}

static struct obs_bool_entry *find_bool(obs_data_t *data, const char *name)
{
	for (size_t i = 0; i < data->bool_count; i++) {
		if (strcmp(data->bools[i].name, name) == 0)
			return &data->bools[i];
	}
	return NULL;
}

static struct obs_bool_entry *get_or_create_bool(obs_data_t *data, const char *name)
{
	struct obs_bool_entry *entry = find_bool(data, name);
	if (entry)
		return entry;
	if (data->bool_count >= OBS_MOCK_MAX_BOOLS)
		return NULL;
	entry = &data->bools[data->bool_count++];
	strncpy(entry->name, name, sizeof(entry->name) - 1);
	return entry;
}

static struct obs_int_entry *find_int(obs_data_t *data, const char *name)
{
	for (size_t i = 0; i < data->int_count; i++) {
		if (strcmp(data->ints[i].name, name) == 0)
			return &data->ints[i];
	}
	return NULL;
}

static struct obs_int_entry *get_or_create_int(obs_data_t *data, const char *name)
{
	struct obs_int_entry *entry = find_int(data, name);
	if (entry)
		return entry;
	if (data->int_count >= OBS_MOCK_MAX_INTS)
		return NULL;
	entry = &data->ints[data->int_count++];
	strncpy(entry->name, name, sizeof(entry->name) - 1);
	return entry;
}

static struct obs_double_entry *find_double(obs_data_t *data, const char *name)
{
	for (size_t i = 0; i < data->double_count; i++) {
		if (strcmp(data->doubles[i].name, name) == 0)
			return &data->doubles[i];
	}
	return NULL;
}

static struct obs_double_entry *get_or_create_double(obs_data_t *data, const char *name)
{
	struct obs_double_entry *entry = find_double(data, name);
	if (entry)
		return entry;
	if (data->double_count >= OBS_MOCK_MAX_DOUBLES)
		return NULL;
	entry = &data->doubles[data->double_count++];
	strncpy(entry->name, name, sizeof(entry->name) - 1);
	return entry;
}

static struct obs_string_entry *find_string(obs_data_t *data, const char *name)
{
	for (size_t i = 0; i < data->string_count; i++) {
		if (strcmp(data->strings[i].name, name) == 0)
			return &data->strings[i];
	}
	return NULL;
}

static struct obs_string_entry *get_or_create_string(obs_data_t *data, const char *name)
{
	struct obs_string_entry *entry = find_string(data, name);
	if (entry)
		return entry;
	if (data->string_count >= OBS_MOCK_MAX_STRINGS)
		return NULL;
	entry = &data->strings[data->string_count++];
	strncpy(entry->name, name, sizeof(entry->name) - 1);
	return entry;
}

bool obs_data_get_bool(obs_data_t *data, const char *name)
{
	struct obs_bool_entry *entry = find_bool(data, name);
	if (!entry)
		return false;
	if (entry->has_default && !entry->value && entry->default_value)
		return entry->default_value;
	return entry->value;
}

void obs_data_set_default_bool(obs_data_t *data, const char *name, bool val)
{
	struct obs_bool_entry *entry = get_or_create_bool(data, name);
	if (!entry)
		return;
	entry->default_value = val;
	entry->has_default = true;
	entry->value = val;
}

int obs_data_get_int(obs_data_t *data, const char *name)
{
	struct obs_int_entry *entry = find_int(data, name);
	if (!entry)
		return 0;
	if (!entry->value_set && entry->has_default)
		return entry->default_value;
	return entry->value;
}

void obs_data_set_default_int(obs_data_t *data, const char *name, int val)
{
	struct obs_int_entry *entry = get_or_create_int(data, name);
	if (!entry)
		return;
	entry->default_value = val;
	entry->has_default = true;
	entry->value = val;
	entry->value_set = true;
}

double obs_data_get_double(obs_data_t *data, const char *name)
{
	struct obs_double_entry *entry = find_double(data, name);
	if (!entry)
		return 0.0;
	if (!entry->value_set && entry->has_default)
		return entry->default_value;
	return entry->value;
}

void obs_data_set_default_double(obs_data_t *data, const char *name, double val)
{
	struct obs_double_entry *entry = get_or_create_double(data, name);
	if (!entry)
		return;
	entry->default_value = val;
	entry->has_default = true;
	entry->value = val;
	entry->value_set = true;
}

const char *obs_data_get_string(obs_data_t *data, const char *name)
{
	struct obs_string_entry *entry = find_string(data, name);
	if (!entry)
		return "";
	if (!entry->value_set && entry->has_default)
		return entry->default_value;
	return entry->value;
}

void obs_data_set_default_string(obs_data_t *data, const char *name, const char *val)
{
	struct obs_string_entry *entry = get_or_create_string(data, name);
	if (!entry)
		return;
	strncpy(entry->default_value, val ? val : "", sizeof(entry->default_value) - 1);
	entry->has_default = true;
	strncpy(entry->value, val ? val : "", sizeof(entry->value) - 1);
	entry->value_set = true;
}

obs_properties_t *obs_properties_create(void)
{
	return calloc(1, sizeof(obs_properties_t));
}

obs_properties_t *obs_properties_add_group(obs_properties_t *props, const char *name, const char *desc,
					   enum obs_group_type type, obs_properties_t *group)
{
	(void)name;
	(void)type;
	(void)group;
	track_property_label(desc, props);
	return props;
}

obs_properties_t *obs_properties_add_text(obs_properties_t *props, const char *name, const char *desc,
					    enum obs_text_type type)
{
	(void)name;
	(void)type;
	track_property_label(desc, props);
	return props;
}

obs_properties_t *obs_properties_add_bool(obs_properties_t *props, const char *name, const char *desc)
{
	(void)name;
	track_property_label(desc, props);
	return props;
}

obs_properties_t *obs_properties_add_float(obs_properties_t *props, const char *name, const char *desc, double min,
					   double max, double step)
{
	(void)name;
	(void)min;
	(void)max;
	(void)step;
	track_property_label(desc, props);
	return props;
}

obs_properties_t *obs_properties_add_float_slider(obs_properties_t *props, const char *name, const char *desc,
						    double min, double max, double step)
{
	return obs_properties_add_float(props, name, desc, min, max, step);
}

obs_properties_t *obs_properties_add_int_slider(obs_properties_t *props, const char *name, const char *desc, int min,
						  int max, int step)
{
	(void)name;
	(void)min;
	(void)max;
	(void)step;
	track_property_label(desc, props);
	return props;
}

obs_properties_t *obs_properties_add_path(obs_properties_t *props, const char *name, const char *desc,
					    enum obs_path_type type, const char *filter, const char *default_path)
{
	(void)name;
	(void)type;
	(void)filter;
	(void)default_path;
	track_property_label(desc, props);
	return props;
}