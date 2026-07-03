/*
 * IRLSAFETY+ — GPU texture capture, CPU pipeline, and redraw.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "gpu_frame.h"

#include "blur/overlay_image.h"
#include "irlsafety_geometry.h"
#include "ocr/ocr_frame_util.h"

#include <obs-module.h>
#include <plugin-support.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct irlsafety_gpu_frame {
	obs_source_t *filter;
	gs_texrender_t *capture;
	gs_texrender_t *ocr_scale;
	gs_stagesurf_t *stage;
	gs_stagesurf_t *ocr_stage;
	gs_texture_t *output;
	uint8_t *cpu_buffer;
	size_t cpu_buffer_size;
	uint32_t width;
	uint32_t height;
	uint32_t linesize;
	uint32_t ocr_stage_w;
	uint32_t ocr_stage_h;
	enum gs_color_format format;
	enum gs_color_space color_space;
	bool captured;
	gs_texture_t *overlay_tex;
	char overlay_tex_key[1024];
};

static uint32_t filter_pixel_width(obs_source_t *filter)
{
	obs_source_t *target = obs_filter_get_target(filter);
	uint32_t w;

	if (!target)
		return 0;

	w = obs_source_get_base_width(target);
	if (w == 0)
		w = obs_source_get_width(target);
	return w;
}

static uint32_t filter_pixel_height(obs_source_t *filter)
{
	obs_source_t *target = obs_filter_get_target(filter);
	uint32_t h;

	if (!target)
		return 0;

	h = obs_source_get_base_height(target);
	if (h == 0)
		h = obs_source_get_height(target);
	return h;
}

static enum gs_color_space filter_color_space(obs_source_t *filter)
{
	const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	return obs_source_get_color_space(obs_filter_get_target(filter), OBS_COUNTOF(preferred_spaces),
					  preferred_spaces);
}

static irlsafety_video_format view_format_for_gs(enum gs_color_format format)
{
	switch (format) {
	case GS_BGRA:
	case GS_BGRX:
		return IRLSAFETY_FORMAT_BGRA;
	default:
		return IRLSAFETY_FORMAT_RGBA;
	}
}

static void force_opaque_alpha(uint8_t *buffer, uint32_t width, uint32_t height, uint32_t linesize)
{
	for (uint32_t y = 0; y < height; y++) {
		uint8_t *row = buffer + (size_t)y * linesize;
		for (uint32_t x = 0; x < width; x++)
			row[x * 4 + 3] = 255;
	}
}

static void draw_region_sprite(gs_texture_t *tex, const irlsafety_rect *rect)
{
	float cx;
	float cy;
	float radians;

	if (!rect || rect->width < 1.0f || rect->height < 1.0f)
		return;

	if (!irlsafety_rect_has_rotation(rect)) {
		gs_matrix_push();
		gs_matrix_translate3f(rect->x, rect->y, 0.0f);
		gs_draw_sprite(tex, 0, (uint32_t)rect->width, (uint32_t)rect->height);
		gs_matrix_pop();
		return;
	}

	cx = rect->x + rect->width * 0.5f;
	cy = rect->y + rect->height * 0.5f;
	radians = rect->rotation_deg * (float)(M_PI / 180.0);

	gs_matrix_push();
	gs_matrix_translate3f(cx, cy, 0.0f);
	gs_matrix_rotaa4f(0.0f, 0.0f, -1.0f, radians);
	gs_matrix_translate3f(-rect->width * 0.5f, -rect->height * 0.5f, 0.0f);
	gs_draw_sprite(tex, 0, (uint32_t)rect->width, (uint32_t)rect->height);
	gs_matrix_pop();
}

static void obs_color_to_vec4(uint32_t color, struct vec4 *out)
{
	uint8_t bytes[4];

	if (!out)
		return;

	memcpy(bytes, &color, sizeof(bytes));
	out->x = (float)bytes[0] / 255.0f;
	out->y = (float)bytes[1] / 255.0f;
	out->z = (float)bytes[2] / 255.0f;
	out->w = (float)(bytes[3] ? bytes[3] : 255) / 255.0f;
}

static void copy_stage_to_buffer(uint8_t *dst, const uint8_t *src, uint32_t width, uint32_t height,
				 uint32_t stage_linesize, uint32_t dst_linesize)
{
	for (uint32_t y = 0; y < height; y++)
		memcpy(dst + (size_t)y * dst_linesize, src + (size_t)y * stage_linesize, (size_t)width * 4);
}

static const char *technique_for_spaces(enum gs_color_space source_space, float *multiplier)
{
	const enum gs_color_space current_space = gs_get_color_space();

	if (multiplier)
		*multiplier = 1.0f;

	switch (source_space) {
	case GS_CS_SRGB:
	case GS_CS_SRGB_16F:
		switch (current_space) {
		case GS_CS_709_SCRGB:
			if (multiplier)
				*multiplier = obs_get_video_sdr_white_level() / 80.0f;
			return "DrawMultiply";
		default:
			return "Draw";
		}
	case GS_CS_709_EXTENDED:
		switch (current_space) {
		case GS_CS_SRGB:
		case GS_CS_SRGB_16F:
			return "DrawTonemap";
		case GS_CS_709_SCRGB:
			if (multiplier)
				*multiplier = obs_get_video_sdr_white_level() / 80.0f;
			return "DrawMultiply";
		default:
			return "Draw";
		}
	default:
		return "Draw";
	}
}

static bool draw_texture_scaled_to_texrender(gs_texture_t *source, gs_texrender_t *dest, uint32_t width,
					     uint32_t height, enum gs_color_space color_space);

static bool capture_target(irlsafety_gpu_frame *gpu, uint32_t *out_w, uint32_t *out_h)
{
	obs_source_t *target;
	obs_source_t *parent;
	uint32_t parent_flags;
	bool custom_draw;
	bool async;
	enum gs_color_format capture_format;

	if (!gpu || !gpu->filter || !out_w || !out_h)
		return false;

	target = obs_filter_get_target(gpu->filter);
	parent = obs_filter_get_parent(gpu->filter);
	if (!target || !parent)
		return false;

	*out_w = filter_pixel_width(gpu->filter);
	*out_h = filter_pixel_height(gpu->filter);
	if (*out_w == 0 || *out_h == 0)
		return false;

	gpu->color_space = filter_color_space(gpu->filter);
	gpu->format = gs_get_format_from_space(gpu->color_space);
	capture_format = gpu->format;

	if (gs_texrender_get_format(gpu->capture) != capture_format) {
		gs_texrender_destroy(gpu->capture);
		gpu->capture = gs_texrender_create(capture_format, GS_ZS_NONE);
		if (!gpu->capture)
			return false;
	}

	if (gpu->width != *out_w || gpu->height != *out_h) {
		if (gpu->stage)
			gs_stagesurface_destroy(gpu->stage);
		if (gpu->output)
			gs_texture_destroy(gpu->output);
		gpu->stage = NULL;
		gpu->output = NULL;
	}

	if (!gpu->stage)
		gpu->stage = gs_stagesurface_create(*out_w, *out_h, capture_format);
	if (!gpu->output)
		gpu->output = gs_texture_create(*out_w, *out_h, capture_format, 1, NULL, GS_DYNAMIC);
	if (!gpu->stage || !gpu->output)
		return false;

	gpu->width = *out_w;
	gpu->height = *out_h;
	gpu->linesize = *out_w * 4;

	gs_texrender_reset(gpu->capture);
	if (!gs_texrender_begin_with_color_space(gpu->capture, *out_w, *out_h, gpu->color_space))
		return false;

	{
		struct vec4 clear_color;
		vec4_zero(&clear_color);
		gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
		gs_ortho(0.0f, (float)*out_w, 0.0f, (float)*out_h, -100.0f, 100.0f);

		parent_flags = obs_source_get_output_flags(parent);
		custom_draw = (parent_flags & OBS_SOURCE_CUSTOM_DRAW) != 0;
		async = (parent_flags & OBS_SOURCE_ASYNC) != 0;

		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

		if (target == parent && !custom_draw && !async)
			obs_source_default_render(target);
		else
			obs_source_video_render(target);

		gs_blend_state_pop();
	}

	gs_texrender_end(gpu->capture);
	gpu->captured = true;
	return true;
}

static int ensure_cpu_buffer(irlsafety_gpu_frame *gpu, uint32_t width, uint32_t height)
{
	size_t needed = (size_t)width * 4 * (size_t)height;

	if (!gpu)
		return -1;

	if (gpu->cpu_buffer_size < needed) {
		uint8_t *resized = realloc(gpu->cpu_buffer, needed);
		if (!resized)
			return -1;
		gpu->cpu_buffer = resized;
		gpu->cpu_buffer_size = needed;
	}

	return 0;
}

static bool ensure_ocr_stage(irlsafety_gpu_frame *gpu, uint32_t ocr_w, uint32_t ocr_h)
{
	enum gs_color_format capture_format = gpu->format;

	if (!gpu->ocr_scale) {
		gpu->ocr_scale = gs_texrender_create(capture_format, GS_ZS_NONE);
		if (!gpu->ocr_scale)
			return false;
	}

	if (gs_texrender_get_format(gpu->ocr_scale) != capture_format) {
		gs_texrender_destroy(gpu->ocr_scale);
		gpu->ocr_scale = gs_texrender_create(capture_format, GS_ZS_NONE);
		if (!gpu->ocr_scale)
			return false;
	}

	if (gpu->ocr_stage_w != ocr_w || gpu->ocr_stage_h != ocr_h || !gpu->ocr_stage) {
		if (gpu->ocr_stage)
			gs_stagesurface_destroy(gpu->ocr_stage);
		gpu->ocr_stage = gs_stagesurface_create(ocr_w, ocr_h, capture_format);
		if (!gpu->ocr_stage)
			return false;
		gpu->ocr_stage_w = ocr_w;
		gpu->ocr_stage_h = ocr_h;
	}

	return true;
}

irlsafety_gpu_frame *irlsafety_gpu_frame_create(obs_source_t *filter)
{
	irlsafety_gpu_frame *gpu = calloc(1, sizeof(*gpu));
	enum gs_color_format capture_format = GS_RGBA;

	if (!gpu)
		return NULL;

	gpu->filter = filter;
	gpu->format = capture_format;
	gpu->color_space = GS_CS_SRGB;
	gpu->capture = gs_texrender_create(capture_format, GS_ZS_NONE);
	if (!gpu->capture) {
		irlsafety_gpu_frame_destroy(gpu);
		return NULL;
	}

	return gpu;
}

void irlsafety_gpu_frame_destroy(irlsafety_gpu_frame *gpu)
{
	if (!gpu)
		return;

	if (gpu->capture)
		gs_texrender_destroy(gpu->capture);
	if (gpu->ocr_scale)
		gs_texrender_destroy(gpu->ocr_scale);
	if (gpu->stage)
		gs_stagesurface_destroy(gpu->stage);
	if (gpu->ocr_stage)
		gs_stagesurface_destroy(gpu->ocr_stage);
	if (gpu->overlay_tex)
		gs_texture_destroy(gpu->overlay_tex);
	if (gpu->output)
		gs_texture_destroy(gpu->output);
	free(gpu->cpu_buffer);
	free(gpu);
}

bool irlsafety_gpu_frame_begin_frame(irlsafety_gpu_frame *gpu, uint32_t *out_width, uint32_t *out_height)
{
	uint32_t width = 0;
	uint32_t height = 0;

	if (!gpu)
		return false;

	gpu->captured = false;
	if (!capture_target(gpu, &width, &height))
		return false;

	if (out_width)
		*out_width = width;
	if (out_height)
		*out_height = height;
	return true;
}

int irlsafety_gpu_frame_readback_ocr(irlsafety_gpu_frame *gpu, irlsafety_frame_view *out_view,
				     uint32_t output_width, uint32_t output_height, uint32_t ocr_max_width)
{
	gs_texture_t *captured;
	uint32_t ocr_w = 0;
	uint32_t ocr_h = 0;
	uint8_t *mapped = NULL;
	uint32_t stage_linesize = 0;
	uint32_t ocr_linesize;

	if (!gpu || !out_view || !gpu->captured || output_width == 0 || output_height == 0)
		return -1;

	irlsafety_compute_ocr_size(output_width, output_height, ocr_max_width, &ocr_w, &ocr_h, NULL, NULL);
	if (!ensure_ocr_stage(gpu, ocr_w, ocr_h))
		return -1;
	if (ensure_cpu_buffer(gpu, ocr_w, ocr_h) != 0)
		return -1;

	captured = gs_texrender_get_texture(gpu->capture);
	if (!captured)
		return -1;

	if (!draw_texture_scaled_to_texrender(captured, gpu->ocr_scale, ocr_w, ocr_h, gpu->color_space))
		return -1;

	gs_stage_texture(gpu->ocr_stage, gs_texrender_get_texture(gpu->ocr_scale));
	gs_flush();
	if (!gs_stagesurface_map(gpu->ocr_stage, &mapped, &stage_linesize))
		return -1;

	ocr_linesize = ocr_w * 4;
	copy_stage_to_buffer(gpu->cpu_buffer, mapped, ocr_w, ocr_h, stage_linesize, ocr_linesize);
	gs_stagesurface_unmap(gpu->ocr_stage);
	force_opaque_alpha(gpu->cpu_buffer, ocr_w, ocr_h, ocr_linesize);

	memset(out_view, 0, sizeof(*out_view));
	out_view->planes[0] = gpu->cpu_buffer;
	out_view->linesize[0] = ocr_linesize;
	out_view->width = ocr_w;
	out_view->height = ocr_h;
	/* D3D11 stage readback is always BGRA8 on Windows. */
	out_view->format = IRLSAFETY_FORMAT_BGRA;
	out_view->plane_count = 1;
	return 0;
}

