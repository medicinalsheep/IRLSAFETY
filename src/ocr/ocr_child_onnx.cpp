/*
 * IRLSAFETY+ — Child OCR backend (ONNX, local-only, cross-platform target).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 *
 * PP-OCR-style det + rec ONNX pair (irlsafety-ocr-det.onnx + irlsafety-ocr-rec.onnx).
 * See data/models/OCR.txt and data/scripts/fetch-ocr-models.ps1.
 */

#include "ocr_backend.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#ifdef IRLSAFETY_ENABLE_ONNX
#include <onnxruntime_cxx_api.h>
#endif

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

struct ChildOcrWorkItem {
	std::vector<uint8_t> pixels;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t stride = 0;
	irlsafety_ocr_hit_list hits{};
	int result = -1;
	uint64_t job_id = 0;
};

struct OcrBox {
	float x = 0.0f;
	float y = 0.0f;
	float w = 0.0f;
	float h = 0.0f;
};

static std::mutex g_mutex;
static std::mutex g_work_mutex;
static std::condition_variable g_work_cv;
static std::condition_variable g_done_cv;
static std::unique_ptr<ChildOcrWorkItem> g_pending_job;
static std::unique_ptr<ChildOcrWorkItem> g_completed_job;
static std::once_flag g_worker_start_flag;
static std::thread g_worker_thread;
static std::atomic<bool> g_worker_running{false};
static std::atomic<bool> g_model_ready{false};
static std::atomic<uint64_t> g_next_job_id{1};
static bool g_tried_load = false;
static char g_status[256] = "Child OCR — place irlsafety-ocr-det.onnx + irlsafety-ocr-rec.onnx in models folder";
static char g_det_path[1024] = {0};
static char g_rec_path[1024] = {0};

#ifdef IRLSAFETY_ENABLE_ONNX
static Ort::Env g_env{ORT_LOGGING_LEVEL_WARNING, "irlsafety_ocr"};
static std::unique_ptr<Ort::Session> g_det_session;
static std::unique_ptr<Ort::Session> g_rec_session;
static Ort::AllocatorWithDefaultOptions g_allocator;
static std::string g_det_input_name;
static std::string g_rec_input_name;
static std::string g_det_output_name;
static std::string g_rec_output_name;
static std::vector<std::string> g_charset;
#endif

static constexpr float kDetMean[3] = {0.485f, 0.456f, 0.406f};
static constexpr float kDetStd[3] = {0.229f, 0.224f, 0.225f};
static constexpr float kRecMean[3] = {0.5f, 0.5f, 0.5f};
static constexpr float kRecStd[3] = {0.5f, 0.5f, 0.5f};
static constexpr uint32_t kDetLimitSide = 960;
static constexpr uint32_t kRecHeight = 48;
static constexpr float kDetThresh = 0.3f;
static constexpr uint32_t kDetMinArea = 16;

static void set_status(const char *msg)
{
	if (!msg)
		return;
	strncpy(g_status, msg, sizeof(g_status) - 1);
	g_status[sizeof(g_status) - 1] = '\0';
}

static bool push_hit(irlsafety_ocr_hit_list *out_hits, const char *text, float x, float y, float width, float height)
{
	irlsafety_ocr_hit *hit;

	if (!out_hits || !text || text[0] == '\0')
		return true;

	if (out_hits->count >= IRLSAFETY_OCR_MAX_HITS)
		return true;

	hit = &out_hits->hits[out_hits->count];
	strncpy(hit->text, text, sizeof(hit->text) - 1);
	hit->text[sizeof(hit->text) - 1] = '\0';
	hit->x = x;
	hit->y = y;
	hit->width = width;
	hit->height = height;
	out_hits->count++;
	return true;
}

static int copy_pixels_to_job(ChildOcrWorkItem &job, const uint8_t *bgra, uint32_t width, uint32_t height,
			      uint32_t stride)
{
	const uint32_t row_bytes = width * 4;
	const size_t needed = (size_t)row_bytes * (size_t)height;

	if (!bgra || width == 0 || height == 0)
		return -1;

	job.width = width;
	job.height = height;
	job.stride = stride ? stride : row_bytes;
	job.pixels.resize(needed);

	if (job.stride == row_bytes) {
		memcpy(job.pixels.data(), bgra, needed);
	} else {
		for (uint32_t y = 0; y < height; y++)
			memcpy(job.pixels.data() + (size_t)y * row_bytes, bgra + (size_t)y * job.stride, row_bytes);
	}

	return 0;
}

