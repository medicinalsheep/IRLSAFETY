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

bool ocr_backend_available(void);
int ocr_backend_recognize(const uint8_t *bgra, uint32_t width, uint32_t height, uint32_t stride,
			  irlsafety_ocr_hit_list *out_hits);

#ifdef __cplusplus
}
#endif