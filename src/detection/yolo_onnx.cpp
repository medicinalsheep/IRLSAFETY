/*
 * IRLSAFETY+ — YOLOv8 ONNX Runtime inference (Windows).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "yolo_onnx.h"

#include "yolo_preprocess.h"

#include <onnxruntime_c_api.h>
#include <onnxruntime_cxx_api.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

struct YoloCandidate {
	float cx;
	float cy;
	float w;
	float h;
	float confidence;
	int class_id;
	float angle_rad;
};

struct yolo_onnx_context {
	Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "irlsafety"};
	std::unique_ptr<Ort::Session> session;
	Ort::AllocatorWithDefaultOptions allocator;
	std::string input_name;
	std::string output_name;
	std::vector<int64_t> output_shape;
	std::vector<float> input_tensor;
	std::mutex mutex;
	char model_path[1024];
	char status[256];
	bool loaded = false;
	bool prefer_gpu = true;
	int num_classes = 0;
	bool obb_mode = false;
};

static void set_status(yolo_onnx_context *ctx, const char *message)
{
	if (!ctx || !message)
		return;
	strncpy(ctx->status, message, sizeof(ctx->status) - 1);
	ctx->status[sizeof(ctx->status) - 1] = '\0';
}

static bool class_enabled(const irlsafety_detection_config *config, int class_id)
{
	if (!config)
		return false;

	switch (class_id) {
	case IRLSAFETY_YOLO_CLASS_LICENSE_PLATE:
		return config->license_plates;
	case IRLSAFETY_YOLO_CLASS_STREET_SIGN:
		return config->street_signs;
	case IRLSAFETY_YOLO_CLASS_SHIPPING_LABEL:
		return config->shipping_labels;
	case IRLSAFETY_YOLO_CLASS_ID_DOCUMENT:
		return config->id_documents;
	default:
		return false;
	}
}

static float rect_iou(const irlsafety_rect *a, const irlsafety_rect *b)
{
	float x0 = std::max(a->x, b->x);
	float y0 = std::max(a->y, b->y);
	float x1 = std::min(a->x + a->width, b->x + b->width);
	float y1 = std::min(a->y + a->height, b->y + b->height);
	float inter_w = std::max(0.0f, x1 - x0);
	float inter_h = std::max(0.0f, y1 - y0);
	float inter = inter_w * inter_h;
	float uni = a->width * a->height + b->width * b->height - inter;

	if (uni <= 0.0f)
		return 0.0f;
	return inter / uni;
}

static void nms_candidates(std::vector<YoloCandidate> &candidates, float iou_threshold)
{
	std::sort(candidates.begin(), candidates.end(),
		  [](const YoloCandidate &a, const YoloCandidate &b) { return a.confidence > b.confidence; });

	std::vector<bool> suppressed(candidates.size(), false);
	for (size_t i = 0; i < candidates.size(); i++) {
		if (suppressed[i])
			continue;

		irlsafety_rect a{
			.x = candidates[i].cx - candidates[i].w * 0.5f,
			.y = candidates[i].cy - candidates[i].h * 0.5f,
			.width = candidates[i].w,
			.height = candidates[i].h,
			.confidence = candidates[i].confidence,
		};

		for (size_t j = i + 1; j < candidates.size(); j++) {
			if (suppressed[j] || candidates[j].class_id != candidates[i].class_id)
				continue;

			irlsafety_rect b{
				.x = candidates[j].cx - candidates[j].w * 0.5f,
				.y = candidates[j].cy - candidates[j].h * 0.5f,
				.width = candidates[j].w,
				.height = candidates[j].h,
				.confidence = candidates[j].confidence,
			};

			if (rect_iou(&a, &b) > iou_threshold)
				suppressed[j] = true;
		}
	}

	std::vector<YoloCandidate> kept;
	for (size_t i = 0; i < candidates.size(); i++) {
		if (!suppressed[i])
			kept.push_back(candidates[i]);
	}
	candidates.swap(kept);
}

yolo_onnx_context *yolo_onnx_create(void)
{
	try {
		return new yolo_onnx_context();
	} catch (...) {
		return nullptr;
	}
}

void yolo_onnx_destroy(yolo_onnx_context *ctx)
{
	if (!ctx)
		return;

	{
		std::lock_guard<std::mutex> lock(ctx->mutex);
		ctx->loaded = false;
		ctx->session.reset();
	}

	delete ctx;
}

int yolo_onnx_load_model(yolo_onnx_context *ctx, const char *model_path, bool prefer_gpu)
{
	if (!ctx)
		return -1;

	std::lock_guard<std::mutex> lock(ctx->mutex);
	ctx->loaded = false;
	ctx->session.reset();

	if (!model_path || model_path[0] == '\0') {
		ctx->model_path[0] = '\0';
		set_status(ctx, "No ONNX model configured");
		return 0;
	}

	try {
		Ort::SessionOptions options;
		/* Cap threads — fewer spikes on the OBS video/render thread (CPU EP). */
		options.SetIntraOpNumThreads(2);
		options.SetInterOpNumThreads(1);
		options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
		(void)prefer_gpu;

		std::wstring wide_path;
		{
			int needed = MultiByteToWideChar(CP_UTF8, 0, model_path, -1, nullptr, 0);
			if (needed > 0) {
				wide_path.resize((size_t)needed);
				MultiByteToWideChar(CP_UTF8, 0, model_path, -1, wide_path.data(), needed);
			}
		}

		ctx->session = std::make_unique<Ort::Session>(ctx->env, wide_path.c_str(), options);
		strncpy(ctx->model_path, model_path, sizeof(ctx->model_path) - 1);
		ctx->prefer_gpu = prefer_gpu;

		{
			Ort::AllocatedStringPtr in_name = ctx->session->GetInputNameAllocated(0, ctx->allocator);
			Ort::AllocatedStringPtr out_name = ctx->session->GetOutputNameAllocated(0, ctx->allocator);
			ctx->input_name = in_name.get();
			ctx->output_name = out_name.get();
			ctx->output_shape = ctx->session->GetOutputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();
		}

		if (ctx->output_shape.size() == 3 && ctx->output_shape[1] > 4) {
			int channels = (int)ctx->output_shape[1];
			if (channels == 7 || channels == 9 || channels == 11) {
				ctx->obb_mode = true;
				ctx->num_classes = channels - 5;
			} else {
				ctx->obb_mode = false;
				ctx->num_classes = channels - 4;
			}
		} else {
			ctx->num_classes = 1;
			ctx->obb_mode = false;
		}

		ctx->input_tensor.resize(3 * IRLSAFETY_YOLO_INPUT_SIZE * IRLSAFETY_YOLO_INPUT_SIZE);
		ctx->loaded = true;
		set_status(ctx, "Detection model loaded");
		return 0;
	} catch (const Ort::Exception &ex) {
		set_status(ctx, ex.what());
		return -1;
	} catch (...) {
		set_status(ctx, "Failed to load ONNX model");
		return -1;
	}
}