#ifdef IRLSAFETY_ENABLE_ONNX

static std::wstring path_to_wide(const char *path)
{
	std::wstring wide_path;
	if (!path || path[0] == '\0')
		return wide_path;

#ifdef _WIN32
	int needed = MultiByteToWideChar(CP_UTF8, 0, path, -1, nullptr, 0);
	if (needed > 0) {
		wide_path.resize((size_t)needed);
		MultiByteToWideChar(CP_UTF8, 0, path, -1, wide_path.data(), needed);
	}
#else
	(void)path;
#endif
	return wide_path;
}

static std::unique_ptr<Ort::Session> load_session(const char *path)
{
	Ort::SessionOptions options;
	options.SetIntraOpNumThreads(2);
	options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

#ifdef _WIN32
	std::wstring wide = path_to_wide(path);
	return std::make_unique<Ort::Session>(g_env, wide.c_str(), options);
#else
	return std::make_unique<Ort::Session>(g_env, path, options);
#endif
}

static void load_charset_near_model(const char *model_path)
{
	g_charset.clear();
	g_charset.push_back(""); /* CTC blank */

	std::string dict_path;
	if (model_path && model_path[0] != '\0') {
		std::string base(model_path);
		const size_t slash = base.find_last_of("/\\");
		if (slash != std::string::npos)
			dict_path = base.substr(0, slash + 1) + "en_dict.txt";
	}

	bool loaded = false;
	if (!dict_path.empty()) {
		std::ifstream in(dict_path);
		std::string line;
		while (std::getline(in, line)) {
			if (!line.empty())
				g_charset.push_back(line);
		}
		loaded = g_charset.size() > 1;
	}

	if (!loaded) {
		const char *fallback =
			"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~ ";
		for (const char *p = fallback; *p; p++)
			g_charset.push_back(std::string(1, *p));
	}
}

static void compute_det_size(uint32_t src_w, uint32_t src_h, uint32_t *out_w, uint32_t *out_h)
{
	float ratio = 1.0f;
	const float max_side = (float)std::max(src_w, src_h);

	if (max_side > (float)kDetLimitSide)
		ratio = (float)kDetLimitSide / max_side;

	uint32_t w = (uint32_t)std::max(32.0f, std::round((float)src_w * ratio / 32.0f) * 32.0f);
	uint32_t h = (uint32_t)std::max(32.0f, std::round((float)src_h * ratio / 32.0f) * 32.0f);

	if (out_w)
		*out_w = w;
	if (out_h)
		*out_h = h;
}

static void sample_bgra(const uint8_t *pixels, uint32_t src_w, uint32_t src_h, uint32_t stride, float fx, float fy,
			float *r, float *g, float *b)
{
	uint32_t x0 = (uint32_t)fx;
	uint32_t y0 = (uint32_t)fy;
	uint32_t x1 = std::min(x0 + 1, src_w - 1);
	uint32_t y1 = std::min(y0 + 1, src_h - 1);
	float dx = fx - (float)x0;
	float dy = fy - (float)y0;

	const uint8_t *p00 = pixels + (size_t)y0 * stride + (size_t)x0 * 4;
	const uint8_t *p10 = pixels + (size_t)y0 * stride + (size_t)x1 * 4;
	const uint8_t *p01 = pixels + (size_t)y1 * stride + (size_t)x0 * 4;
	const uint8_t *p11 = pixels + (size_t)y1 * stride + (size_t)x1 * 4;

	auto lerp = [&](float a, float b, float c, float d) {
		return ((1.0f - dx) * (1.0f - dy) * a + dx * (1.0f - dy) * b + (1.0f - dx) * dy * c + dx * dy * d) / 255.0f;
	};

	*b = lerp((float)p00[0], (float)p10[0], (float)p01[0], (float)p11[0]);
	*g = lerp((float)p00[1], (float)p10[1], (float)p01[1], (float)p11[1]);
	*r = lerp((float)p00[2], (float)p10[2], (float)p01[2], (float)p11[2]);
}

