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

/* Map OCR hits to blur regions where text matches any custom PII entry. */
int irlsafety_match_custom_pii_hits(const irlsafety_ocr_hit_list *hits, const irlsafety_custom_pii_list *custom_pii,
				    float scale_x, float scale_y, irlsafety_region_list *out_regions);

#ifdef __cplusplus
}
#endif