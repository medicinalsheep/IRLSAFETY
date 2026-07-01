/*
 * IRLSAFETY+ — case-insensitive custom PII text matching.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "pii_match.h"

#include <ctype.h>
#include <string.h>

#define IRLSAFETY_REGION_PADDING 0.12f

static char to_lower_char(char c)
{
	return (char)tolower((unsigned char)c);
}

bool irlsafety_ci_contains(const char *haystack, const char *needle)
{
	size_t needle_len;
	size_t i;

	if (!haystack || !needle)
		return false;

	needle_len = strlen(needle);
	if (needle_len == 0)
		return false;

	for (i = 0; haystack[i] != '\0'; i++) {
		size_t j = 0;
		while (j < needle_len && haystack[i + j] != '\0' &&
		       to_lower_char(haystack[i + j]) == to_lower_char(needle[j]))
			j++;

		if (j == needle_len)
			return true;
	}

	return false;
}

static bool hit_matches_custom_pii(const irlsafety_ocr_hit *hit, const irlsafety_custom_pii_list *custom_pii)
{
	for (size_t i = 0; i < custom_pii->count; i++) {
		const char *entry = custom_pii->entries[i];
		if (entry[0] == '\0')
			continue;

		if (irlsafety_ci_contains(hit->text, entry))
			return true;
		if (irlsafety_ci_contains(entry, hit->text))
			return true;
	}
	return false;
}

static void expand_rect(irlsafety_rect *rect)
{
	float pad_x = rect->width * IRLSAFETY_REGION_PADDING;
	float pad_y = rect->height * IRLSAFETY_REGION_PADDING;

	rect->x -= pad_x;
	rect->y -= pad_y;
	rect->width += pad_x * 2.0f;
	rect->height += pad_y * 2.0f;

	if (rect->x < 0.0f)
		rect->x = 0.0f;
	if (rect->y < 0.0f)
		rect->y = 0.0f;
}

int irlsafety_match_custom_pii_hits(const irlsafety_ocr_hit_list *hits, const irlsafety_custom_pii_list *custom_pii,
				    float scale_x, float scale_y, irlsafety_region_list *out_regions)
{
	if (!hits || !custom_pii || !out_regions)
		return -1;

	out_regions->count = 0;

	for (size_t i = 0; i < hits->count && out_regions->count < IRLSAFETY_MAX_REGIONS; i++) {
		const irlsafety_ocr_hit *hit = &hits->hits[i];
		irlsafety_rect rect;

		if (!hit_matches_custom_pii(hit, custom_pii))
			continue;

		rect.x = hit->x * scale_x;
		rect.y = hit->y * scale_y;
		rect.width = hit->width * scale_x;
		rect.height = hit->height * scale_y;
		rect.confidence = 1.0f;
		expand_rect(&rect);

		out_regions->regions[out_regions->count++] = rect;
	}

	return 0;
}