bool irlsafety_gpu_frame_draw_captured(irlsafety_gpu_frame *gpu)
{
	gs_effect_t *effect;
	gs_eparam_t *image;
	gs_texture_t *captured;
	float multiplier;
	const char *technique;
	const bool previous = gs_framebuffer_srgb_enabled();

	if (!gpu || !gpu->captured || gpu->width == 0 || gpu->height == 0)
		return false;

	captured = gs_texrender_get_texture(gpu->capture);
	if (!captured)
		return false;

	effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	if (!effect)
		return false;

	image = gs_effect_get_param_by_name(effect, "image");
	if (!image)
		return false;

	technique = technique_for_spaces(gpu->color_space, &multiplier);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	gs_enable_framebuffer_srgb(true);
	gs_effect_set_texture_srgb(image, captured);
	gs_effect_set_float(gs_effect_get_param_by_name(effect, "multiplier"), multiplier);

	while (gs_effect_loop(effect, technique))
		gs_draw_sprite(captured, 0, gpu->width, gpu->height);

	gs_enable_framebuffer_srgb(previous);
	gs_blend_state_pop();
	return true;
}

static gs_texture_t *ensure_gpu_overlay_texture(irlsafety_gpu_frame *gpu, const irlsafety_overlay_image *overlay,
						const char *cache_key)
{
	if (!gpu || !overlay || !overlay->rgba || overlay->width == 0 || overlay->height == 0)
		return NULL;

	if (gpu->overlay_tex && strcmp(gpu->overlay_tex_key, cache_key) == 0)
		return gpu->overlay_tex;

	if (gpu->overlay_tex) {
		gs_texture_destroy(gpu->overlay_tex);
		gpu->overlay_tex = NULL;
	}

	gpu->overlay_tex =
		gs_texture_create(overlay->width, overlay->height, GS_RGBA, 1, (const uint8_t **)&overlay->rgba, 0);
	if (!gpu->overlay_tex)
		return NULL;

	strncpy(gpu->overlay_tex_key, cache_key, sizeof(gpu->overlay_tex_key) - 1);
	gpu->overlay_tex_key[sizeof(gpu->overlay_tex_key) - 1] = '\0';
	return gpu->overlay_tex;
}