static void build_det_tensor(const uint8_t *pixels, uint32_t src_w, uint32_t src_h, uint32_t stride, uint32_t dst_w,
			     uint32_t dst_h, std::vector<float> &tensor)
{
	const size_t plane = (size_t)dst_w * (size_t)dst_h;
	tensor.resize(plane * 3);

	const float scale_x = (float)src_w / (float)dst_w;
	const float scale_y = (float)src_h / (float)dst_h;

	for (uint32_t y = 0; y < dst_h; y++) {
		for (uint32_t x = 0; x < dst_w; x++) {
			float r, g, b;
			const float fx = ((float)x + 0.5f) * scale_x - 0.5f;
			const float fy = ((float)y + 0.5f) * scale_y - 0.5f;
			sample_bgra(pixels, src_w, src_h, stride, fx, fy, &r, &g, &b);
			const size_t idx = (size_t)y * dst_w + x;
			tensor[idx] = (r - kDetMean[0]) / kDetStd[0];
			tensor[plane + idx] = (g - kDetMean[1]) / kDetStd[1];
			tensor[plane * 2 + idx] = (b - kDetMean[2]) / kDetStd[2];
		}
	}
}

static void extract_boxes_from_bitmap(const std::vector<uint8_t> &bitmap, uint32_t map_w, uint32_t map_h,
				      float scale_x, float scale_y, std::vector<OcrBox> &boxes)
{
	std::vector<uint8_t> visited(bitmap.size(), 0);

	for (uint32_t y = 0; y < map_h; y++) {
		for (uint32_t x = 0; x < map_w; x++) {
			const size_t idx = (size_t)y * map_w + x;
			if (!bitmap[idx] || visited[idx])
				continue;

			uint32_t min_x = x;
			uint32_t min_y = y;
			uint32_t max_x = x;
			uint32_t max_y = y;
			uint32_t area = 0;

			std::queue<std::pair<uint32_t, uint32_t>> q;
			q.push({x, y});
			visited[idx] = 1;

			while (!q.empty()) {
				auto [cx, cy] = q.front();
				q.pop();
				area++;

				if (cx < min_x)
					min_x = cx;
				if (cy < min_y)
					min_y = cy;
				if (cx > max_x)
					max_x = cx;
				if (cy > max_y)
					max_y = cy;

				static const int dx[4] = {1, -1, 0, 0};
				static const int dy[4] = {0, 0, 1, -1};
				for (int i = 0; i < 4; i++) {
					const int nx = (int)cx + dx[i];
					const int ny = (int)cy + dy[i];
					if (nx < 0 || ny < 0 || nx >= (int)map_w || ny >= (int)map_h)
						continue;
					const size_t nidx = (size_t)ny * map_w + (size_t)nx;
					if (!bitmap[nidx] || visited[nidx])
						continue;
					visited[nidx] = 1;
					q.push({(uint32_t)nx, (uint32_t)ny});
				}
			}

			if (area < kDetMinArea)
				continue;

			OcrBox box;
			box.x = (float)min_x * scale_x;
			box.y = (float)min_y * scale_y;
			box.w = (float)(max_x - min_x + 1) * scale_x;
			box.h = (float)(max_y - min_y + 1) * scale_y;
			boxes.push_back(box);
		}
	}
}

static int run_det(const uint8_t *pixels, uint32_t src_w, uint32_t src_h, uint32_t stride, std::vector<OcrBox> &boxes)
{
	uint32_t det_w = 0;
	uint32_t det_h = 0;
	compute_det_size(src_w, src_h, &det_w, &det_h);

	std::vector<float> tensor;
	build_det_tensor(pixels, src_w, src_h, stride, det_w, det_h, tensor);

	std::array<int64_t, 4> shape = {1, 3, (int64_t)det_h, (int64_t)det_w};
	Ort::MemoryInfo mem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
	Ort::Value input = Ort::Value::CreateTensor<float>(mem, tensor.data(), tensor.size(), shape.data(), shape.size());

	const char *input_names[] = {g_det_input_name.c_str()};
	const char *output_names[] = {g_det_output_name.c_str()};
	auto outputs = g_det_session->Run(Ort::RunOptions{nullptr}, input_names, &input, 1, output_names, 1);

	float *map = outputs[0].GetTensorMutableData<float>();
	auto map_info = outputs[0].GetTensorTypeAndShapeInfo();
	std::vector<int64_t> map_shape = map_info.GetShape();
	if (map_shape.size() < 2)
		return -1;

	uint32_t map_h = (uint32_t)map_shape[map_shape.size() - 2];
	uint32_t map_w = (uint32_t)map_shape[map_shape.size() - 1];
	const size_t map_size = (size_t)map_w * (size_t)map_h;

	std::vector<uint8_t> bitmap(map_size, 0);
	for (size_t i = 0; i < map_size; i++)
		bitmap[i] = map[i] >= kDetThresh ? 1 : 0;

	const float scale_x = (float)src_w / (float)map_w;
	const float scale_y = (float)src_h / (float)map_h;
	extract_boxes_from_bitmap(bitmap, map_w, map_h, scale_x, scale_y, boxes);
	return 0;
}

