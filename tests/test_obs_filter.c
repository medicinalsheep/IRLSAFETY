/*
 * IRLSAFETY+ — tests OBS module load and filter_video entry points.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include <stdio.h>
#include <string.h>

#define TEST_ASSERT(cond)                                                                          \
	do {                                                                                       \
		if (!(cond)) {                                                                     \
			fprintf(stderr, "FAIL: %s (%s:%d)\n", #cond, __FILE__, __LINE__);          \
			return 1;                                                                  \
		}                                                                                  \
	} while (0)

#include <obs-module.h>

#include "filter_settings.h"
#include "frame_convert.h"
#include <pii-filter.h>

bool obs_module_load(void);

static int test_module_registers_filter(void)
{
	obs_mock_reset();

	TEST_ASSERT(obs_module_load() == true);
	TEST_ASSERT(obs_mock_registered_source_count() == 1);

	const struct obs_source_info *info = obs_mock_get_registered_source(0);
	TEST_ASSERT(info != NULL);
	TEST_ASSERT(strcmp(info->id, "irlsafety_plus_pii_blur") == 0);
	TEST_ASSERT(info->type == OBS_SOURCE_TYPE_FILTER);
	TEST_ASSERT((info->output_flags & OBS_SOURCE_VIDEO) != 0);
	TEST_ASSERT(info->get_defaults != NULL);
	TEST_ASSERT(info->get_properties != NULL);
	TEST_ASSERT(info->filter_video != NULL);
	TEST_ASSERT(info->video_render != NULL);
	TEST_ASSERT(info->video_tick != NULL);
	return 0;
}

static int test_filter_defaults_and_properties(void)
{
	const struct obs_source_info *info = obs_mock_get_registered_source(0);
	obs_data_t *settings = obs_data_create();

	info->get_defaults(settings);
	TEST_ASSERT(obs_data_get_bool(settings, IRLSAFETY_SET_ENABLE_ALL) == true);
	TEST_ASSERT(obs_data_get_double(settings, IRLSAFETY_SET_BLUR_STRENGTH) == 24.0);
	TEST_ASSERT(obs_data_get_int(settings, IRLSAFETY_SET_FRAME_SKIP) == 3);
	TEST_ASSERT(obs_data_get_bool(settings, IRLSAFETY_SET_CAT_STREET_SIGNS) == true);
	TEST_ASSERT(obs_data_get_bool(settings, IRLSAFETY_SET_CAT_LICENSE_PLATES) == true);

	obs_properties_t *props = info->get_properties(NULL);
	TEST_ASSERT(props != NULL);
	TEST_ASSERT(obs_mock_property_count() >= 10);

	obs_data_release(settings);
	return 0;
}

static int test_filter_video_multiplanar_i420(void)
{
	const struct obs_source_info *info = obs_mock_get_registered_source(0);
	obs_data_t *settings = obs_data_create();

	info->get_defaults(settings);
	void *filter = info->create(settings, NULL);
	TEST_ASSERT(filter != NULL);

	uint8_t y_plane[640 * 480];
	uint8_t u_plane[320 * 240];
	uint8_t v_plane[320 * 240];

	struct obs_source_frame frame = {0};
	frame.format = VIDEO_FORMAT_I420;
	frame.width = 640;
	frame.height = 480;
	frame.data[0] = y_plane;
	frame.linesize[0] = 640;
	frame.data[1] = u_plane;
	frame.linesize[1] = 320;
	frame.data[2] = v_plane;
	frame.linesize[2] = 320;

	struct obs_source_frame *out = info->filter_video(filter, &frame);
	TEST_ASSERT(out == &frame);

	info->video_tick(filter, 0.033f);

	info->destroy(filter);
	obs_data_release(settings);
	return 0;
}

static int test_frame_convert_yuy2(void)
{
	uint8_t yuy2_plane[16];
	struct obs_source_frame frame = {0};
	irlsafety_frame_view view;

	frame.format = VIDEO_FORMAT_YUY2;
	frame.width = 4;
	frame.height = 2;
	frame.data[0] = yuy2_plane;
	frame.linesize[0] = 8;

	TEST_ASSERT(irlsafety_frame_view_from_obs(&frame, &view) == 0);
	TEST_ASSERT(view.format == IRLSAFETY_FORMAT_YUY2);
	TEST_ASSERT(view.plane_count == 1);
	TEST_ASSERT(view.planes[0] == yuy2_plane);
	return 0;
}

static int test_frame_convert_planes(void)
{
	uint8_t y_plane[16];
	uint8_t u_plane[4];
	uint8_t v_plane[4];
	struct obs_source_frame frame = {0};
	irlsafety_frame_view view;

	frame.format = VIDEO_FORMAT_I420;
	frame.width = 4;
	frame.height = 4;
	frame.data[0] = y_plane;
	frame.linesize[0] = 4;
	frame.data[1] = u_plane;
	frame.linesize[1] = 2;
	frame.data[2] = v_plane;
	frame.linesize[2] = 2;

	TEST_ASSERT(irlsafety_frame_view_from_obs(&frame, &view) == 0);
	TEST_ASSERT(view.format == IRLSAFETY_FORMAT_I420);
	TEST_ASSERT(view.plane_count == 3);
	TEST_ASSERT(view.planes[0] == y_plane);
	TEST_ASSERT(view.planes[1] == u_plane);
	TEST_ASSERT(view.planes[2] == v_plane);
	TEST_ASSERT(view.linesize[0] == 4);
	return 0;
}

int main(void)
{
	if (test_module_registers_filter() != 0)
		return 1;
	if (test_filter_defaults_and_properties() != 0)
		return 1;
	if (test_filter_video_multiplanar_i420() != 0)
		return 1;
	if (test_frame_convert_planes() != 0)
		return 1;
	if (test_frame_convert_yuy2() != 0)
		return 1;
	printf("IRLSAFETY+ OBS filter entry-point tests passed.\n");
	return 0;
}