void irlsafety_gpu_frame_draw_overlays(irlsafety_gpu_frame *gpu, struct obs_source *filter,
				       const irlsafety_region_list *regions, const irlsafety_filter_settings *settings)
{
	gs_effect_t *solid;
	gs_eparam_t *color_param;
	gs_technique_t *tech;
	struct vec4 color;
	const bool previous = gs_framebuffer_srgb_enabled();
	uint32_t color_value;
	size_t drawn = 0;

	if (!filter || !regions || !settings || regions->count == 0)
		return;

	if (settings->censor_mode == IRLSAFETY_CENSOR_BLUR)
		return;

	if (settings->censor_mode == IRLSAFETY_CENSOR_OVERLAY) {
		char resolved[1024];
		irlsafety_overlay_image *overlay;
		gs_effect_t *effect;
		gs_eparam_t *image_param;
		gs_technique_t *draw_tech;
		gs_texture_t *tex;

		if (!gpu)
			return;

		irlsafety_overlay_resolve_path(settings->censor_overlay_file, resolved, sizeof(resolved));
		overlay = irlsafety_overlay_acquire(settings->censor_overlay_file);
		tex = ensure_gpu_overlay_texture(gpu, overlay, resolved);
		if (!tex)
			goto solid_fallback;

		effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
		image_param = gs_effect_get_param_by_name(effect, "image");
		draw_tech = gs_effect_get_technique(effect, "Draw");
		if (!effect || !image_param || !draw_tech)
			goto solid_fallback;

		gs_blend_state_push();
		gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);
		gs_enable_framebuffer_srgb(true);
		gs_effect_set_texture(image_param, tex);
		gs_technique_begin(draw_tech);
		gs_technique_begin_pass(draw_tech, 0);

		for (size_t i = 0; i < regions->count; i++) {
			const irlsafety_rect *rect = &regions->regions[i];

			if (rect->width < 1.0f || rect->height < 1.0f)
				continue;

			draw_region_sprite(tex, rect);
			drawn++;
		}

		gs_technique_end_pass(draw_tech);
		gs_technique_end(draw_tech);
		gs_enable_framebuffer_srgb(previous);
		gs_blend_state_pop();

		if (settings->enable_logging && drawn > 0)
			obs_log(LOG_INFO, "IRLSAFETY+: drew %zu GPU overlay image(s)", drawn);
		return;
	}

