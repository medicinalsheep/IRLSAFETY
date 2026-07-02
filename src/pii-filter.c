/*
 * IRLSAFETY+ — OBS video filter for PII blurring.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "pii-filter.h"

#include "frame_convert.h"
#include "filter_settings.h"
#include "irlsafety_control.h"
#include "ocr/ocr_frame_util.h"
#include "pipeline.h"
#ifndef IRLSAFETY_TEST_BUILD
#include "filter_property_ui.h"
#endif

#ifndef IRLSAFETY_TEST_BUILD
#include "gpu_frame.h"
#endif

#include <plugin-support.h>
#include <stdlib.h>

#ifndef IRLSAFETY_TEST_BUILD
#include <util/platform.h>
#endif

struct pii_filter_data {
	obs_source_t *context;
	irlsafety_pipeline *pipeline;
#ifndef IRLSAFETY_TEST_BUILD
	irlsafety_gpu_frame *gpu;
#endif
	irlsafety_filter_settings settings;
	uint64_t frame_count;
};

static void pii_filter_destroy(void *data);
static void pii_filter_apply_settings(struct pii_filter_data *filter, obs_data_t *settings);

#ifndef IRLSAFETY_TEST_BUILD
static struct pii_filter_data *pii_filter_from_source(obs_source_t *source)
{
	if (!source)
		return NULL;

	return (struct pii_filter_data *)obs_obj_get_data(source);
}

int irlsafety_filter_get_runtime_status(obs_source_t *filter, irlsafety_runtime_status *out)
{
	struct pii_filter_data *data;

	if (!filter || !out)
		return -1;

	data = pii_filter_from_source(filter);
	if (!data || !data->pipeline)
		return -1;

	irlsafety_pipeline_get_runtime_status(data->pipeline, out, data->frame_count);
	if (out->model_path[0] != '\0')
		out->model_file_exists = os_file_exists(out->model_path);
	else
		out->model_file_exists = false;
	return 0;
}

int irlsafety_filter_reload_model(obs_source_t *filter)
{
	struct pii_filter_data *data;
	obs_data_t *settings;

	if (!filter)
		return -1;

	data = pii_filter_from_source(filter);
	if (!data || !data->pipeline)
		return -1;

	settings = obs_source_get_settings(filter);
	if (!settings)
		return -1;

	pii_filter_apply_settings(data, settings);
	obs_data_release(settings);
	return 0;
}
#else
int irlsafety_filter_get_runtime_status(obs_source_t *filter, irlsafety_runtime_status *out)
{
	UNUSED_PARAMETER(filter);
	UNUSED_PARAMETER(out);
	return -1;
}

int irlsafety_filter_reload_model(obs_source_t *filter)
{
	UNUSED_PARAMETER(filter);
	return -1;
}
#endif

static bool pii_filter_target_is_heavy(struct pii_filter_data *filter)
{
#ifndef IRLSAFETY_TEST_BUILD
	obs_source_t *target;

	if (!filter || !filter->context)
		return false;

	target = obs_filter_get_target(filter->context);
	if (!target)
		return false;

	return (obs_source_get_output_flags(target) & OBS_SOURCE_ASYNC) != 0;
#else
	UNUSED_PARAMETER(filter);
	return false;
#endif
}

static void pii_filter_resolve_model_path(char *dest, size_t dest_size, const char *user_path)
{
	if (!dest || dest_size == 0)
		return;

	dest[0] = '\0';

	if (user_path && user_path[0] != '\0') {
		strncpy(dest, user_path, dest_size - 1);
		dest[dest_size - 1] = '\0';
		return;
	}

#ifndef IRLSAFETY_TEST_BUILD
	{
		char *bundled = obs_module_file("models/irlsafety-detect.onnx");

		if (bundled) {
			strncpy(dest, bundled, dest_size - 1);
			dest[dest_size - 1] = '\0';
			bfree(bundled);
		}
	}
#else
	UNUSED_PARAMETER(user_path);
#endif
}

static void pii_filter_apply_settings(struct pii_filter_data *filter, obs_data_t *settings)
{
	if (!filter)
		return;

	char user_model_path[sizeof(filter->settings.model_path)];

	irlsafety_filter_settings_load(settings, &filter->settings);
	strncpy(user_model_path, filter->settings.model_path, sizeof(user_model_path) - 1);
	user_model_path[sizeof(user_model_path) - 1] = '\0';
	pii_filter_resolve_model_path(filter->settings.model_path, sizeof(filter->settings.model_path),
				      user_model_path[0] != '\0' ? user_model_path : NULL);
	if (filter->pipeline)
		irlsafety_pipeline_update_settings(filter->pipeline, &filter->settings);
}

static bool pii_filter_censor_mode_modified(obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	int mode = IRLSAFETY_CENSOR_BOX;

	if (settings)
		mode = (int)obs_data_get_int(settings, IRLSAFETY_SET_CENSOR_MODE);
	obs_property_t *color_prop = obs_properties_get(props, IRLSAFETY_SET_CENSOR_COLOR);
	obs_property_t *blur_prop = obs_properties_get(props, IRLSAFETY_SET_BLUR_STRENGTH);

	UNUSED_PARAMETER(property);

	if (color_prop)
		obs_property_set_visible(color_prop, mode != IRLSAFETY_CENSOR_BLUR);
	if (blur_prop)
		obs_property_set_visible(blur_prop, mode == IRLSAFETY_CENSOR_BLUR);
	return true;
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

	filter = bzalloc(sizeof(*filter));
	filter->context = source;
	filter->pipeline = irlsafety_pipeline_create();
	if (!filter->pipeline) {
		pii_filter_destroy(filter);
		return NULL;
	}

#ifndef IRLSAFETY_TEST_BUILD
	obs_enter_graphics();
	filter->gpu = irlsafety_gpu_frame_create(source);
	obs_leave_graphics();

	if (!filter->gpu) {
		pii_filter_destroy(filter);
		return NULL;
	}
#endif

	pii_filter_apply_settings(filter, settings);
	return filter;
}

static void pii_filter_destroy(void *data)
{
	struct pii_filter_data *filter = data;
	if (!filter)
		return;

#ifndef IRLSAFETY_TEST_BUILD
	obs_enter_graphics();
	irlsafety_gpu_frame_destroy(filter->gpu);
	obs_leave_graphics();
#endif

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
	obs_property_t *mode_prop;

	UNUSED_PARAMETER(unused);

	props = obs_properties_create();
	obs_properties_add_text(props, "info", obs_module_text("IRLSAFETYPlus.FilterDescription"), OBS_TEXT_INFO);

	obs_properties_add_bool(props, IRLSAFETY_SET_ENABLE_ALL, obs_module_text("IRLSAFETYPlus.EnableAll"));
	obs_properties_add_bool(props, IRLSAFETY_SET_TEST_EFFECT, obs_module_text("IRLSAFETYPlus.TestEffect"));

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

	custom = obs_properties_create();
	obs_properties_add_text(custom, "custom_pii_hint", obs_module_text("IRLSAFETYPlus.CustomPiiHint"), OBS_TEXT_INFO);
	obs_properties_add_text(custom, IRLSAFETY_SET_CUSTOM_PII_INLINE,
				obs_module_text("IRLSAFETYPlus.CustomPiiInline"), OBS_TEXT_MULTILINE);
	obs_properties_add_path(custom, IRLSAFETY_SET_CUSTOM_PII_FILE, obs_module_text("IRLSAFETYPlus.CustomPiiFile"),
			      OBS_PATH_FILE, "Text files (*.txt)", NULL);
	obs_properties_add_group(props, "custom_pii", obs_module_text("IRLSAFETYPlus.GroupCustomPii"), OBS_GROUP_NORMAL,
				 custom);

	protection = obs_properties_create();
	obs_properties_add_float_slider(protection, IRLSAFETY_SET_CONFIDENCE,
					obs_module_text("IRLSAFETYPlus.Confidence"), 0.1, 0.95, 0.05);
	obs_properties_add_int_slider(protection, IRLSAFETY_SET_FRAME_SKIP, obs_module_text("IRLSAFETYPlus.FrameSkip"),
				      1, 120, 1);
	{
		obs_property_t *detail_prop = obs_properties_add_list(
			protection, IRLSAFETY_SET_OCR_DETAIL, obs_module_text("IRLSAFETYPlus.OcrDetail"),
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
		obs_property_list_add_int(detail_prop, obs_module_text("IRLSAFETYPlus.OcrDetailStandard"), 0);
		obs_property_list_add_int(detail_prop, obs_module_text("IRLSAFETYPlus.OcrDetailDetailed"), 1);
		obs_property_list_add_int(detail_prop, obs_module_text("IRLSAFETYPlus.OcrDetailMaximum"), 2);
	}
	mode_prop = obs_properties_add_list(protection, IRLSAFETY_SET_CENSOR_MODE,
					    obs_module_text("IRLSAFETYPlus.CensorMode"), OBS_COMBO_TYPE_LIST,
					    OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(mode_prop, obs_module_text("IRLSAFETYPlus.CensorModeBlur"), IRLSAFETY_CENSOR_BLUR);
	obs_property_list_add_int(mode_prop, obs_module_text("IRLSAFETYPlus.CensorModeBox"), IRLSAFETY_CENSOR_BOX);
	obs_property_list_add_int(mode_prop, obs_module_text("IRLSAFETYPlus.CensorModeEllipse"), IRLSAFETY_CENSOR_ELLIPSE);
	obs_property_list_add_int(mode_prop, obs_module_text("IRLSAFETYPlus.CensorModeCloud"), IRLSAFETY_CENSOR_CLOUD);
	obs_property_set_modified_callback(mode_prop, pii_filter_censor_mode_modified);
	obs_properties_add_color_alpha(protection, IRLSAFETY_SET_CENSOR_COLOR,
				       obs_module_text("IRLSAFETYPlus.CensorColor"));
	obs_properties_add_float_slider(protection, IRLSAFETY_SET_BLUR_STRENGTH,
					obs_module_text("IRLSAFETYPlus.BlurStrength"), 1.0, 32.0, 1.0);
	obs_properties_add_float_slider(protection, IRLSAFETY_SET_OVERLAY_OVERLAP,
					obs_module_text("IRLSAFETYPlus.OverlayOverlap"), 0.0, 0.80, 0.05);
	obs_properties_add_bool(protection, IRLSAFETY_SET_HYBRID_DELAY_ENABLE,
				obs_module_text("IRLSAFETYPlus.HybridDelayEnable"));
	obs_properties_add_float_slider(protection, IRLSAFETY_SET_GLOBAL_DELAY_SEC,
					obs_module_text("IRLSAFETYPlus.GlobalDelaySec"), 0.0, 5.0, 0.1);
	obs_properties_add_float_slider(protection, IRLSAFETY_SET_AUTO_DELAY_SEC,
					obs_module_text("IRLSAFETYPlus.AutoDelaySec"), 0.0, 5.0, 0.1);
	obs_properties_add_float_slider(protection, IRLSAFETY_SET_AUTO_DELAY_HOLD_SEC,
					obs_module_text("IRLSAFETYPlus.AutoDelayHoldSec"), 0.0, 15.0, 0.5);
	obs_properties_add_float_slider(protection, IRLSAFETY_SET_PARTIAL_PII_THRESHOLD,
					obs_module_text("IRLSAFETYPlus.PartialPiiThreshold"), 0.0, 1.0, 0.05);
	obs_properties_add_bool(protection, IRLSAFETY_SET_SECURE_MODE_ENABLE,
				obs_module_text("IRLSAFETYPlus.SecureModeEnable"));
	obs_properties_add_bool(protection, IRLSAFETY_SET_SECURE_DROP_FRAMES,
				obs_module_text("IRLSAFETYPlus.SecureDropFrames"));
	obs_properties_add_bool(protection, IRLSAFETY_SET_COVER_WHILE_TYPING,
				obs_module_text("IRLSAFETYPlus.CoverWhileTyping"));
	obs_properties_add_text(protection, "hybrid_delay_audio_note",
				obs_module_text("IRLSAFETYPlus.HybridDelayAudioNote"), OBS_TEXT_INFO);
	obs_properties_add_group(props, "protection", obs_module_text("IRLSAFETYPlus.GroupProtection"),
				 OBS_GROUP_NORMAL, protection);

	advanced = obs_properties_create();
	obs_properties_add_bool(advanced, IRLSAFETY_SET_PREFER_GPU, obs_module_text("IRLSAFETYPlus.PreferGpu"));
	obs_properties_add_bool(advanced, IRLSAFETY_SET_SHOW_PREVIEW, obs_module_text("IRLSAFETYPlus.ShowPreview"));
	obs_properties_add_bool(advanced, IRLSAFETY_SET_ENABLE_LOGGING, obs_module_text("IRLSAFETYPlus.EnableLogging"));
	obs_properties_add_path(advanced, IRLSAFETY_SET_MODEL_PATH, obs_module_text("IRLSAFETYPlus.ModelPath"),
			      OBS_PATH_FILE, "ONNX models (*.onnx)", NULL);
	obs_properties_add_group(props, "advanced", obs_module_text("IRLSAFETYPlus.GroupAdvanced"), OBS_GROUP_NORMAL,
				 advanced);

	pii_filter_censor_mode_modified(props, mode_prop, NULL);
#ifndef IRLSAFETY_TEST_BUILD
	irlsafety_filter_apply_property_tooltips(props, categories, custom, protection, advanced);
#endif
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

	if (!filter)
		return;

	filter->frame_count++;
	if (filter->pipeline)
		irlsafety_pipeline_tick(filter->pipeline, filter->frame_count, &filter->settings);
}

static struct obs_source_frame *pii_filter_video(void *data, struct obs_source_frame *frame)
{
	struct pii_filter_data *filter = data;
	irlsafety_frame_view view;
	irlsafety_region_list overlays;

	if (!filter || !filter->pipeline || !frame)
		return frame;

	if (!filter->settings.enable_all)
		return frame;

	if (irlsafety_frame_view_from_obs(frame, &view) != 0)
		return frame;

	irlsafety_pipeline_poll_detection(filter->pipeline, &filter->settings, view.width, view.height);

	if ((irlsafety_pipeline_needs_urgent_scan(filter->pipeline) ||
	     irlsafety_filter_should_run_detection(&filter->settings, filter->frame_count,
						   pii_filter_target_is_heavy(filter))) &&
	    !irlsafety_pipeline_ocr_busy(filter->pipeline))
		irlsafety_pipeline_submit_detection(filter->pipeline, &view, &filter->settings, filter->frame_count,
						    view.width, view.height);

	if (irlsafety_pipeline_should_drop_frame(filter->pipeline, &filter->settings))
		return NULL;

	irlsafety_pipeline_get_overlays(filter->pipeline, view.width, view.height, &filter->settings, &overlays);
	if (overlays.count > 0)
		irlsafety_pipeline_apply_cpu_censor(filter->pipeline, &view, &filter->settings, view.width, view.height);

	return frame;
}

#ifndef IRLSAFETY_TEST_BUILD
static enum gs_color_space pii_filter_get_color_space(void *data, size_t count,
						      const enum gs_color_space *preferred_spaces)
{
	struct pii_filter_data *filter = data;
	const enum gs_color_space preferred[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	if (!filter || !filter->context)
		return (count > 0) ? preferred_spaces[0] : GS_CS_SRGB;

	return obs_source_get_color_space(obs_filter_get_target(filter->context), OBS_COUNTOF(preferred), preferred);
}
#endif

#ifdef IRLSAFETY_TEST_BUILD
static void pii_filter_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(effect);
}
#else
static void pii_filter_video_render(void *data, gs_effect_t *effect)
{
	struct pii_filter_data *filter = data;
	obs_source_t *target;
	uint32_t target_flags;
	uint32_t cx;
	uint32_t cy;
	irlsafety_frame_view view;
	irlsafety_region_list overlays;
	bool needs_cpu_replace;

	UNUSED_PARAMETER(effect);

	if (!filter || !filter->context)
		return;

	if (!filter->settings.enable_all) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	target = obs_filter_get_target(filter->context);
	if (!target) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	target_flags = obs_source_get_output_flags(target);
	if (target_flags & OBS_SOURCE_ASYNC) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	if (!irlsafety_gpu_frame_begin_frame(filter->gpu, &cx, &cy)) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	irlsafety_pipeline_poll_detection(filter->pipeline, &filter->settings, cx, cy);

	if ((irlsafety_pipeline_needs_urgent_scan(filter->pipeline) ||
	     irlsafety_filter_should_run_detection(&filter->settings, filter->frame_count, false)) &&
	    !irlsafety_pipeline_ocr_busy(filter->pipeline)) {
		uint32_t ocr_max_width = irlsafety_ocr_max_width_for_detail(filter->settings.ocr_detail);
		uint32_t readback_width = filter->settings.ocr_detail == 2 ? 0 : ocr_max_width;

		if (irlsafety_gpu_frame_readback_ocr(filter->gpu, &view, cx, cy, readback_width) == 0) {
			irlsafety_pipeline_submit_detection(filter->pipeline, &view, &filter->settings,
							  filter->frame_count, cx, cy);
		} else if (filter->settings.enable_logging) {
			obs_log(LOG_WARNING, "IRLSAFETY+: GPU OCR readback failed (%ux%u)", cx, cy);
		}
	}

	if (irlsafety_pipeline_should_drop_frame(filter->pipeline, &filter->settings)) {
		irlsafety_gpu_frame_draw_fullscreen_censor(filter->context, &filter->settings, cx, cy);
		return;
	}

	irlsafety_pipeline_get_overlays(filter->pipeline, cx, cy, &filter->settings, &overlays);

	needs_cpu_replace = overlays.count > 0 &&
			    (filter->settings.censor_mode == IRLSAFETY_CENSOR_BLUR ||
			     filter->settings.censor_mode == IRLSAFETY_CENSOR_ELLIPSE ||
			     filter->settings.censor_mode == IRLSAFETY_CENSOR_CLOUD);

	if (needs_cpu_replace) {
		if (irlsafety_gpu_frame_render_cpu_censor(filter->gpu, filter->pipeline, &filter->settings, cx, cy) == 0)
			return;
		obs_source_skip_video_filter(filter->context);
		return;
	}

	if (!irlsafety_gpu_frame_draw_captured(filter->gpu)) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	if (overlays.count > 0)
		irlsafety_gpu_frame_draw_overlays(filter->context, &overlays, &filter->settings);
}
#endif /* IRLSAFETY_TEST_BUILD */

struct obs_source_info irlsafety_pii_filter = {
	.id = IRLSAFETY_PII_FILTER_ID,
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = pii_filter_get_name,
	.create = pii_filter_create,
	.destroy = pii_filter_destroy,
	.get_defaults = pii_filter_defaults,
	.get_properties = pii_filter_properties,
	.update = pii_filter_update,
	.video_tick = pii_filter_video_tick,
#ifndef IRLSAFETY_TEST_BUILD
	.video_get_color_space = pii_filter_get_color_space,
#endif
	.video_render = pii_filter_video_render,
	.filter_video = pii_filter_video,
};