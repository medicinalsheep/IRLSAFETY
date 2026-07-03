/*
 * IRLSAFETY+ — Windows.Media.Ocr backend (Windows 10+).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "ocr_backend.h"

#include <Windows.h>

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Storage.Streams.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media::Ocr;
using namespace Windows::Security::Cryptography;
using namespace Windows::Storage::Streams;

struct OcrWorkItem {
	std::vector<uint8_t> pixels;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t stride = 0;
	irlsafety_ocr_hit_list hits{};
	int result = -1;
	uint64_t job_id = 0;
};

static std::once_flag g_worker_start_flag;
static std::mutex g_work_mutex;
static std::condition_variable g_work_cv;
static std::condition_variable g_done_cv;
static std::unique_ptr<OcrWorkItem> g_pending_job;
static std::unique_ptr<OcrWorkItem> g_completed_job;
static std::thread g_worker_thread;
static std::atomic<bool> g_worker_running{false};
static std::atomic<bool> g_winrt_ready{false};
static std::atomic<uint32_t> g_last_hresult{0};
static std::atomic<uint64_t> g_next_job_id{1};

static void wide_to_utf8(hstring const &wide, char *out, size_t out_len)
{
	if (!out || out_len == 0)
		return;

	std::wstring_view view(wide.c_str(), wide.size());
	int needed = WideCharToMultiByte(CP_UTF8, 0, view.data(), (int)view.size(), nullptr, 0, nullptr, nullptr);
	if (needed <= 0) {
		out[0] = '\0';
		return;
	}

	if ((size_t)needed >= out_len)
		needed = (int)out_len - 1;

	WideCharToMultiByte(CP_UTF8, 0, view.data(), (int)view.size(), out, needed, nullptr, nullptr);
	out[needed] = '\0';
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

static SoftwareBitmap create_bitmap_from_pixels(const OcrWorkItem &work)
{
	const uint32_t row_bytes = work.width * 4;
	const size_t needed = (size_t)row_bytes * (size_t)work.height;

	if (work.pixels.size() < needed)
		throw winrt::hresult_error(E_INVALIDARG);

	std::vector<uint8_t> packed(needed);
	if (work.stride == row_bytes) {
		memcpy(packed.data(), work.pixels.data(), needed);
	} else {
		for (uint32_t y = 0; y < work.height; y++)
			memcpy(packed.data() + (size_t)y * row_bytes, work.pixels.data() + (size_t)y * work.stride,
			       row_bytes);
	}

	array_view<uint8_t const> const view(packed.data(), static_cast<uint32_t>(packed.size()));
	IBuffer buffer = CryptographicBuffer::CreateFromByteArray(view);
	return SoftwareBitmap::CreateCopyFromBuffer(buffer, BitmapPixelFormat::Bgra8, work.width, work.height,
						      BitmapAlphaMode::Ignore);
}

static int recognize_on_worker(OcrEngine const &engine, const OcrWorkItem &work, irlsafety_ocr_hit_list *out_hits)
{
	if (!out_hits || work.pixels.empty() || work.width == 0 || work.height == 0)
		return -1;

	memset(out_hits, 0, sizeof(*out_hits));

	try {
		SoftwareBitmap bitmap = create_bitmap_from_pixels(work);
		OcrResult result = engine.RecognizeAsync(bitmap).get();
		IVectorView<OcrLine> lines = result.Lines();
		uint32_t line_count = lines.Size();

		for (uint32_t li = 0; li < line_count; li++) {
			OcrLine line = lines.GetAt(li);
			IVectorView<OcrWord> words = line.Words();
			uint32_t word_count = words.Size();
			char line_text[IRLSAFETY_OCR_TEXT_LEN];
			float line_x = 0.0f;
			float line_y = 0.0f;
			float line_x2 = 0.0f;
			float line_y2 = 0.0f;
			bool have_line_bounds = false;

			wide_to_utf8(line.Text(), line_text, sizeof(line_text));
			if (line_text[0] != '\0') {
				for (uint32_t wi = 0; wi < word_count; wi++) {
					Rect rect = words.GetAt(wi).BoundingRect();
					if (!have_line_bounds) {
						line_x = (float)rect.X;
						line_y = (float)rect.Y;
						line_x2 = line_x + (float)rect.Width;
						line_y2 = line_y + (float)rect.Height;
						have_line_bounds = true;
					} else {
						float x2 = (float)rect.X + (float)rect.Width;
						float y2 = (float)rect.Y + (float)rect.Height;
						if ((float)rect.X < line_x)
							line_x = (float)rect.X;
						if ((float)rect.Y < line_y)
							line_y = (float)rect.Y;
						if (x2 > line_x2)
							line_x2 = x2;
						if (y2 > line_y2)
							line_y2 = y2;
					}
				}

				if (have_line_bounds)
					push_hit(out_hits, line_text, line_x, line_y, line_x2 - line_x, line_y2 - line_y);
			}

			for (uint32_t wi = 0; wi < word_count; wi++) {
				OcrWord word = words.GetAt(wi);
				Rect rect = word.BoundingRect();
				char word_text[IRLSAFETY_OCR_TEXT_LEN];

				wide_to_utf8(word.Text(), word_text, sizeof(word_text));
				push_hit(out_hits, word_text, (float)rect.X, (float)rect.Y, (float)rect.Width,
					 (float)rect.Height);
			}
		}
	} catch (winrt::hresult_error const &err) {
		g_last_hresult.store((uint32_t)err.code().value, std::memory_order_relaxed);
		return -1;
	} catch (...) {
		g_last_hresult.store(0x80004005u, std::memory_order_relaxed);
		return -1;
	}

	return 0;
}

static void ocr_worker_loop(void)
{
	try {
		init_apartment(apartment_type::multi_threaded);
	} catch (...) {
		g_winrt_ready.store(false, std::memory_order_release);
		return;
	}

	OcrEngine engine{nullptr};
	try {
		engine = OcrEngine::TryCreateFromUserProfileLanguages();
	} catch (...) {
		engine = nullptr;
	}

	if (!engine) {
		g_winrt_ready.store(false, std::memory_order_release);
		return;
	}

	g_winrt_ready.store(true, std::memory_order_release);

	while (g_worker_running.load(std::memory_order_acquire)) {
		OcrWorkItem job;

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

		job.result = recognize_on_worker(engine, job, &job.hits);

		{
			std::lock_guard<std::mutex> lock(g_work_mutex);
			g_completed_job = std::make_unique<OcrWorkItem>(std::move(job));
		}
		g_done_cv.notify_all();
	}
}

static void ensure_ocr_worker(void)
{
	std::call_once(g_worker_start_flag, []() {
		g_worker_running.store(true, std::memory_order_release);
		g_worker_thread = std::thread(ocr_worker_loop);
	});
}

extern "C" const char *ocr_win_name(void)
{
	return "Windows OCR (local)";
}

extern "C" const char *ocr_win_status_message(void)
{
	if (g_winrt_ready.load(std::memory_order_acquire))
		return "Windows.Media.Ocr ready";
	return "Windows OCR initializing or unavailable";
}

extern "C" bool ocr_win_available(void)
{
	ensure_ocr_worker();

	for (int i = 0; i < 100; i++) {
		if (g_winrt_ready.load(std::memory_order_acquire))
			return true;
		Sleep(10);
	}

	return g_winrt_ready.load(std::memory_order_acquire);
}

extern "C" void ocr_win_configure(const char *model_path)
{
	(void)model_path;
}

extern "C" void ocr_win_configure_models(const char *det_path, const char *rec_path)
{
	(void)det_path;
	(void)rec_path;
}

extern "C" void ocr_win_shutdown(void)
{
	if (!g_worker_running.exchange(false))
		return;

	g_work_cv.notify_all();
	if (g_worker_thread.joinable())
		g_worker_thread.join();

	std::lock_guard<std::mutex> lock(g_work_mutex);
	g_pending_job.reset();
	g_completed_job.reset();
}

extern "C" uint32_t ocr_win_last_error(void)
{
	return g_last_hresult.load(std::memory_order_relaxed);
}

static int copy_pixels_to_job(OcrWorkItem &job, const uint8_t *bgra, uint32_t width, uint32_t height, uint32_t stride)
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

extern "C" int ocr_win_submit(const uint8_t *bgra, uint32_t width, uint32_t height, uint32_t stride,
				  uint64_t *out_job_id)
{
	if (!bgra || width == 0 || height == 0)
		return -1;

	ensure_ocr_worker();
	if (!g_winrt_ready.load(std::memory_order_acquire))
		return -1;

	const uint64_t job_id = g_next_job_id.fetch_add(1, std::memory_order_relaxed);
	auto job = std::make_unique<OcrWorkItem>();
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

extern "C" int ocr_win_poll(uint64_t job_id, irlsafety_ocr_hit_list *out_hits)
{
	if (!out_hits)
		return -1;

	std::lock_guard<std::mutex> lock(g_work_mutex);

	if (!g_completed_job || g_completed_job->job_id != job_id)
		return 0;

	*out_hits = g_completed_job->hits;
	return g_completed_job->result == 0 ? 1 : -1;
}

extern "C" int ocr_win_recognize(const uint8_t *bgra, uint32_t width, uint32_t height, uint32_t stride,
				    irlsafety_ocr_hit_list *out_hits)
{
	uint64_t job_id = 0;

	if (!out_hits)
		return -1;

	if (ocr_win_submit(bgra, width, height, stride, &job_id) != 0)
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