static void build_rec_tensor(const uint8_t *pixels, uint32_t src_w, uint32_t src_h, uint32_t stride, const OcrBox &box,
			     std::vector<float> &tensor, uint32_t *out_w)
{
	uint32_t x0 = (uint32_t)std::max(0.0f, box.x);
	uint32_t y0 = (uint32_t)std::max(0.0f, box.y);
	uint32_t x1 = (uint32_t)std::min((float)src_w, box.x + box.w);
	uint32_t y1 = (uint32_t)std::min((float)src_h, box.y + box.h);
	if (x1 <= x0 || y1 <= y0)
		return;

	const uint32_t crop_w = x1 - x0;
	const uint32_t crop_h = y1 - y0;
	const uint32_t rec_w = std::max(8u, (uint32_t)std::round((float)crop_w * (float)kRecHeight / (float)crop_h));
	const size_t plane = (size_t)rec_w * (size_t)kRecHeight;
	tensor.resize(plane * 3);

	for (uint32_t y = 0; y < kRecHeight; y++) {
		for (uint32_t x = 0; x < rec_w; x++) {
			const float fx = (float)x0 + ((float)x + 0.5f) * (float)crop_w / (float)rec_w - 0.5f;
			const float fy = (float)y0 + ((float)y + 0.5f) * (float)crop_h / (float)kRecHeight - 0.5f;
			float r, g, b;
			sample_bgra(pixels, src_w, src_h, stride, fx, fy, &r, &g, &b);
			const size_t idx = (size_t)y * rec_w + x;
			tensor[idx] = (r - kRecMean[0]) / kRecStd[0];
			tensor[plane + idx] = (g - kRecMean[1]) / kRecStd[1];
			tensor[plane * 2 + idx] = (b - kRecMean[2]) / kRecStd[2];
		}
	}

	if (out_w)
		*out_w = rec_w;
}

static std::string ctc_decode(const float *logits, size_t seq_len, size_t num_classes)
{
	std::string text;
	int prev = -1;

	for (size_t t = 0; t < seq_len; t++) {
		const float *step = logits + t * num_classes;
		int best = 0;
		float best_val = step[0];
		for (size_t c = 1; c < num_classes; c++) {
			if (step[c] > best_val) {
				best_val = step[c];
				best = (int)c;
			}
		}

		if (best != 0 && best != prev && (size_t)best < g_charset.size())
			text += g_charset[(size_t)best];

		prev = best;
	}

	return text;
}

static int recognize_box(const uint8_t *pixels, uint32_t src_w, uint32_t src_h, uint32_t stride, const OcrBox &box,
			 char *out_text, size_t out_len)
{
	std::vector<float> tensor;
	uint32_t rec_w = 0;
	build_rec_tensor(pixels, src_w, src_h, stride, box, tensor, &rec_w);
	if (tensor.empty() || rec_w == 0)
		return -1;

	std::array<int64_t, 4> shape = {1, 3, (int64_t)kRecHeight, (int64_t)rec_w};
	Ort::MemoryInfo mem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
	Ort::Value input = Ort::Value::CreateTensor<float>(mem, tensor.data(), tensor.size(), shape.data(), shape.size());

	const char *input_names[] = {g_rec_input_name.c_str()};
	const char *output_names[] = {g_rec_output_name.c_str()};
	auto outputs = g_rec_session->Run(Ort::RunOptions{nullptr}, input_names, &input, 1, output_names, 1);

	float *logits = outputs[0].GetTensorMutableData<float>();
	auto info = outputs[0].GetTensorTypeAndShapeInfo();
	std::vector<int64_t> out_shape = info.GetShape();
	if (out_shape.size() < 3)
		return -1;

	const size_t seq_len = (size_t)out_shape[1];
	const size_t num_classes = (size_t)out_shape[2];
	std::string decoded = ctc_decode(logits, seq_len, num_classes);
	if (decoded.empty())
		return 0;

	strncpy(out_text, decoded.c_str(), out_len - 1);
	out_text[out_len - 1] = '\0';
	return 0;
}

