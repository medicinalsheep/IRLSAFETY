/*
 * IRLSAFETY+ — OCR text hit structures.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include "../irlsafety_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IRLSAFETY_OCR_MAX_HITS 128
#define IRLSAFETY_OCR_TEXT_LEN 256

typedef struct irlsafety_ocr_hit {
	char text[IRLSAFETY_OCR_TEXT_LEN];
	float x;
	float y;
	float width;
	float height;
} irlsafety_ocr_hit;

typedef struct irlsafety_ocr_hit_list {
	irlsafety_ocr_hit hits[IRLSAFETY_OCR_MAX_HITS];
	size_t count;
} irlsafety_ocr_hit_list;

#ifdef __cplusplus
}
#endif