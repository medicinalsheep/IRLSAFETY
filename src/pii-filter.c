/*
 * IRLSAFETY+ — OBS video filter for PII blurring.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "pii-filter.h"

#include "pipeline.h"

#include <stdlib.h>

#define IRLSAFETY_FILTER_ID "irlsafety_plus_pii_blur"

struct pii_filter_data {
	irlsafety_pipeline *pipeline;
	float blur_strength;
	uint64_t frame_count;
};

static const char *pii_filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("IRLSAFETYPlus.FilterName");
}

static void *pii_filter_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(source);

	struct pii_filter_data *filter = bzalloc(sizeof(*filter));
	filter->blur_strength = (float)obs_data_get_double(settings, "blur_strength");
	if (filter->blur_strength <= 0.0f)
		filter->blur_strength = 8.0f;

	/* Model path will be configurable via filter properties in a future revision. */
	filter->pipeline = irlsafety_pipeline_create(NULL);
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
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();
	obs_properties_add_float(props, "blur_strength", obs_module_text("IRLSAFETYPlus.FilterName"), 1.0, 32.0, 1.0);
	return props;
}

static void pii_filter_update(void *data, obs_data_t *settings)
{
	struct pii_filter_data *filter = data;
	filter->blur_strength = (float)obs_data_get_double(settings, "blur_strength");
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
	if (!filter || !filter->pipeline || !frame)
		return frame;

	irlsafety_frame_view view = {
		.data = frame->data[0],
		.width = frame->width,
		.height = frame->height,
		.linesize = frame->linesize[0],
	};

	irlsafety_pipeline_process_frame(filter->pipeline, &view, filter->blur_strength);
	return frame;
}

struct obs_source_info irlsafety_pii_filter = {
	.id = IRLSAFETY_FILTER_ID,
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = pii_filter_get_name,
	.create = pii_filter_create,
	.destroy = pii_filter_destroy,
	.get_properties = pii_filter_properties,
	.update = pii_filter_update,
	.video_tick = pii_filter_video_tick,
	.filter_video = pii_filter_video,
};