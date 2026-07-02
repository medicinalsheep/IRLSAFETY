/*
 * IRLSAFETY+ — built-in sensitive text pattern detection (OCR hits).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include <stdbool.h>

#include "../irlsafety_types.h"
#include "ocr_text.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum irlsafety_sensitive_pattern_kind {
	IRLSAFETY_PATTERN_NONE = 0,
	IRLSAFETY_PATTERN_CREDIT_CARD,
	IRLSAFETY_PATTERN_TRACKING_NUMBER,
	IRLSAFETY_PATTERN_SSN,
	IRLSAFETY_PATTERN_PHONE,
	IRLSAFETY_PATTERN_LONG_ID_SEQUENCE,
} irlsafety_sensitive_pattern_kind;

/* Returns first matching pattern kind found in text, or NONE. */
irlsafety_sensitive_pattern_kind irlsafety_sensitive_pattern_classify(const char *text);

/* True when OCR hit text matches any built-in sensitive pattern. */
bool irlsafety_text_has_sensitive_pattern(const char *text);

/* Map OCR hits to censor regions for built-in sensitive patterns. */
int irlsafety_match_sensitive_pattern_hits(const irlsafety_ocr_hit_list *hits, float scale_x, float scale_y,
					     float overlay_overlap, irlsafety_region_list *out_regions);

#ifdef __cplusplus
}
#endif