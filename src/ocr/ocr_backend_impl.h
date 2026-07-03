/*
 * IRLSAFETY+ — internal OCR backend implementations (Windows / child ONNX).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "ocr_text.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ocr_win_configure(const char *model_path);
void ocr_win_configure_models(const char *det_path, const char *rec_path);
bool ocr_win_available(void);
const char *ocr_win_name(void);
const char *ocr_win_status_message(void);
void ocr_win_shutdown(void);
uint32_t ocr_win_last_error(void);
int ocr_win_submit(const uint8_t *bgra, uint32_t width, uint32_t height, uint32_t stride, uint64_t *out_job_id);
int ocr_win_poll(uint64_t job_id, irlsafety_ocr_hit_list *out_hits);
int ocr_win_recognize(const uint8_t *bgra, uint32_t width, uint32_t height, uint32_t stride,
		      irlsafety_ocr_hit_list *out_hits);

void ocr_child_configure(const char *model_path);
void ocr_child_configure_models(const char *det_path, const char *rec_path);
bool ocr_child_available(void);
const char *ocr_child_name(void);
const char *ocr_child_status_message(void);
void ocr_child_shutdown(void);
uint32_t ocr_child_last_error(void);
int ocr_child_submit(const uint8_t *bgra, uint32_t width, uint32_t height, uint32_t stride, uint64_t *out_job_id);
int ocr_child_poll(uint64_t job_id, irlsafety_ocr_hit_list *out_hits);
int ocr_child_recognize(const uint8_t *bgra, uint32_t width, uint32_t height, uint32_t stride,
			irlsafety_ocr_hit_list *out_hits);

#ifdef __cplusplus
}
#endif