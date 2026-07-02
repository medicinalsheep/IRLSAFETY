/*
 * IRLSAFETY+ — platform OCR backend interface.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "ocr_text.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Optional: set ONNX model path before first use (child OCR backend). */
void ocr_backend_configure(const char *model_path);

/* Child OCR: det + rec ONNX pair (irlsafety-ocr-det.onnx + irlsafety-ocr-rec.onnx). */
void ocr_backend_configure_models(const char *det_path, const char *rec_path);

bool ocr_backend_available(void);
const char *ocr_backend_name(void);
const char *ocr_backend_status_message(void);
void ocr_backend_shutdown(void);
uint32_t ocr_backend_last_error(void);
int ocr_backend_recognize(const uint8_t *bgra, uint32_t width, uint32_t height, uint32_t stride,
			  irlsafety_ocr_hit_list *out_hits);

/* Non-blocking OCR — submit returns 0 on success, -1 if busy or unavailable. */
int ocr_backend_submit(const uint8_t *bgra, uint32_t width, uint32_t height, uint32_t stride, uint64_t *out_job_id);

/* Poll: 1 = result ready (out_hits filled), 0 = still running, -1 = no matching job / error. */
int ocr_backend_poll(uint64_t job_id, irlsafety_ocr_hit_list *out_hits);

#ifdef __cplusplus
}
#endif