solid_fallback:
	solid = obs_get_base_effect(OBS_EFFECT_SOLID);
	if (!solid)
		return;

	color_param = gs_effect_get_param_by_name(solid, "color");
	tech = gs_effect_get_technique(solid, "Solid");
	if (!color_param || !tech)
		return;

	color_value = settings->censor_color ? settings->censor_color : 0xFF404040;
	obs_color_to_vec4(color_value, &color);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);
	gs_enable_framebuffer_srgb(true);
	gs_effect_set_vec4(color_param, &color);

	gs_technique_begin(tech);
	gs_technique_begin_pass(tech, 0);

	for (size_t i = 0; i < regions->count; i++) {
		const irlsafety_rect *rect = &regions->regions[i];

		if (rect->width < 1.0f || rect->height < 1.0f)
			continue;

		draw_region_sprite(NULL, rect);
		drawn++;
	}

	gs_technique_end_pass(tech);
	gs_technique_end(tech);
	gs_enable_framebuffer_srgb(previous);
	gs_blend_state_pop();

	if (settings->enable_logging && drawn > 0)
		obs_log(LOG_INFO, "IRLSAFETY+: drew %zu GPU overlay box(es)", drawn);
}

void irlsafety_gpu_frame_draw_fullscreen_censor(irlsafety_gpu_frame *gpu, struct obs_source *filter,
						const irlsafety_filter_settings *settings, uint32_t width,
						uint32_t height)
{
	irlsafety_region_list regions;
	irlsafety_filter_settings solid_settings;

	if (!gpu || !filter || !settings || width == 0 || height == 0)
		return;

	solid_settings = *settings;
	if (solid_settings.censor_mode == IRLSAFETY_CENSOR_BLUR || solid_settings.censor_mode == IRLSAFETY_CENSOR_CLOUD ||
	    solid_settings.censor_mode == IRLSAFETY_CENSOR_ELLIPSE)
		solid_settings.censor_mode = IRLSAFETY_CENSOR_BOX;

	regions.count = 1;
	regions.regions[0] = (irlsafety_rect){
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)width,
		.height = (float)height,
		.confidence = 1.0f,
	};
	irlsafety_gpu_frame_draw_overlays(gpu, filter, &regions, &solid_settings);
}

