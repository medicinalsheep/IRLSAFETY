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
#include <winrt/Windows.Storage.Streams.h>

#include <cstring>
#include <mutex>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media::Ocr;
using namespace Windows::Storage::Streams;

static std::once_flag g_winrt_init_flag;
static bool g_winrt_ready = false;
static OcrEngine g_ocr_engine{nullptr};

static void ensure_winrt(void)
{
	std::call_once(g_winrt_init_flag, []() {
		try {
			init_apartment(apartment_type::multi_threaded);
			g_ocr_engine = OcrEngine::TryCreateFromUserProfileLanguages();
			g_winrt_ready = g_ocr_engine != nullptr;
		} catch (...) {
			g_winrt_ready = false;
		}
	});
}

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

extern "C" bool ocr_backend_available(void)
{
	ensure_winrt();
	return g_winrt_ready;
}

extern "C" int ocr_backend_recognize(const uint8_t *bgra, uint32_t width, uint32_t height, uint32_t stride,
				    irlsafety_ocr_hit_list *out_hits)
{
	if (!bgra || !out_hits || width == 0 || height == 0)
		return -1;

	ensure_winrt();
	if (!g_winrt_ready)
		return -1;

	out_hits->count = 0;

	try {
		const uint32_t row_bytes = width * 4;
		Buffer buffer(row_bytes * height);
		uint8_t *dest = buffer.data();

		if (stride == row_bytes) {
			memcpy(dest, bgra, row_bytes * height);
		} else {
			for (uint32_t y = 0; y < height; y++)
				memcpy(dest + y * row_bytes, bgra + y * stride, row_bytes);
		}

		SoftwareBitmap bitmap =
			SoftwareBitmap::CreateCopyFromBuffer(buffer, BitmapPixelFormat::Bgra8, width, height,
							     BitmapAlphaMode::Ignore);

		OcrResult result = g_ocr_engine.RecognizeAsync(bitmap).get();
		IVectorView<OcrLine> lines = result.Lines();
		uint32_t line_count = lines.Size();

		for (uint32_t li = 0; li < line_count; li++) {
			OcrLine line = lines.GetAt(li);
			IVectorView<OcrWord> words = line.Words();
			uint32_t word_count = words.Size();

			for (uint32_t wi = 0; wi < word_count; wi++) {
				OcrWord word = words.GetAt(wi);

				if (out_hits->count >= IRLSAFETY_OCR_MAX_HITS)
					return 0;

				irlsafety_ocr_hit *hit = &out_hits->hits[out_hits->count];
				Rect rect = word.BoundingRect();

				hit->x = (float)rect.X;
				hit->y = (float)rect.Y;
				hit->width = (float)rect.Width;
				hit->height = (float)rect.Height;
				wide_to_utf8(word.Text(), hit->text, sizeof(hit->text));
				out_hits->count++;
			}
		}
	} catch (...) {
		return -1;
	}

	return 0;
}