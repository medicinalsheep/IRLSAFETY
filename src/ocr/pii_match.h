/*
 * IRLSAFETY+ — case-insensitive custom PII text matching.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#pragma once

#include <stdbool.h>

#include "../custom_pii.h"
#include "ocr_text.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Returns true if needle appears in haystack (case-insensitive). */
bool irlsafety_ci_contains(const char *haystack, const char *needle);

/*
 * Fraction of keyword characters matched in observed text (0–1).
 * Counts non-space characters; supports prefix typing and in-string fragments.
 */
float irlsafety_keyword_coverage_ratio(const char *observed, const char *keyword);

/* True when coverage meets or exceeds threshold (1.0 = full match only). */
bool irlsafety_partial_pii_matches(const char *observed, const char *keyword, float threshold);

/* Map OCR hits to blur regions where text matches any custom PII entry. */
int irlsafety_match_custom_pii_hits(const irlsafety_ocr_hit_list *hits, const irlsafety_custom_pii_list *custom_pii,
				    float scale_x, float scale_y, float partial_pii_threshold,
				    irlsafety_region_list *out_regions);

/* Map OCR word hits to screen-text censor regions (scaled to full frame coordinates). */
int irlsafety_regions_from_screen_text_hits(const irlsafety_ocr_hit_list *hits, float scale_x, float scale_y,
					    irlsafety_region_list *out_regions);

#ifdef __cplusplus
}
#endif