static bool draw_texture_scaled_to_texrender(gs_texture_t *source, gs_texrender_t *dest, uint32_t width,
					     uint32_t height, enum gs_color_space color_space)
{
	gs_effect_t *effect;
	gs_eparam_t *image;
	float multiplier;
	const char *technique;
	const bool previous = gs_framebuffer_srgb_enabled();

	if (!source || !dest || width == 0 || height == 0)
		return false;

	effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	if (!effect)
		return false;

	image = gs_effect_get_param_by_name(effect, "image");
	if (!image)
		return false;

	technique = technique_for_spaces(color_space, &multiplier);

	gs_texrender_reset(dest);
	if (!gs_texrender_begin_with_color_space(dest, width, height, color_space))
		return false;

	gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f, 100.0f);
	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	gs_enable_framebuffer_srgb(true);
	gs_effect_set_texture_srgb(image, source);
	gs_effect_set_float(gs_effect_get_param_by_name(effect, "multiplier"), multiplier);

	while (gs_effect_loop(effect, technique))
		gs_draw_sprite(source, 0, width, height);

	gs_enable_framebuffer_srgb(previous);
	gs_blend_state_pop();
	gs_texrender_end(dest);
	return true;
}

static bool draw_output_texture(irlsafety_gpu_frame *gpu)
{
	gs_effect_t *effect;
	gs_eparam_t *image;
	float multiplier;
	const char *technique;
	const bool previous = gs_framebuffer_srgb_enabled();

	if (!gpu || !gpu->output || gpu->width == 0 || gpu->height == 0)
		return false;

	effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	if (!effect)
		return false;

	image = gs_effect_get_param_by_name(effect, "image");
	if (!image)
		return false;

	technique = technique_for_spaces(gpu->color_space, &multiplier);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	gs_enable_framebuffer_srgb(true);
	gs_effect_set_texture_srgb(image, gpu->output);
	gs_effect_set_float(gs_effect_get_param_by_name(effect, "multiplier"), multiplier);

	while (gs_effect_loop(effect, technique))
		gs_draw_sprite(gpu->output, 0, gpu->width, gpu->height);

	gs_enable_framebuffer_srgb(previous);
	gs_blend_state_pop();
	return true;
}

