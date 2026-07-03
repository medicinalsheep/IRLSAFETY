/*
 * IRLSAFETY+ — OCR backend router (Windows OCR vs child ONNX toggle, P8).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "ocr_backend.h"
#include "ocr_backend_impl.h"

#include <stdbool.h>

static bool g_use_child = false;

void ocr_backend_set_use_child(bool use_child)
{
	g_use_child = use_child;
}

bool ocr_backend_using_child(void)
{
#if defined(IRLSAFETY_OCR_HAS_CHILD)
	if (g_use_child && ocr_child_available())
		return true;
#endif
	return false;
}

static bool route_child(void)
{
#if defined(IRLSAFETY_OCR_HAS_CHILD) && defined(IRLSAFETY_OCR_HAS_WINDOWS)
	return g_use_child && ocr_child_available();
#elif defined(IRLSAFETY_OCR_HAS_CHILD)
	(void)g_use_child;
	return ocr_child_available();
#else
	(void)g_use_child;
	return false;
#endif
}

extern "C" void ocr_backend_configure(const char *model_path)
{
#if defined(IRLSAFETY_OCR_HAS_CHILD)
	ocr_child_configure(model_path);
#endif
#if defined(IRLSAFETY_OCR_HAS_WINDOWS)
	ocr_win_configure(model_path);
#else
	(void)model_path;
#endif
}

extern "C" void ocr_backend_configure_models(const char *det_path, const char *rec_path)
{
#if defined(IRLSAFETY_OCR_HAS_CHILD)
	ocr_child_configure_models(det_path, rec_path);
#endif
#if defined(IRLSAFETY_OCR_HAS_WINDOWS)
	ocr_win_configure_models(det_path, rec_path);
#else
	(void)det_path;
	(void)rec_path;
#endif
}

extern "C" const char *ocr_backend_name(void)
{
#if defined(IRLSAFETY_OCR_HAS_CHILD)
	if (route_child())
		return ocr_child_name();
#endif
#if defined(IRLSAFETY_OCR_HAS_WINDOWS)
	return ocr_win_name();
#else
	return "OCR unavailable";
#endif
}

extern "C" const char *ocr_backend_status_message(void)
{
#if defined(IRLSAFETY_OCR_HAS_CHILD)
	if (route_child())
		return ocr_child_status_message();
#endif
#if defined(IRLSAFETY_OCR_HAS_WINDOWS)
	return ocr_win_status_message();
#else
	return "OCR backend not built";
#endif
}

extern "C" bool ocr_backend_available(void)
{
#if defined(IRLSAFETY_OCR_HAS_CHILD)
	if (route_child())
		return ocr_child_available();
#endif
#if defined(IRLSAFETY_OCR_HAS_WINDOWS)
	return ocr_win_available();
#else
	return false;
#endif
}

extern "C" void ocr_backend_shutdown(void)
{
#if defined(IRLSAFETY_OCR_HAS_CHILD)
	ocr_child_shutdown();
#endif
#if defined(IRLSAFETY_OCR_HAS_WINDOWS)
	ocr_win_shutdown();
#endif
}

extern "C" uint32_t ocr_backend_last_error(void)
{
#if defined(IRLSAFETY_OCR_HAS_CHILD)
	if (route_child())
		return ocr_child_last_error();
#endif
#if defined(IRLSAFETY_OCR_HAS_WINDOWS)
	return ocr_win_last_error();
#else
	return 0;
#endif
}

extern "C" int ocr_backend_submit(const uint8_t *bgra, uint32_t width, uint32_t height, uint32_t stride,
				  uint64_t *out_job_id)
{
#if defined(IRLSAFETY_OCR_HAS_CHILD)
	if (route_child())
		return ocr_child_submit(bgra, width, height, stride, out_job_id);
#endif
#if defined(IRLSAFETY_OCR_HAS_WINDOWS)
	return ocr_win_submit(bgra, width, height, stride, out_job_id);
#else
	(void)bgra;
	(void)width;
	(void)height;
	(void)stride;
	(void)out_job_id;
	return -1;
#endif
}

extern "C" int ocr_backend_poll(uint64_t job_id, irlsafety_ocr_hit_list *out_hits)
{
#if defined(IRLSAFETY_OCR_HAS_CHILD)
	if (route_child())
		return ocr_child_poll(job_id, out_hits);
#endif
#if defined(IRLSAFETY_OCR_HAS_WINDOWS)
	return ocr_win_poll(job_id, out_hits);
#else
	(void)job_id;
	(void)out_hits;
	return -1;
#endif
}

extern "C" int ocr_backend_recognize(const uint8_t *bgra, uint32_t width, uint32_t height, uint32_t stride,
				    irlsafety_ocr_hit_list *out_hits)
{
#if defined(IRLSAFETY_OCR_HAS_CHILD)
	if (route_child())
		return ocr_child_recognize(bgra, width, height, stride, out_hits);
#endif
#if defined(IRLSAFETY_OCR_HAS_WINDOWS)
	return ocr_win_recognize(bgra, width, height, stride, out_hits);
#else
	(void)bgra;
	(void)width;
	(void)height;
	(void)stride;
	(void)out_hits;
	return -1;
#endif
}