#endif /* IRLSAFETY_ENABLE_ONNX */

static void try_load_model_locked()
{
	if (g_tried_load)
		return;

	g_tried_load = true;
	g_model_ready.store(false, std::memory_order_release);

#ifndef IRLSAFETY_ENABLE_ONNX
	set_status("Child OCR requires ONNX Runtime build (IRLSAFETY_ENABLE_ONNX)");
	return;
#else
	if (g_det_path[0] == '\0' || g_rec_path[0] == '\0') {
		set_status("Child OCR — irlsafety-ocr-det.onnx + irlsafety-ocr-rec.onnx not found (see models/OCR.txt)");
		return;
	}

	try {
		g_det_session = load_session(g_det_path);
		g_rec_session = load_session(g_rec_path);

		{
			Ort::AllocatedStringPtr in = g_det_session->GetInputNameAllocated(0, g_allocator);
			Ort::AllocatedStringPtr out = g_det_session->GetOutputNameAllocated(0, g_allocator);
			g_det_input_name = in.get();
			g_det_output_name = out.get();
		}
		{
			Ort::AllocatedStringPtr in = g_rec_session->GetInputNameAllocated(0, g_allocator);
			Ort::AllocatedStringPtr out = g_rec_session->GetOutputNameAllocated(0, g_allocator);
			g_rec_input_name = in.get();
			g_rec_output_name = out.get();
		}

		load_charset_near_model(g_rec_path);

		g_model_ready.store(true, std::memory_order_release);
		set_status("Child OCR ready (local ONNX det + rec)");
	} catch (const std::exception &ex) {
		g_det_session.reset();
		g_rec_session.reset();
		set_status(ex.what());
	}
#endif
}

static int recognize_on_worker(const ChildOcrWorkItem &work, irlsafety_ocr_hit_list *out_hits)
{
	if (!out_hits || work.pixels.empty() || work.width == 0 || work.height == 0)
		return -1;

	memset(out_hits, 0, sizeof(*out_hits));

	if (!g_model_ready.load(std::memory_order_acquire))
		return -1;

#ifdef IRLSAFETY_ENABLE_ONNX
	const uint32_t row_bytes = work.width * 4;
	const uint8_t *pixels = work.pixels.data();
	const uint32_t stride = work.stride ? work.stride : row_bytes;

	std::vector<OcrBox> boxes;
	if (run_det(pixels, work.width, work.height, stride, boxes) != 0)
		return -1;

	if (boxes.empty()) {
		OcrBox full{0.0f, 0.0f, (float)work.width, (float)work.height};
		boxes.push_back(full);
	}

	std::sort(boxes.begin(), boxes.end(), [](const OcrBox &a, const OcrBox &b) {
		if (a.y != b.y)
			return a.y < b.y;
		return a.x < b.x;
	});

	for (const OcrBox &box : boxes) {
		char text[IRLSAFETY_OCR_TEXT_LEN];
		text[0] = '\0';
		if (recognize_box(pixels, work.width, work.height, stride, box, text, sizeof(text)) != 0)
			continue;
		push_hit(out_hits, text, box.x, box.y, box.w, box.h);
	}
#endif

	return 0;
}

static void child_ocr_worker_loop(void)
{
	while (g_worker_running.load(std::memory_order_acquire)) {
		ChildOcrWorkItem job;

		{
			std::unique_lock<std::mutex> lock(g_work_mutex);
			g_work_cv.wait(lock, []() {
				return g_pending_job != nullptr || !g_worker_running.load(std::memory_order_acquire);
			});

			if (!g_worker_running.load(std::memory_order_acquire))
				break;

			if (!g_pending_job)
				continue;

			job = std::move(*g_pending_job);
			g_pending_job.reset();
		}

		job.result = recognize_on_worker(job, &job.hits);

		{
			std::lock_guard<std::mutex> lock(g_work_mutex);
			g_completed_job = std::make_unique<ChildOcrWorkItem>(std::move(job));
		}
		g_done_cv.notify_all();
	}
}

static void ensure_worker(void)
{
	std::call_once(g_worker_start_flag, []() {
		g_worker_running.store(true, std::memory_order_release);
		g_worker_thread = std::thread(child_ocr_worker_loop);
	});
}