int irlsafety_gpu_frame_render_cpu_censor(irlsafety_gpu_frame *gpu, irlsafety_pipeline *pipeline,
					  const irlsafety_filter_settings *settings, uint32_t frame_width,
					  uint32_t frame_height)
{
	irlsafety_frame_view view;
	gs_texture_t *captured;
	uint8_t *mapped = NULL;
	uint32_t stage_linesize = 0;

	if (!gpu || !pipeline || !settings)
		return -1;

	if (!gpu->captured && !irlsafety_gpu_frame_begin_frame(gpu, NULL, NULL))
		return -1;

	captured = gs_texrender_get_texture(gpu->capture);
	if (!captured)
		return -1;

	if (ensure_cpu_buffer(gpu, gpu->width, gpu->height) != 0)
		return -1;

	gs_stage_texture(gpu->stage, captured);
	gs_flush();
	if (!gs_stagesurface_map(gpu->stage, &mapped, &stage_linesize))
		return -1;

	copy_stage_to_buffer(gpu->cpu_buffer, mapped, gpu->width, gpu->height, stage_linesize, gpu->linesize);
	gs_stagesurface_unmap(gpu->stage);

	memset(&view, 0, sizeof(view));
	view.planes[0] = gpu->cpu_buffer;
	view.linesize[0] = gpu->linesize;
	view.width = gpu->width;
	view.height = gpu->height;
	view.format = view_format_for_gs(gpu->format);
	view.plane_count = 1;

	if (irlsafety_pipeline_apply_cpu_censor(pipeline, &view, settings, frame_width, frame_height) != 0)
		return -1;

	force_opaque_alpha(gpu->cpu_buffer, gpu->width, gpu->height, gpu->linesize);
	gs_texture_set_image(gpu->output, gpu->cpu_buffer, gpu->linesize, false);

	if (!draw_output_texture(gpu))
		return -1;

	return 0;
}