bool yolo_onnx_is_ready(const yolo_onnx_context *ctx)
{
	return ctx && ctx->loaded;
}

const char *yolo_onnx_status_message(const yolo_onnx_context *ctx)
{
	if (!ctx)
		return "Detector unavailable";
	return ctx->status[0] ? ctx->status : "Unknown";
}

int detect_regions(yolo_onnx_context *ctx, const irlsafety_frame_view *frame,
		   const irlsafety_detection_config *config, irlsafety_region_list *out_regions)
{
	if (!ctx || !frame || !config || !out_regions)
		return -1;

	out_regions->count = 0;

	if (!config->license_plates && !config->street_signs && !config->shipping_labels && !config->id_documents)
		return 0;

	if (!ctx->loaded)
		return 0;

	/* Reject generic COCO models (80 classes) — class 0 is "person", not a license plate. */
	if (ctx->num_classes > 8) {
		set_status(ctx, "Incompatible model (COCO) — train or export a plate/sign ONNX (see models/README.txt)");
		return 0;
	}

	if (frame->width == 0 || frame->height == 0 || !frame->planes[0])
		return -1;

	float threshold = config->confidence_threshold;
	if (threshold < 0.05f)
		threshold = 0.05f;
	if (threshold > 0.95f)
		threshold = 0.95f;

	try {
		std::lock_guard<std::mutex> lock(ctx->mutex);
		yolo_letterbox letterbox{};
		std::vector<YoloCandidate> candidates;

		if (yolo_frame_to_tensor(frame, ctx->input_tensor.data(), IRLSAFETY_YOLO_INPUT_SIZE,
					 IRLSAFETY_YOLO_INPUT_SIZE, &letterbox) != 0)
			return -1;

		std::vector<int64_t> input_shape = {1, 3, IRLSAFETY_YOLO_INPUT_SIZE, IRLSAFETY_YOLO_INPUT_SIZE};
		Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
		Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
			memory_info, ctx->input_tensor.data(), ctx->input_tensor.size(), input_shape.data(), input_shape.size());

		const char *input_names[] = {ctx->input_name.c_str()};
		const char *output_names[] = {ctx->output_name.c_str()};
		auto outputs = ctx->session->Run(Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);
		float *output_data = outputs[0].GetTensorMutableData<float>();
		auto out_shape = outputs[0].GetTensorTypeAndShapeInfo().GetShape();

		if (out_shape.size() != 3)
			return -1;

		int64_t channels = out_shape[1];
		int64_t anchors = out_shape[2];
		int num_classes = ctx->num_classes;
		int angle_channel = ctx->obb_mode ? (4 + num_classes) : -1;

		if (num_classes < 1)
			return -1;

		for (int64_t i = 0; i < anchors; i++) {
			float cx = output_data[0 * anchors + i];
			float cy = output_data[1 * anchors + i];
			float w = output_data[2 * anchors + i];
			float h = output_data[3 * anchors + i];
			int best_class = 0;
			float best_score = 0.0f;
			float angle_rad = 0.0f;

			for (int c = 0; c < num_classes; c++) {
				float score = output_data[(4 + c) * anchors + i];
				if (score > best_score) {
					best_score = score;
					best_class = c;
				}
			}

			if (best_score < threshold)
				continue;
			if (!class_enabled(config, best_class))
				continue;

			if (angle_channel >= 0)
				angle_rad = output_data[angle_channel * anchors + i];

			candidates.push_back({cx, cy, w, h, best_score, best_class, angle_rad});
		}

		nms_candidates(candidates, 0.45f);

		for (const YoloCandidate &cand : candidates) {
			irlsafety_rect rect{};
			if (out_regions->count >= IRLSAFETY_MAX_REGIONS)
				break;

			float pad_x;
			float pad_y;

			yolo_unmap_box(cand.cx, cand.cy, cand.w, cand.h, &letterbox, &rect);
			pad_x = rect.width * 0.05f;
			pad_y = rect.height * 0.05f;
			rect.x -= pad_x;
			rect.y -= pad_y;
			rect.width += pad_x * 2.0f;
			rect.height += pad_y * 2.0f;
			rect.confidence = cand.confidence;
			rect.rotation_deg = 0.0f;
			if (config->angled_cover && ctx->obb_mode && fabsf(cand.angle_rad) > 0.01f)
				rect.rotation_deg = cand.angle_rad * (180.0f / 3.14159265f);
			out_regions->regions[out_regions->count++] = rect;
		}

		return 0;
	} catch (const Ort::Exception &ex) {
		set_status(ctx, ex.what());
		return -1;
	} catch (...) {
		set_status(ctx, "Detection inference failed");
		return -1;
	}
}