extern "C" void ocr_backend_configure_models(const char *det_path, const char *rec_path)
{
	std::lock_guard<std::mutex> lock(g_mutex);

	if (det_path && det_path[0] != '\0') {
		strncpy(g_det_path, det_path, sizeof(g_det_path) - 1);
		g_det_path[sizeof(g_det_path) - 1] = '\0';
	} else {
		g_det_path[0] = '\0';
	}

	if (rec_path && rec_path[0] != '\0') {
		strncpy(g_rec_path, rec_path, sizeof(g_rec_path) - 1);
		g_rec_path[sizeof(g_rec_path) - 1] = '\0';
	} else {
		g_rec_path[0] = '\0';
	}

	g_tried_load = false;
#ifdef IRLSAFETY_ENABLE_ONNX
	g_det_session.reset();
	g_rec_session.reset();
#endif
	g_model_ready.store(false, std::memory_order_release);
	try_load_model_locked();
}

extern "C" void ocr_backend_configure(const char *model_path)
{
	ocr_backend_configure_models(model_path, nullptr);
}

extern "C" const char *ocr_backend_name(void)
{
	return "IRLSAFETY Child OCR (ONNX)";
}

extern "C" bool ocr_backend_available(void)
{
	std::lock_guard<std::mutex> lock(g_mutex);
	try_load_model_locked();
	ensure_worker();
	return g_model_ready.load(std::memory_order_acquire);
}

extern "C" void ocr_backend_shutdown(void)
{
	if (g_worker_running.exchange(false)) {
		g_work_cv.notify_all();
		if (g_worker_thread.joinable())
			g_worker_thread.join();
	}

	{
		std::lock_guard<std::mutex> lock(g_work_mutex);
		g_pending_job.reset();
		g_completed_job.reset();
	}

	std::lock_guard<std::mutex> lock(g_mutex);
#ifdef IRLSAFETY_ENABLE_ONNX
	g_det_session.reset();
	g_rec_session.reset();
#endif
	g_model_ready.store(false, std::memory_order_release);
	g_tried_load = false;
	set_status("Child OCR stopped");
}

extern "C" uint32_t ocr_backend_last_error(void)
{
	return g_model_ready.load(std::memory_order_acquire) ? 0u : 1u;
}

extern "C" int ocr_backend_submit(const uint8_t *bgra, uint32_t width, uint32_t height, uint32_t stride,
				  uint64_t *out_job_id)
{
	if (!bgra || width == 0 || height == 0)
		return -1;

	if (!ocr_backend_available())
		return -1;

	const uint64_t job_id = g_next_job_id.fetch_add(1, std::memory_order_relaxed);
	auto job = std::make_unique<ChildOcrWorkItem>();
	job->job_id = job_id;

	if (copy_pixels_to_job(*job, bgra, width, height, stride) != 0)
		return -1;

	{
		std::lock_guard<std::mutex> lock(g_work_mutex);

		if (g_pending_job)
			return -1;

		g_completed_job.reset();
		g_pending_job = std::move(job);
	}

	g_work_cv.notify_one();

	if (out_job_id)
		*out_job_id = job_id;
	return 0;
}

extern "C" int ocr_backend_poll(uint64_t job_id, irlsafety_ocr_hit_list *out_hits)
{
	if (!out_hits)
		return -1;

	std::lock_guard<std::mutex> lock(g_work_mutex);

	if (!g_completed_job || g_completed_job->job_id != job_id)
		return 0;

	*out_hits = g_completed_job->hits;
	return g_completed_job->result == 0 ? 1 : -1;
}

extern "C" int ocr_backend_recognize(const uint8_t *bgra, uint32_t width, uint32_t height, uint32_t stride,
				     irlsafety_ocr_hit_list *out_hits)
{
	uint64_t job_id = 0;

	if (!out_hits)
		return -1;

	if (ocr_backend_submit(bgra, width, height, stride, &job_id) != 0)
		return -1;

	{
		std::unique_lock<std::mutex> lock(g_work_mutex);
		const bool finished = g_done_cv.wait_for(lock, std::chrono::seconds(10), [job_id]() {
			return g_completed_job && g_completed_job->job_id == job_id;
		});

		if (!finished || !g_completed_job || g_completed_job->job_id != job_id)
			return -1;

		*out_hits = g_completed_job->hits;
		return g_completed_job->result;
	}
}

extern "C" const char *ocr_backend_status_message(void)
{
	std::lock_guard<std::mutex> lock(g_mutex);
	return g_status;
}