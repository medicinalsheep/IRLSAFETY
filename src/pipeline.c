/*
 * IRLSAFETY+ — detection → OCR → blur orchestration.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "pipeline.h"

#include "blur/blur_compositor.h"
#include "detection/yolo_onnx.h"
#include "ocr/ocr_engine.h"

#include <stdlib.h>

struct irlsafety_pipeline {
	yolo_onnx_context *detector;
	ocr_engine_context *ocr;
	blur_compositor_context *blur;
	irlsafety_region_list detection_regions;
	irlsafety_region_list ocr_regions;
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

int irlsafety_pipeline_process_frame(irlsafety_pipeline *pipeline, irlsafety_frame_view *frame,
				     float blur_strength)
{
	if (!pipeline || !frame)
		return -1;

	if (detect_regions(pipeline->detector, frame, &pipeline->detection_regions) != 0)
		return -1;

	if (ocr_regions(pipeline->ocr, frame, &pipeline->detection_regions, &pipeline->ocr_regions) != 0)
		return -1;

	/* Merge OCR text boxes with YOLO detections for the final blur mask. */
	irlsafety_region_list merged = pipeline->detection_regions;
	for (size_t i = 0; i < pipeline->ocr_regions.count && merged.count < IRLSAFETY_MAX_REGIONS; i++)
		merged.regions[merged.count++] = pipeline->ocr_regions.regions[i];

	return apply_blur(pipeline->blur, frame, &merged, blur_strength);
}