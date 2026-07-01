/*
 * IRLSAFETY+ — detection → OCR → blur orchestration.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "pipeline.h"

#include "blur/blur_compositor.h"
#include "detection/yolo_onnx.h"
#include "ocr/ocr_engine.h"

#include <obs-module.h>
#include <plugin-support.h>
#include <stdlib.h>
#include <string.h>

struct irlsafety_pipeline {
	yolo_onnx_context *detector;
	ocr_engine_context *ocr;
	blur_compositor_context *blur;
	irlsafety_region_list detection_regions;
	irlsafety_region_list ocr_regions;
	irlsafety_custom_pii_list custom_pii;
	irlsafety_filter_settings settings;
};

irlsafety_pipeline *irlsafety_pipeline_create(const char *yolo_model_path)
{
	irlsafety_pipeline *pipeline = calloc(1, sizeof(*pipeline));
	if (!pipeline)
		return NULL;

	pipeline->detector = yolo_onnx_create(yolo_model_path);
	pipeline->ocr = ocr_engine_create();
	pipeline->blur = blur_compositor_create();

	if (!pipeline->detector || !pipeline->ocr || !pipeline->blur) {
		irlsafety_pipeline_destroy(pipeline);
		return NULL;
	}

	irlsafety_filter_settings_load(NULL, &pipeline->settings);
	return pipeline;
}

void irlsafety_pipeline_destroy(irlsafety_pipeline *pipeline)
{
	if (!pipeline)
		return;

	yolo_onnx_destroy(pipeline->detector);
	ocr_engine_destroy(pipeline->ocr);
	blur_compositor_destroy(pipeline->blur);
	free(pipeline);
}

int irlsafety_pipeline_update_settings(irlsafety_pipeline *pipeline, const irlsafety_filter_settings *settings)
{
	if (!pipeline || !settings)
		return -1;

	pipeline->settings = *settings;

	if (!settings->cat_custom_pii) {
		memset(&pipeline->custom_pii, 0, sizeof(pipeline->custom_pii));
		return 0;
	}

	return irlsafety_custom_pii_load(settings->custom_pii_inline, settings->custom_pii_file, &pipeline->custom_pii);
}

int irlsafety_pipeline_process_frame(irlsafety_pipeline *pipeline, irlsafety_frame_view *frame,
				     const irlsafety_filter_settings *settings)
{
	irlsafety_region_list merged;
	const irlsafety_filter_settings *active = settings ? settings : &pipeline->settings;

	if (!pipeline || !frame)
		return -1;

	if (!active->enable_all)
		return 0;

	/* Category gates — stubs honor toggles so UI wiring is ready for ONNX/OCR backends. */
	if (active->cat_street_signs || active->cat_license_plates || active->cat_documents || active->cat_faces) {
		if (detect_regions(pipeline->detector, frame, &pipeline->detection_regions) != 0)
			return -1;
	} else {
		pipeline->detection_regions.count = 0;
	}

	if (active->cat_screen_text || active->cat_custom_pii) {
		if (ocr_regions(pipeline->ocr, frame, &pipeline->detection_regions, &pipeline->ocr_regions) != 0)
			return -1;
	} else {
		pipeline->ocr_regions.count = 0;
	}

	merged = pipeline->detection_regions;
	for (size_t i = 0; i < pipeline->ocr_regions.count && merged.count < IRLSAFETY_MAX_REGIONS; i++)
		merged.regions[merged.count++] = pipeline->ocr_regions.regions[i];

	if (active->enable_logging && merged.count == 0 && pipeline->custom_pii.count > 0)
		obs_log(LOG_INFO, "Custom PII list loaded (%zu entries), awaiting OCR backend", pipeline->custom_pii.count);

	return apply_blur(pipeline->blur, frame, &merged, active->blur_strength);
}