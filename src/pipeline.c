/*
 * IRLSAFETY+ — detection → OCR → blur orchestration.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "pipeline.h"

#include "blur/blur_compositor.h"
#include "blur/overlay_image.h"
#include "censor_log.h"
#include "detection/yolo_onnx.h"
#include "irlsafety_control.h"
#include "ocr/ocr_backend.h"
#include "ocr/ocr_engine.h"
#include "ocr/ocr_frame_util.h"
#include "ocr/pii_match.h"
#include "ocr/pii_patterns.h"
#include "hybrid_delay.h"
#include "irlsafety_log.h"
#include "irlsafety_runtime.h"
#include "irlsafety_settings.h"
#include "irlsafety_shutdown.h"
#include "region_tracker.h"

#include <plugin-support.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IRLSAFETY_SECURE_RECENT_FRAMES 120
#define IRLSAFETY_SECURE_DROP_STREAK 2

struct irlsafety_pipeline {
	yolo_onnx_context *detector;
	ocr_engine_context *ocr;
	blur_compositor_context *blur;
	irlsafety_region_list detection_regions;
	irlsafety_region_list fresh_regions;
	irlsafety_region_tracker overlay_tracker;
	irlsafety_custom_pii_list custom_pii;
	irlsafety_filter_settings settings;
	uint64_t last_log_frame;
	uint64_t ocr_job_id;
	bool ocr_in_flight;
	float ocr_scale_x;
	float ocr_scale_y;
	float ocr_region_scale_x;
	float ocr_region_scale_y;
	uint32_t ocr_output_width;
	uint32_t ocr_output_height;
	uint64_t ocr_submit_frame;
	int ocr_pass;
	irlsafety_ocr_hit_list ocr_merged_hits;
	uint32_t ocr_source_width;
	uint32_t ocr_source_height;
	uint64_t last_tick_frame;
	uint64_t last_tracker_update_frame;
	irlsafety_hybrid_delay_runtime hybrid_delay;
	irlsafety_censor_log censor_log;
	uint64_t detection_run_count;
	irlsafety_region_list last_secured_regions;
	uint64_t last_secured_frame;
	int escalation_level;
	int insecure_streak;
	bool shutting_down;
};

static void copy_region_list(irlsafety_region_list *dest, const irlsafety_region_list *src)
{
	if (!dest)
		return;

	dest->count = 0;
	if (!src)
		return;

	for (size_t i = 0; i < src->count && dest->count < IRLSAFETY_MAX_REGIONS; i++)
		dest->regions[dest->count++] = src->regions[i];
}

static bool pipeline_wants_secure_protection(const irlsafety_pipeline *pipeline,
					     const irlsafety_filter_settings *active)
{
	if (!pipeline || !active || !active->secure_mode_enable)
		return false;

	if (active->cat_custom_pii && pipeline->custom_pii.count > 0)
		return true;

	return pipeline->last_secured_regions.count > 0;
}

static void pipeline_reset_secure_state(irlsafety_pipeline *pipeline)
{
	if (!pipeline)
		return;

	pipeline->last_secured_regions.count = 0;
	pipeline->last_secured_frame = 0;
	pipeline->escalation_level = 0;
	pipeline->insecure_streak = 0;
}

static void pipeline_mark_secured(irlsafety_pipeline *pipeline, uint64_t frame_index,
				  const irlsafety_region_list *regions)
{
	if (!pipeline || !regions || regions->count == 0)
		return;

	copy_region_list(&pipeline->last_secured_regions, regions);
	pipeline->last_secured_frame = frame_index;
	pipeline->escalation_level = 0;
	pipeline->insecure_streak = 0;
}

static void pipeline_update_tracker(irlsafety_pipeline *pipeline, const irlsafety_region_list *detected,
				    uint32_t output_width, uint32_t output_height, int frames_since,
				    uint64_t frame_index)
{
	const irlsafety_filter_settings *active = &pipeline->settings;
	bool secure_hold = active->secure_mode_enable && pipeline->escalation_level >= 1 &&
			   (!detected || detected->count == 0);

	if (secure_hold) {
		irlsafety_region_tracker_hold_predict(&pipeline->overlay_tracker, frames_since);
		pipeline->last_tracker_update_frame = frame_index;
		return;
	}

	irlsafety_region_tracker_update(&pipeline->overlay_tracker, detected, output_width, output_height,
					frames_since);
	pipeline->last_tracker_update_frame = frame_index;
}

static void pipeline_update_escalation(irlsafety_pipeline *pipeline, const irlsafety_filter_settings *active,
				       uint64_t frame_index, bool any_overlays)
{
	bool recent_secured;
	bool waiting_for_ocr;

	if (!pipeline || !active)
		return;

	if (!active->secure_mode_enable) {
		pipeline_reset_secure_state(pipeline);
		return;
	}

	if (!pipeline_wants_secure_protection(pipeline, active)) {
		pipeline->escalation_level = 0;
		pipeline->insecure_streak = 0;
		return;
	}

	if (any_overlays) {
		pipeline->insecure_streak = 0;
		if (!pipeline->ocr_in_flight)
			pipeline->escalation_level = 0;
		return;
	}

	recent_secured = pipeline->last_secured_regions.count > 0 && pipeline->last_secured_frame > 0 &&
			 frame_index >= pipeline->last_secured_frame &&
			 (frame_index - pipeline->last_secured_frame) <= IRLSAFETY_SECURE_RECENT_FRAMES;
	waiting_for_ocr = pipeline->ocr_in_flight && active->cat_custom_pii && pipeline->custom_pii.count > 0;

	if (!recent_secured && !waiting_for_ocr) {
		pipeline->escalation_level = 0;
		pipeline->insecure_streak = 0;
		return;
	}

	pipeline->insecure_streak++;

	if (waiting_for_ocr || pipeline->insecure_streak >= 1)
		pipeline->escalation_level = 1;

	if (active->secure_drop_frames && recent_secured) {
		if ((waiting_for_ocr && !any_overlays) ||
		    pipeline->insecure_streak >= IRLSAFETY_SECURE_DROP_STREAK)
			pipeline->escalation_level = 2;
	}
}

static bool should_log_frame(const irlsafety_filter_settings *settings, uint64_t frame_index, uint64_t *last_log_frame)
{
	if (!settings || !settings->enable_logging || !last_log_frame)
		return false;

	if (*last_log_frame == 0 || frame_index < *last_log_frame || (frame_index - *last_log_frame) >= 60) {
		*last_log_frame = frame_index;
		return true;
	}

	return false;
}

static bool settings_need_detection(const irlsafety_filter_settings *settings)
{
	return irlsafety_settings_detection_enabled(settings);
}

static uint8_t pipeline_estimate_load_tier(const irlsafety_filter_settings *settings)
{
	int score = 0;

	if (!settings)
		return 0;

	if (irlsafety_settings_detection_enabled(settings))
		score += 2;
	if (settings->cat_screen_text)
		score += 3;
	if (settings->cat_custom_pii)
		score += 2;
	if (settings->cat_sensitive_patterns)
		score += 1;
	if (settings->ocr_detail >= 2)
		score += 2;
	if (settings->frame_skip > 0 && settings->frame_skip <= 3)
		score += 2;
	else if (settings->frame_skip > 0 && settings->frame_skip <= 6)
		score += 1;

	if (score >= 6)
		return 2;
	if (score >= 3)
		return 1;
	return 0;
}

static uint32_t pipeline_estimate_scans_per_min(const irlsafety_filter_settings *settings)
{
	uint32_t scans = 0;

	if (!settings || settings->frame_skip < 1)
		return 0;

	if (irlsafety_settings_detection_enabled(settings))
		scans += (uint32_t)(60.0f * 30.0f / (float)settings->frame_skip + 0.5f);

	if (settings->cat_screen_text || settings->cat_custom_pii || settings->cat_sensitive_patterns) {
		uint32_t ocr_interval = settings->frame_skip;
		if (settings->ocr_detail >= 2)
			ocr_interval = settings->frame_skip < 2 ? 1 : settings->frame_skip / 2;
		if (ocr_interval < 1)
			ocr_interval = 1;
		scans += (uint32_t)(60.0f * 30.0f / (float)ocr_interval + 0.5f);
	}

	return scans;
}

static irlsafety_detection_config build_detection_config(const irlsafety_filter_settings *settings)
{
	irlsafety_detection_config config;

	memset(&config, 0, sizeof(config));
	if (!settings)
		return config;

	config.license_plates = settings->cat_license_plates;
	config.street_signs = settings->cat_street_signs;
	config.shipping_labels = settings->cat_shipping_labels;
	config.id_documents = settings->cat_id_documents;
	config.confidence_threshold = settings->confidence_threshold;
	config.angled_cover = settings->angled_cover;
	return config;
}

static bool settings_need_ocr(const irlsafety_filter_settings *settings, const irlsafety_custom_pii_list *custom_pii)
{
	if (!settings)
		return false;

	if (settings->cat_screen_text)
		return true;

	if (settings->cat_sensitive_patterns)
		return true;

	if (settings->cat_custom_pii && custom_pii && custom_pii->count > 0)
		return true;

	return false;
}

static void merge_ocr_hits_scaled(irlsafety_ocr_hit_list *dest, const irlsafety_ocr_hit_list *src, float scale_x,
				  float scale_y)
{
	size_t i;
	irlsafety_ocr_hit normalized;

	if (!dest || !src)
		return;

	for (i = 0; i < src->count && dest->count < IRLSAFETY_OCR_MAX_HITS; i++) {
		normalized = src->hits[i];
		normalized.x *= scale_x;
		normalized.y *= scale_y;
		normalized.width *= scale_x;
		normalized.height *= scale_y;
		dest->hits[dest->count++] = normalized;
	}
}

static void merge_regions(irlsafety_region_list *dest, const irlsafety_region_list *src)
{
	size_t i;

	if (!dest || !src)
		return;

	for (i = 0; i < src->count && dest->count < IRLSAFETY_MAX_REGIONS; i++)
		dest->regions[dest->count++] = src->regions[i];
}

static void append_test_region(irlsafety_region_list *regions, uint32_t frame_width, uint32_t frame_height,
				const irlsafety_filter_settings *settings)
{
	if (!regions || !settings || !settings->test_effect || regions->count >= IRLSAFETY_MAX_REGIONS)
		return;

	if (frame_width == 0 || frame_height == 0)
		return;

	regions->regions[regions->count++] = (irlsafety_rect){
		.x = frame_width * 0.35f,
		.y = frame_height * 0.35f,
		.width = frame_width * 0.30f,
		.height = frame_height * 0.30f,
		.confidence = 1.0f,
	};
}

irlsafety_pipeline *irlsafety_pipeline_create(void)
{
	irlsafety_pipeline *pipeline = calloc(1, sizeof(*pipeline));
	if (!pipeline)
		return NULL;

	pipeline->detector = yolo_onnx_create();
	pipeline->ocr = ocr_engine_create();
	pipeline->blur = blur_compositor_create();

	if (!pipeline->detector || !pipeline->ocr || !pipeline->blur) {
		irlsafety_pipeline_destroy(pipeline);
		return NULL;
	}

	irlsafety_settings_apply_defaults(&pipeline->settings);
	irlsafety_hybrid_delay_runtime_init(&pipeline->hybrid_delay);
	irlsafety_censor_log_init(&pipeline->censor_log);
	return pipeline;
}

void irlsafety_pipeline_shutdown(irlsafety_pipeline *pipeline)
{
	irlsafety_ocr_hit_list hits;

	if (!pipeline || pipeline->shutting_down)
		return;

	pipeline->shutting_down = true;

	for (int i = 0; i < 200 && pipeline->ocr_in_flight; i++) {
		memset(&hits, 0, sizeof(hits));
		if (ocr_poll_hits(pipeline->ocr_job_id, &hits) != 0)
			break;
		irlsafety_sleep_ms(5);
	}

	pipeline->ocr_in_flight = false;
	pipeline->ocr_pass = 0;
	memset(&pipeline->ocr_merged_hits, 0, sizeof(pipeline->ocr_merged_hits));
	ocr_clear_cached_source(pipeline->ocr);
	irlsafety_region_tracker_clear(&pipeline->overlay_tracker);
	irlsafety_hybrid_delay_runtime_reset(&pipeline->hybrid_delay);
	pipeline_reset_secure_state(pipeline);
}

void irlsafety_pipeline_destroy(irlsafety_pipeline *pipeline)
{
	if (!pipeline)
		return;

	irlsafety_pipeline_shutdown(pipeline);
	yolo_onnx_destroy(pipeline->detector);
	ocr_engine_destroy(pipeline->ocr);
	blur_compositor_destroy(pipeline->blur);
	free(pipeline);
}

int irlsafety_pipeline_update_settings(irlsafety_pipeline *pipeline, const irlsafety_filter_settings *settings)
{
	int loaded;

	if (!pipeline || !settings)
		return -1;

	pipeline->settings = *settings;
	ocr_backend_set_use_child(settings->use_child_ocr);
	memset(&pipeline->custom_pii, 0, sizeof(pipeline->custom_pii));
	irlsafety_region_tracker_clear(&pipeline->overlay_tracker);
	irlsafety_hybrid_delay_runtime_reset(&pipeline->hybrid_delay);
	pipeline_reset_secure_state(pipeline);
	pipeline->last_log_frame = 0;
	pipeline->last_tick_frame = 0;
	pipeline->last_tracker_update_frame = 0;
	pipeline->ocr_in_flight = false;
	pipeline->ocr_job_id = 0;
	pipeline->ocr_pass = 0;
	memset(&pipeline->ocr_merged_hits, 0, sizeof(pipeline->ocr_merged_hits));
	ocr_clear_cached_source(pipeline->ocr);

	irlsafety_pipeline_set_detector_model(pipeline, settings->model_path, settings->prefer_gpu);

	if (!settings->cat_custom_pii)
		return 0;

	loaded = irlsafety_custom_pii_load(settings->custom_pii_inline, settings->custom_pii_file, &pipeline->custom_pii);
	if (settings->enable_logging && loaded == 0)
		irlsafety_log(IRLSAFETY_LOG_INFO, "IRLSAFETY+: loaded %zu custom PII keyword(s)", pipeline->custom_pii.count);
	return loaded;
}

int irlsafety_pipeline_set_detector_model(irlsafety_pipeline *pipeline, const char *model_path, bool prefer_gpu)
{
	if (!pipeline || !pipeline->detector)
		return -1;

	return yolo_onnx_load_model(pipeline->detector, model_path, prefer_gpu);
}

void irlsafety_pipeline_get_runtime_status(const irlsafety_pipeline *pipeline, struct irlsafety_runtime_status *out,
					   uint64_t frame_count)
{
	const irlsafety_filter_settings *settings;

	if (!pipeline || !out)
		return;

	memset(out, 0, sizeof(*out));
	settings = &pipeline->settings;

	out->protection_enabled = settings->enable_all;
	out->cat_license_plates = settings->cat_license_plates;
	out->cat_street_signs = settings->cat_street_signs;
	out->cat_screen_text = settings->cat_screen_text;
	out->cat_sensitive_patterns = settings->cat_sensitive_patterns;
	out->cat_shipping_labels = settings->cat_shipping_labels;
	out->cat_id_documents = settings->cat_id_documents;
	out->cat_custom_pii = settings->cat_custom_pii;
	out->frame_count = frame_count;
	out->ocr_busy = pipeline->ocr_in_flight;
	out->overlay_count = (uint32_t)pipeline->overlay_tracker.count;
	out->frame_skip = settings->frame_skip;
	out->detection_run_count = pipeline->detection_run_count;
	out->load_tier = pipeline_estimate_load_tier(settings);
	out->approx_scans_per_min = pipeline_estimate_scans_per_min(settings);

	strncpy(out->model_path, settings->model_path, sizeof(out->model_path) - 1);
	out->model_path[sizeof(out->model_path) - 1] = '\0';

	if (pipeline->detector) {
		const char *ep = yolo_onnx_active_ep(pipeline->detector);

		out->detector_ready = yolo_onnx_is_ready(pipeline->detector);
		out->last_yolo_ms = yolo_onnx_last_inference_ms(pipeline->detector);
		strncpy(out->detector_ep, ep ? ep : "CPU", sizeof(out->detector_ep) - 1);
		out->detector_ep[sizeof(out->detector_ep) - 1] = '\0';
		strncpy(out->detector_message, yolo_onnx_status_message(pipeline->detector),
			sizeof(out->detector_message) - 1);
		out->detector_message[sizeof(out->detector_message) - 1] = '\0';
	}

	out->ocr_available = ocr_backend_available();
	{
		const char *name = ocr_backend_name();
		const char *status = ocr_backend_status_message();

		if (name) {
			strncpy(out->ocr_backend, name, sizeof(out->ocr_backend) - 1);
			out->ocr_backend[sizeof(out->ocr_backend) - 1] = '\0';
		}
		if (status) {
			strncpy(out->ocr_backend_status, status, sizeof(out->ocr_backend_status) - 1);
			out->ocr_backend_status[sizeof(out->ocr_backend_status) - 1] = '\0';
		}
	}
}

static void process_ocr_hits(irlsafety_pipeline *pipeline, const irlsafety_filter_settings *active,
			     const irlsafety_ocr_hit_list *hits, float scale_x, float scale_y, uint32_t output_width,
			     uint32_t output_height, bool log_frame)
{
	irlsafety_region_list custom_regions;
	irlsafety_region_list screen_regions;
	irlsafety_region_list pattern_regions;

	custom_regions.count = 0;
	screen_regions.count = 0;
	pattern_regions.count = 0;
	pipeline->fresh_regions.count = 0;

	if (active->cat_custom_pii) {
		if (pipeline->custom_pii.count == 0) {
			if (log_frame)
				irlsafety_log(IRLSAFETY_LOG_INFO, "IRLSAFETY+: Custom PII enabled but no keywords loaded");
		} else {
			irlsafety_match_custom_pii_hits(hits, &pipeline->custom_pii, scale_x, scale_y,
							irlsafety_settings_effective_partial_threshold(active),
							&custom_regions);
			if (log_frame)
				irlsafety_log(IRLSAFETY_LOG_INFO, "IRLSAFETY+: Custom PII — %zu region(s) matched (%zu keyword(s))",
					custom_regions.count, pipeline->custom_pii.count);
		}
	}

	if (active->cat_screen_text) {
		irlsafety_regions_from_screen_text_hits(hits, scale_x, scale_y, &screen_regions);
		if (log_frame)
			irlsafety_log(IRLSAFETY_LOG_INFO, "IRLSAFETY+: Screen Text — %zu region(s)", screen_regions.count);
	}

	if (active->cat_sensitive_patterns) {
		irlsafety_match_sensitive_pattern_hits(hits, scale_x, scale_y, &pattern_regions);
		if (log_frame)
			irlsafety_log(IRLSAFETY_LOG_INFO, "IRLSAFETY+: Sensitive Patterns — %zu region(s)", pattern_regions.count);
	}

	merge_regions(&pipeline->fresh_regions, &custom_regions);
	merge_regions(&pipeline->fresh_regions, &screen_regions);
	merge_regions(&pipeline->fresh_regions, &pattern_regions);

	{
		int frames_since = 1;

		if (pipeline->last_tracker_update_frame > 0 &&
		    pipeline->ocr_submit_frame > pipeline->last_tracker_update_frame)
			frames_since = (int)(pipeline->ocr_submit_frame - pipeline->last_tracker_update_frame);
		if (frames_since < 1)
			frames_since = 1;

		pipeline_update_tracker(pipeline, &pipeline->fresh_regions, output_width, output_height, frames_since,
					pipeline->ocr_submit_frame);
	}

	if (pipeline->fresh_regions.count > 0) {
		char detail[96];

		snprintf(detail, sizeof(detail), "OCR %zu region(s)", pipeline->fresh_regions.count);
		irlsafety_censor_log_push(&pipeline->censor_log, pipeline->ocr_submit_frame, IRLSAFETY_CENSOR_OCR,
					  (uint32_t)pipeline->fresh_regions.count, detail);
		pipeline_mark_secured(pipeline, pipeline->ocr_submit_frame, &pipeline->fresh_regions);
	}

	if (log_frame)
		irlsafety_log(IRLSAFETY_LOG_INFO, "IRLSAFETY+: tracking %zu overlay region(s) after detection",
			pipeline->overlay_tracker.count);
}

size_t irlsafety_pipeline_copy_censor_log(const irlsafety_pipeline *pipeline, irlsafety_censor_log_entry *out,
					  size_t max_entries)
{
	if (!pipeline)
		return 0;

	return irlsafety_censor_log_copy_recent(&pipeline->censor_log, out, max_entries);
}

void irlsafety_pipeline_clear_censor_log(irlsafety_pipeline *pipeline)
{
	if (!pipeline)
		return;

	irlsafety_censor_log_clear(&pipeline->censor_log);
}

void irlsafety_pipeline_poll_detection(irlsafety_pipeline *pipeline, const irlsafety_filter_settings *settings,
				       uint32_t output_width, uint32_t output_height)
{
	const irlsafety_filter_settings *active = settings ? settings : &pipeline->settings;
	irlsafety_ocr_hit_list hits;
	int poll_status;
	bool log_frame;

	if (!pipeline || !active->enable_all || !pipeline->ocr_in_flight)
		return;

	memset(&hits, 0, sizeof(hits));
	poll_status = ocr_poll_hits(pipeline->ocr_job_id, &hits);
	if (poll_status == 0)
		return;

	if (poll_status < 0) {
		pipeline->ocr_in_flight = false;
		pipeline->ocr_pass = 0;
		memset(&pipeline->ocr_merged_hits, 0, sizeof(pipeline->ocr_merged_hits));
		ocr_clear_cached_source(pipeline->ocr);
		log_frame = should_log_frame(active, pipeline->ocr_submit_frame, &pipeline->last_log_frame);
		if (log_frame)
			irlsafety_log(IRLSAFETY_LOG_WARNING, "IRLSAFETY+: OCR recognize failed (HRESULT 0x%08X)",
				ocr_backend_last_error());
		return;
	}

	merge_ocr_hits_scaled(&pipeline->ocr_merged_hits, &hits,
			      pipeline->ocr_scale_x * pipeline->ocr_region_scale_x,
			      pipeline->ocr_scale_y * pipeline->ocr_region_scale_y);

	if (active->ocr_detail == 2 && pipeline->ocr_pass == 0) {
		pipeline->ocr_pass = 1;
		if (ocr_submit_cached_hits(pipeline->ocr, IRLSAFETY_OCR_WIDTH_DETAILED, pipeline->ocr_source_width,
					   pipeline->ocr_source_height, &pipeline->ocr_job_id, &pipeline->ocr_scale_x,
					   &pipeline->ocr_scale_y) == 0)
			return;

		log_frame = should_log_frame(active, pipeline->ocr_submit_frame, &pipeline->last_log_frame);
		if (log_frame)
			irlsafety_log(IRLSAFETY_LOG_WARNING, "IRLSAFETY+: OCR dual-scan pass 2 submit failed — using pass 1 only");
	}

	pipeline->ocr_in_flight = false;
	pipeline->ocr_pass = 0;
	ocr_clear_cached_source(pipeline->ocr);
	log_frame = should_log_frame(active, pipeline->ocr_submit_frame, &pipeline->last_log_frame);

	if (log_frame)
		irlsafety_log(IRLSAFETY_LOG_INFO, "IRLSAFETY+: OCR pass — %zu hit(s) (scan → display %ux%u)",
			pipeline->ocr_merged_hits.count, pipeline->ocr_output_width, pipeline->ocr_output_height);

	if (log_frame && pipeline->ocr_merged_hits.count > 0)
		irlsafety_log(IRLSAFETY_LOG_INFO, "IRLSAFETY+: OCR sample: \"%s\"", pipeline->ocr_merged_hits.hits[0].text);
	else if (log_frame && settings_need_ocr(active, &pipeline->custom_pii))
		irlsafety_log(IRLSAFETY_LOG_INFO,
			"IRLSAFETY+: OCR found no text — try OCR Detail = Maximum (dual scan) or Standard for dense UI");

	process_ocr_hits(pipeline, active, &pipeline->ocr_merged_hits, 1.0f, 1.0f, pipeline->ocr_output_width,
		       pipeline->ocr_output_height, log_frame);
	memset(&pipeline->ocr_merged_hits, 0, sizeof(pipeline->ocr_merged_hits));
}

bool irlsafety_pipeline_ocr_busy(const irlsafety_pipeline *pipeline)
{
	return pipeline && pipeline->ocr_in_flight;
}

bool irlsafety_pipeline_needs_ocr(const irlsafety_pipeline *pipeline, const irlsafety_filter_settings *settings)
{
	const irlsafety_filter_settings *active = settings ? settings : (pipeline ? &pipeline->settings : NULL);

	if (!pipeline || !active)
		return false;

	return settings_need_ocr(active, &pipeline->custom_pii);
}

int irlsafety_pipeline_submit_detection(irlsafety_pipeline *pipeline, irlsafety_frame_view *frame,
					const irlsafety_filter_settings *settings, uint64_t frame_index,
					uint32_t output_width, uint32_t output_height)
{
	const irlsafety_filter_settings *active = settings ? settings : &pipeline->settings;
	bool log_frame;
	bool need_ocr;
	float region_scale_x = 1.0f;
	float region_scale_y = 1.0f;
	uint32_t max_width;

	if (!pipeline || !frame)
		return -1;

	if (!active->enable_all || pipeline->shutting_down || irlsafety_is_shutting_down())
		return 0;

	if (frame->width == 0 || frame->height == 0)
		return -1;

	if (output_width == 0 || output_height == 0) {
		output_width = frame->width;
		output_height = frame->height;
	}

	if (frame->width != output_width || frame->height != output_height) {
		region_scale_x = (float)output_width / (float)frame->width;
		region_scale_y = (float)output_height / (float)frame->height;
	}

	log_frame = should_log_frame(active, frame_index, &pipeline->last_log_frame);
	need_ocr = settings_need_ocr(active, &pipeline->custom_pii);
	pipeline->fresh_regions.count = 0;
	pipeline->detection_regions.count = 0;

	if (settings_need_detection(active)) {
		irlsafety_detection_config detection = build_detection_config(active);

		if (!yolo_onnx_is_ready(pipeline->detector)) {
			if (log_frame)
				irlsafety_log(IRLSAFETY_LOG_WARNING, "IRLSAFETY+: Object detection enabled but no ONNX model loaded (%s)",
					yolo_onnx_status_message(pipeline->detector));
		} else if (detect_regions(pipeline->detector, frame, &detection, &pipeline->detection_regions) != 0) {
			if (log_frame)
				irlsafety_log(IRLSAFETY_LOG_WARNING, "IRLSAFETY+: Object detection failed (%s)",
					yolo_onnx_status_message(pipeline->detector));
		} else {
			pipeline->detection_run_count++;
			if (pipeline->detection_regions.count > 0) {
				char detail[96];

				snprintf(detail, sizeof(detail), "YOLO %zu region(s)", pipeline->detection_regions.count);
				irlsafety_censor_log_push(&pipeline->censor_log, frame_index, IRLSAFETY_CENSOR_DETECT,
							  (uint32_t)pipeline->detection_regions.count, detail);
			}
			if (log_frame && pipeline->detection_regions.count > 0)
				irlsafety_log(IRLSAFETY_LOG_INFO, "IRLSAFETY+: Object detection — %zu region(s)",
					pipeline->detection_regions.count);
			merge_regions(&pipeline->fresh_regions, &pipeline->detection_regions);
			{
				int frames_since = 1;

				if (pipeline->last_tracker_update_frame > 0 && frame_index > pipeline->last_tracker_update_frame)
					frames_since = (int)(frame_index - pipeline->last_tracker_update_frame);
				if (frames_since < 1)
					frames_since = 1;

				pipeline_update_tracker(pipeline, &pipeline->fresh_regions, output_width, output_height,
							frames_since, frame_index);
			}

			if (pipeline->fresh_regions.count > 0)
				pipeline_mark_secured(pipeline, frame_index, &pipeline->fresh_regions);
		}
	}

	if (!need_ocr)
		return 0;

	if (pipeline->ocr_in_flight)
		return 0;

	if (!ocr_backend_available()) {
		if (log_frame)
			irlsafety_log(IRLSAFETY_LOG_WARNING, "IRLSAFETY+: Windows OCR is unavailable on this system");
		return 0;
	}

	pipeline->ocr_pass = 0;
	memset(&pipeline->ocr_merged_hits, 0, sizeof(pipeline->ocr_merged_hits));
	pipeline->ocr_source_width = frame->width;
	pipeline->ocr_source_height = frame->height;

	if (active->ocr_detail == 2)
		max_width = IRLSAFETY_OCR_WIDTH_STANDARD;
	else
		max_width = irlsafety_ocr_max_width_for_detail(active->ocr_detail);

	if (ocr_submit_hits(pipeline->ocr, frame, max_width, &pipeline->ocr_job_id, &pipeline->ocr_scale_x,
			    &pipeline->ocr_scale_y, active->ocr_detail == 2) != 0) {
		if (log_frame)
			irlsafety_log(IRLSAFETY_LOG_WARNING, "IRLSAFETY+: OCR submit busy or failed");
		return 0;
	}

	pipeline->ocr_in_flight = true;
	pipeline->ocr_region_scale_x = region_scale_x;
	pipeline->ocr_region_scale_y = region_scale_y;
	pipeline->ocr_output_width = output_width;
	pipeline->ocr_output_height = output_height;
	pipeline->ocr_submit_frame = frame_index;
	return 0;
}

int irlsafety_pipeline_detect_frame(irlsafety_pipeline *pipeline, irlsafety_frame_view *frame,
				    const irlsafety_filter_settings *settings, uint64_t frame_index,
				    uint32_t output_width, uint32_t output_height, bool heavy_source)
{
	const irlsafety_filter_settings *active = settings ? settings : &pipeline->settings;

	if (!pipeline)
		return -1;

	irlsafety_pipeline_poll_detection(pipeline, settings, output_width, output_height);

	if (!active->enable_all || !frame)
		return 0;

	if (!irlsafety_settings_should_run_detection(active, frame_index, heavy_source))
		return 0;

	if (irlsafety_pipeline_ocr_busy(pipeline) && irlsafety_pipeline_needs_ocr(pipeline, active) &&
	    !irlsafety_settings_detection_enabled(active))
		return 0;

	return irlsafety_pipeline_submit_detection(pipeline, frame, settings, frame_index, output_width, output_height);
}

void irlsafety_pipeline_tick(irlsafety_pipeline *pipeline, uint64_t frame_index,
			       const irlsafety_filter_settings *settings)
{
	const irlsafety_filter_settings *active = settings ? settings : &pipeline->settings;
	irlsafety_region_list current;
	int frames_elapsed = 1;
	bool any_overlays;

	if (!pipeline || !active || !active->enable_all)
		return;

	if (pipeline->last_tick_frame > 0 && frame_index > pipeline->last_tick_frame)
		frames_elapsed = (int)(frame_index - pipeline->last_tick_frame);
	if (frames_elapsed < 1)
		frames_elapsed = 1;

	if (pipeline->escalation_level >= 1 && pipeline->overlay_tracker.count > 0)
		irlsafety_region_tracker_hold_predict(&pipeline->overlay_tracker, frames_elapsed);

	current.count = 0;
	irlsafety_region_tracker_copy_regions(&pipeline->overlay_tracker, &current);
	any_overlays = current.count > 0;

	if (any_overlays)
		pipeline_mark_secured(pipeline, frame_index, &current);

	pipeline_update_escalation(pipeline, active, frame_index, any_overlays);

	if (pipeline->escalation_level >= 1 && current.count == 0 && pipeline->last_secured_regions.count > 0)
		copy_region_list(&current, &pipeline->last_secured_regions);

	irlsafety_hybrid_delay_tick(&pipeline->hybrid_delay, active, frame_index, frames_elapsed,
				    current.count > 0);

	if (active->hybrid_delay_enable)
		irlsafety_hybrid_delay_record(&pipeline->hybrid_delay, frame_index, &current,
					      current.count > 0 || pipeline->hybrid_delay.protection_latched);

	pipeline->last_tick_frame = frame_index;
}

void irlsafety_pipeline_get_overlays(irlsafety_pipeline *pipeline, uint32_t frame_width, uint32_t frame_height,
				     const irlsafety_filter_settings *settings, irlsafety_region_list *out_regions)
{
	const irlsafety_filter_settings *active;

	if (!pipeline || !out_regions)
		return;

	active = settings ? settings : &pipeline->settings;
	out_regions->count = 0;

	if (!active->enable_all)
		return;

	irlsafety_region_tracker_copy_regions(&pipeline->overlay_tracker, out_regions);

	if (out_regions->count == 0 && active->secure_mode_enable && pipeline->escalation_level >= 1 &&
	    pipeline->last_secured_regions.count > 0)
		copy_region_list(out_regions, &pipeline->last_secured_regions);

	if (active->hybrid_delay_enable && pipeline->hybrid_delay.effective_delay_sec > 0.0) {
		irlsafety_region_list live;
		irlsafety_region_list delayed;

		live = *out_regions;
		delayed.count = 0;

		if (irlsafety_hybrid_delay_lookup(&pipeline->hybrid_delay, pipeline->last_tick_frame,
						  pipeline->hybrid_delay.effective_delay_sec, &delayed) &&
		    delayed.count > 0)
			*out_regions = delayed;
		else
			*out_regions = live;
	}

	append_test_region(out_regions, frame_width, frame_height, active);
}

bool irlsafety_pipeline_needs_urgent_scan(const irlsafety_pipeline *pipeline)
{
	if (!pipeline || !pipeline->settings.secure_mode_enable)
		return false;

	return pipeline->escalation_level >= 1;
}

bool irlsafety_pipeline_should_drop_frame(const irlsafety_pipeline *pipeline,
					  const irlsafety_filter_settings *settings)
{
	const irlsafety_filter_settings *active = settings ? settings : (pipeline ? &pipeline->settings : NULL);

	if (!pipeline || !active || !active->enable_all || !active->secure_mode_enable || !active->secure_drop_frames)
		return false;

	return pipeline->escalation_level >= 2;
}

int irlsafety_pipeline_apply_cpu_censor(irlsafety_pipeline *pipeline, irlsafety_frame_view *frame,
				      const irlsafety_filter_settings *settings, uint32_t frame_width,
				      uint32_t frame_height)
{
	irlsafety_region_list merged;
	const irlsafety_filter_settings *active = settings ? settings : &pipeline->settings;
	irlsafety_censor_options options;

	if (!pipeline || !frame)
		return -1;

	if (!active->enable_all)
		return 0;

	irlsafety_pipeline_get_overlays(pipeline, frame_width, frame_height, active, &merged);
	if (merged.count == 0)
		return 0;

	options = (irlsafety_censor_options){
		.mode = active->censor_mode,
		.blur_strength = active->blur_strength,
		.color = active->censor_color ? active->censor_color : 0xFF404040,
		.overlay = NULL,
		.show_preview = active->show_preview,
	};

	if (active->censor_mode == IRLSAFETY_CENSOR_OVERLAY)
		options.overlay = irlsafety_overlay_acquire(active->censor_overlay_file);

	return apply_censor(pipeline->blur, frame, &merged, &options);
}