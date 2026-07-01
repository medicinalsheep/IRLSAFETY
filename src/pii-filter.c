/*
 * IRLSAFETY+ — OBS video filter for PII blurring.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "pii-filter.h"

#include "frame_convert.h"
#include "filter_settings.h"
#include "pipeline.h"

#include <plugin-support.h>
#include <stdlib.h>

#define IRLSAFETY_FILTER_ID "irlsafety_plus_pii_blur"

struct pii_filter_data {
	irlsafety_pipeline *pipeline;
	irlsafety_filter_settings settings;
	uint64_t frame_count;
};

static void pii_filter_apply_settings(struct pii_filter_data *filter, obs_data_t *settings)
{
	if (!filter)
		return;

	irlsafety_filter_settings_load(settings, &filter->settings);
	if (filter->pipeline)
		irlsafety_pipeline_update_settings(filter->pipeline, &filter->settings);
}

static const char *pii_filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("IRLSAFETYPlus.FilterName");
}

static void pii_filter_defaults(obs_data_t *settings)
{
	irlsafety_filter_settings_set_defaults(settings);
}

static void *pii_filter_create(obs_data_t *settings, obs_source_t *source)
{
	struct pii_filter_data *filter;

	UNUSED_PARAMETER(source);

	filter = bzalloc(sizeof(*filter));
	filter->pipeline = irlsafety_pipeline_create(NULL);
	pii_filter_apply_settings(filter, settings);
	return filter;
}

static void pii_filter_destroy(void *data)
{
	struct pii_filter_data *filter = data;
	if (!filter)
		return;

	irlsafety_pipeline_destroy(filter->pipeline);
	bfree(filter);
}

static obs_properties_t *pii_filter_properties(void *unused)
{
	obs_properties_t *props;
	obs_properties_t *protection;
	obs_properties_t *categories;
	obs_properties_t *custom;
	obs_properties_t *advanced;

	UNUSED_PARAMETER(unused);

	props = obs_properties_create();
	obs_properties_add_text(props, "info", obs_module_text("IRLSAFETYPlus.FilterDescription"), OBS_TEXT_INFO);

	/* Master protection toggle */
	obs_properties_add_bool(props, IRLSAFETY_SET_ENABLE_ALL, obs_module_text("IRLSAFETYPlus.EnableAll"));

	/* Category toggles */
	categories = obs_properties_create();
	obs_properties_add_bool(categories, IRLSAFETY_SET_CAT_STREET_SIGNS,
				obs_module_text("IRLSAFETYPlus.CatStreetSigns"));
	obs_properties_add_bool(categories, IRLSAFETY_SET_CAT_LICENSE_PLATES,
				obs_module_text("IRLSAFETYPlus.CatLicensePlates"));
	obs_properties_add_bool(categories, IRLSAFETY_SET_CAT_DOCUMENTS,
				obs_module_text("IRLSAFETYPlus.CatDocuments"));
	obs_properties_add_bool(categories, IRLSAFETY_SET_CAT_FACES, obs_module_text("IRLSAFETYPlus.CatFaces"));
	obs_properties_add_bool(categories, IRLSAFETY_SET_CAT_SCREEN_TEXT,
				obs_module_text("IRLSAFETYPlus.CatScreenText"));
	obs_properties_add_bool(categories, IRLSAFETY_SET_CAT_CUSTOM_PII,
				obs_module_text("IRLSAFETYPlus.CatCustomPii"));
	obs_properties_add_group(props, "categories", obs_module_text("IRLSAFETYPlus.GroupCategories"),
				 OBS_GROUP_NORMAL, categories);

	/* Custom PII input */
	custom = obs_properties_create();
	obs_properties_add_text(custom, IRLSAFETY_SET_CUSTOM_PII_INLINE,
				obs_module_text("IRLSAFETYPlus.CustomPiiInline"), OBS_TEXT_MULTILINE);
	obs_properties_add_path(custom, IRLSAFETY_SET_CUSTOM_PII_FILE, obs_module_text("IRLSAFETYPlus.CustomPiiFile"),
			      OBS_PATH_FILE, "Text files (*.txt)", NULL);
	obs_properties_add_group(props, "custom_pii", obs_module_text("IRLSAFETYPlus.GroupCustomPii"), OBS_GROUP_NORMAL,
				 custom);

	/* Detection + blur tuning */
	protection = obs_properties_create();
	obs_properties_add_float_slider(protection, IRLSAFETY_SET_CONFIDENCE,
					obs_module_text("IRLSAFETYPlus.Confidence"), 0.1, 0.95, 0.05);
	obs_properties_add_int_slider(protection, IRLSAFETY_SET_FRAME_SKIP, obs_module_text("IRLSAFETYPlus.FrameSkip"),
				      1, 10, 1);
	obs_properties_add_float_slider(protection, IRLSAFETY_SET_BLUR_STRENGTH,
					obs_module_text("IRLSAFETYPlus.BlurStrength"), 1.0, 32.0, 1.0);
	obs_properties_add_group(props, "protection", obs_module_text("IRLSAFETYPlus.GroupProtection"),
				 OBS_GROUP_NORMAL, protection);

	/* Advanced */
	advanced = obs_properties_create();
	obs_properties_add_bool(advanced, IRLSAFETY_SET_PREFER_GPU, obs_module_text("IRLSAFETYPlus.PreferGpu"));
	obs_properties_add_bool(advanced, IRLSAFETY_SET_SHOW_PREVIEW, obs_module_text("IRLSAFETYPlus.ShowPreview"));
	obs_properties_add_bool(advanced, IRLSAFETY_SET_ENABLE_LOGGING, obs_module_text("IRLSAFETYPlus.EnableLogging"));
	obs_properties_add_path(advanced, IRLSAFETY_SET_MODEL_PATH, obs_module_text("IRLSAFETYPlus.ModelPath"),
			      OBS_PATH_FILE, "ONNX models (*.onnx)", NULL);
	obs_properties_add_group(props, "advanced", obs_module_text("IRLSAFETYPlus.GroupAdvanced"), OBS_GROUP_NORMAL,
				 advanced);

	return props;
}

static void pii_filter_update(void *data, obs_data_t *settings)
{
	pii_filter_apply_settings(data, settings);
}

static void pii_filter_video_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);
	struct pii_filter_data *filter = data;
	if (filter)
		filter->frame_count++;
}

static struct obs_source_frame *pii_filter_video(void *data, struct obs_source_frame *frame)
{
	struct pii_filter_data *filter = data;
	irlsafety_frame_view view;

	if (!filter || !filter->pipeline || !frame)
		return frame;

	if (!irlsafety_filter_should_process_frame(&filter->settings, filter->frame_count))
		return frame;

	if (irlsafety_frame_view_from_obs(frame, &view) != 0)
		return frame;

	irlsafety_pipeline_process_frame(filter->pipeline, &view, &filter->settings);
	return frame;
}

struct obs_source_info irlsafety_pii_filter = {
	.id = IRLSAFETY_FILTER_ID,
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = pii_filter_get_name,
	.create = pii_filter_create,
	.destroy = pii_filter_destroy,
	.get_defaults = pii_filter_defaults,
	.get_properties = pii_filter_properties,
	.update = pii_filter_update,
	.video_tick = pii_filter_video_tick,
	.filter_video = pii_filter_video,
};