/*
 * IRLSAFETY+ — case-insensitive custom PII text matching.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "pii_match.h"

#include <ctype.h>
#include <math.h>
#include <string.h>

#define IRLSAFETY_MIN_REGION_PX 3.0f
#define IRLSAFETY_SMALL_TEXT_HEIGHT 18.0f
#define IRLSAFETY_ROLLING_LINE_TOLERANCE 0.55f
#define IRLSAFETY_ROLLING_GAP_FACTOR 1.8f

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

static bool ci_equal(const char *a, const char *b)
{
	if (!a || !b)
		return false;

	while (*a && *b) {
		if (to_lower_char(*a) != to_lower_char(*b))
			return false;
		a++;
		b++;
	}

	return *a == '\0' && *b == '\0';
}

static bool keyword_is_phrase(const char *keyword)
{
	return keyword && strchr(keyword, ' ') != NULL;
}

static bool keyword_is_compound(const char *keyword)
{
	return keyword && (strchr(keyword, '@') != NULL || strchr(keyword, '.') != NULL ||
			   strchr(keyword, '/') != NULL || strchr(keyword, '-') != NULL);
}

static bool hit_is_line_level(const irlsafety_ocr_hit *hit)
{
	size_t len;

	if (!hit || hit->text[0] == '\0')
		return false;

	if (strchr(hit->text, ' ') != NULL)
		return true;

	len = strlen(hit->text);
	return len >= 12;
}

static size_t significant_char_count(const char *text)
{
	size_t count = 0;

	if (!text)
		return 0;

	for (; *text; text++) {
		if (!isspace((unsigned char)*text))
			count++;
	}

	return count;
}

static bool ci_prefix_match_len(const char *observed, const char *prefix, size_t *out_len)
{
	size_t i = 0;

	if (!observed || !prefix || !out_len)
		return false;

	while (prefix[i] != '\0' && observed[i] != '\0') {
		if (to_lower_char(observed[i]) != to_lower_char(prefix[i]))
			return false;
		i++;
	}

	if (prefix[i] != '\0')
		return false;

	*out_len = i;
	return true;
}

static size_t significant_prefix_len(const char *text, size_t char_len)
{
	size_t count = 0;
	size_t i;

	if (!text)
		return 0;

	for (i = 0; i < char_len && text[i] != '\0'; i++) {
		if (!isspace((unsigned char)text[i]))
			count++;
	}

	return count;
}

float irlsafety_keyword_coverage_ratio(const char *observed, const char *keyword)
{
	size_t observed_len;
	size_t keyword_sig;
	size_t best_sig = 0;
	size_t prefix_len;
	size_t i;

	if (!observed || !keyword || keyword[0] == '\0')
		return 0.0f;

	keyword_sig = significant_char_count(keyword);
	if (keyword_sig == 0)
		return 0.0f;

	observed_len = strlen(observed);

	if (ci_equal(observed, keyword) || irlsafety_ci_contains(observed, keyword))
		return 1.0f;

	if (ci_prefix_match_len(observed, keyword, &prefix_len))
		return 1.0f;

	if (ci_prefix_match_len(keyword, observed, &prefix_len)) {
		best_sig = significant_prefix_len(keyword, prefix_len);
		return (float)best_sig / (float)keyword_sig;
	}

	for (i = 0; i < observed_len; i++) {
		size_t j = 0;

		while (observed[i + j] != '\0' && keyword[j] != '\0' &&
		       to_lower_char(observed[i + j]) == to_lower_char(keyword[j]))
			j++;

		if (j == 0)
			continue;

		prefix_len = significant_prefix_len(keyword, j);
		if (prefix_len > best_sig)
			best_sig = prefix_len;
	}

	if (best_sig == 0)
		return 0.0f;

	return (float)best_sig / (float)keyword_sig;
}

bool irlsafety_partial_pii_matches(const char *observed, const char *keyword, float threshold)
{
	float coverage;

	if (!observed || !keyword || keyword[0] == '\0')
		return false;

	if (threshold >= 1.0f)
		return irlsafety_ci_contains(observed, keyword);

	coverage = irlsafety_keyword_coverage_ratio(observed, keyword);
	return coverage >= threshold;
}

static bool is_typing_cover_threshold(float partial_pii_threshold)
{
	return partial_pii_threshold <= 0.02f;
}

static bool hit_matches_entry(const irlsafety_ocr_hit *hit, const char *entry, float partial_pii_threshold)
{
	float coverage;
	size_t prefix_len = 0;

	if (!hit || !entry || entry[0] == '\0')
		return false;

	if (partial_pii_threshold >= 1.0f) {
		if (!irlsafety_ci_contains(hit->text, entry))
			return false;
	} else {
		coverage = irlsafety_keyword_coverage_ratio(hit->text, entry);
		if (coverage < partial_pii_threshold)
			return false;
	}

	if (keyword_is_phrase(entry) || keyword_is_compound(entry)) {
		if (hit_is_line_level(hit))
			return true;
		if (is_typing_cover_threshold(partial_pii_threshold))
			return ci_prefix_match_len(entry, hit->text, &prefix_len) && prefix_len > 0;
		return partial_pii_threshold < 1.0f &&
		       irlsafety_keyword_coverage_ratio(hit->text, entry) >= partial_pii_threshold;
	}

	if (!hit_is_line_level(hit))
		return ci_equal(hit->text, entry) || irlsafety_ci_contains(hit->text, entry) ||
		       irlsafety_keyword_coverage_ratio(hit->text, entry) >= partial_pii_threshold;

	return ci_equal(hit->text, entry) || irlsafety_keyword_coverage_ratio(hit->text, entry) >= partial_pii_threshold;
}

static float effective_overlap_ratio(float overlap_ratio, float text_height)
{
	float ratio = overlap_ratio;

	if (ratio < 0.05f)
		ratio = 0.05f;
	if (ratio > 0.95f)
		ratio = 0.95f;

	if (text_height < IRLSAFETY_SMALL_TEXT_HEIGHT)
		ratio *= 1.45f;
	if (ratio > 0.95f)
		ratio = 0.95f;

	return ratio;
}

static void expand_rect(irlsafety_rect *rect, float overlap_ratio, bool horizontal_boost)
{
	float pad_ratio = effective_overlap_ratio(overlap_ratio, rect->height);
	float pad_x = rect->width * pad_ratio;
	float pad_y = rect->height * pad_ratio;
	float angled = (rect->width > rect->height ? rect->width : rect->height) * pad_ratio * 0.35f;

	if (horizontal_boost)
		pad_x *= 1.85f;

	rect->x -= pad_x + angled;
	rect->y -= pad_y + angled;
	rect->width += (pad_x + angled) * 2.0f;
	rect->height += (pad_y + angled) * 2.0f;

	if (rect->x < 0.0f)
		rect->x = 0.0f;
	if (rect->y < 0.0f)
		rect->y = 0.0f;
}

static bool region_large_enough(const irlsafety_rect *rect)
{
	return rect && rect->width >= IRLSAFETY_MIN_REGION_PX && rect->height >= IRLSAFETY_MIN_REGION_PX;
}

static bool rect_contains_hit(const irlsafety_rect *rect, const irlsafety_ocr_hit *hit, float scale_x, float scale_y)
{
	float hx = hit->x * scale_x;
	float hy = hit->y * scale_y;
	float hw = hit->width * scale_x;
	float hh = hit->height * scale_y;
	float cx = hx + hw * 0.5f;
	float cy = hy + hh * 0.5f;

	return cx >= rect->x && cx <= (rect->x + rect->width) && cy >= rect->y && cy <= (rect->y + rect->height);
}

static void append_scaled_region(irlsafety_region_list *out_regions, const irlsafety_ocr_hit *hit, float scale_x,
				 float scale_y, float overlap_ratio, bool horizontal_boost, float coverage_ratio)
{
	irlsafety_rect rect;
	float width_scale = 1.0f;

	rect.x = hit->x * scale_x;
	rect.y = hit->y * scale_y;
	rect.width = hit->width * scale_x;
	rect.height = hit->height * scale_y;
	rect.confidence = 1.0f;

	if (coverage_ratio > 0.05f && coverage_ratio < 0.98f)
		width_scale = 1.0f / coverage_ratio;
	if (width_scale > 3.5f)
		width_scale = 3.5f;
	rect.width *= width_scale;

	expand_rect(&rect, overlap_ratio, horizontal_boost);

	if (!region_large_enough(&rect))
		return;

	out_regions->regions[out_regions->count++] = rect;
}

static bool hits_share_rolling_line(const irlsafety_ocr_hit *a, const irlsafety_ocr_hit *b, float scale_x,
				    float scale_y)
{
	float ay = a->y * scale_y;
	float by = b->y * scale_y;
	float ah = a->height * scale_y;
	float bh = b->height * scale_y;
	float line_h = ah > bh ? ah : bh;
	float y_tol = line_h * IRLSAFETY_ROLLING_LINE_TOLERANCE;

	return fabsf(ay - by) <= y_tol;
}

static bool hits_are_rolling_neighbors(const irlsafety_ocr_hit *left, const irlsafety_ocr_hit *right, float scale_x,
				       float scale_y)
{
	float left_x = left->x * scale_x;
	float left_w = left->width * scale_x;
	float right_x = right->x * scale_x;
	float gap = right_x - (left_x + left_w);
	float gap_limit = (left->width * scale_x > right->width * scale_x ? left->width * scale_x
									  : right->width * scale_x) *
			  IRLSAFETY_ROLLING_GAP_FACTOR;

	if (!hits_share_rolling_line(left, right, scale_x, scale_y))
		return false;

	return gap >= -left_w * 0.25f && gap <= gap_limit;
}

static void merge_hit_bounds(const irlsafety_ocr_hit *hit, float scale_x, float scale_y, float *min_x, float *min_y,
			     float *max_x, float *max_y)
{
	float x = hit->x * scale_x;
	float y = hit->y * scale_y;
	float x2 = x + hit->width * scale_x;
	float y2 = y + hit->height * scale_y;

	if (x < *min_x)
		*min_x = x;
	if (y < *min_y)
		*min_y = y;
	if (x2 > *max_x)
		*max_x = x2;
	if (y2 > *max_y)
		*max_y = y2;
}

static void merge_rolling_word_hits(const irlsafety_ocr_hit_list *hits, float scale_x, float scale_y,
				    irlsafety_ocr_hit_list *merged)
{
	bool used[IRLSAFETY_OCR_MAX_HITS];
	size_t i;

	if (!hits || !merged)
		return;

	memset(used, 0, sizeof(used));
	merged->count = 0;

	for (i = 0; i < hits->count; i++) {
		irlsafety_ocr_hit group;
		float min_x;
		float min_y;
		float max_x;
		float max_y;
		size_t group_count = 0;
		bool expanded;

		if (used[i] || hits->hits[i].text[0] == '\0' || hit_is_line_level(&hits->hits[i]))
			continue;

		min_x = hits->hits[i].x * scale_x;
		min_y = hits->hits[i].y * scale_y;
		max_x = min_x + hits->hits[i].width * scale_x;
		max_y = min_y + hits->hits[i].height * scale_y;
		used[i] = true;
		group_count = 1;

		do {
			expanded = false;
			for (size_t j = 0; j < hits->count; j++) {
				const irlsafety_ocr_hit *candidate = &hits->hits[j];
				bool touches_group = false;

				if (used[j] || candidate->text[0] == '\0' || hit_is_line_level(candidate))
					continue;

				for (size_t k = 0; k < hits->count; k++) {
					if (!used[k] || k == j)
						continue;
					if (hits_are_rolling_neighbors(&hits->hits[k], candidate, scale_x, scale_y)) {
						touches_group = true;
						break;
					}
				}

				if (!touches_group)
					continue;

				merge_hit_bounds(candidate, scale_x, scale_y, &min_x, &min_y, &max_x, &max_y);
				used[j] = true;
				group_count++;
				expanded = true;
			}
		} while (expanded);

		if (group_count < 2 || merged->count >= IRLSAFETY_OCR_MAX_HITS)
			continue;

		group = hits->hits[i];
		group.x = min_x / scale_x;
		group.y = min_y / scale_y;
		group.width = (max_x - min_x) / scale_x;
		group.height = (max_y - min_y) / scale_y;
		strncpy(group.text, "rolling", sizeof(group.text) - 1);
		merged->hits[merged->count++] = group;
	}
}

int irlsafety_match_custom_pii_hits(const irlsafety_ocr_hit_list *hits, const irlsafety_custom_pii_list *custom_pii,
				    float scale_x, float scale_y, float overlay_overlap, float partial_pii_threshold,
				    irlsafety_region_list *out_regions)
{
	float threshold = partial_pii_threshold;

	if (!hits || !custom_pii || !out_regions)
		return -1;

	if (threshold < 0.0f)
		threshold = 0.0f;
	if (threshold > 1.0f)
		threshold = 1.0f;

	out_regions->count = 0;

	for (size_t i = 0; i < hits->count && out_regions->count < IRLSAFETY_MAX_REGIONS; i++) {
		const irlsafety_ocr_hit *hit = &hits->hits[i];
		bool matched = false;
		float best_coverage = 0.0f;

		for (size_t k = 0; k < custom_pii->count; k++) {
			float coverage = irlsafety_keyword_coverage_ratio(hit->text, custom_pii->entries[k]);

			if (hit_matches_entry(hit, custom_pii->entries[k], threshold)) {
				matched = true;
				if (coverage > best_coverage)
					best_coverage = coverage;
			}
		}

		if (!matched)
			continue;

		append_scaled_region(out_regions, hit, scale_x, scale_y, overlay_overlap, false,
				     best_coverage > 0.0f ? best_coverage : 1.0f);
	}

	return 0;
}

int irlsafety_regions_from_screen_text_hits(const irlsafety_ocr_hit_list *hits, float scale_x, float scale_y,
					    float overlay_overlap, irlsafety_region_list *out_regions)
{
	irlsafety_ocr_hit_list rolling_merged;

	if (!hits || !out_regions)
		return -1;

	out_regions->count = 0;
	memset(&rolling_merged, 0, sizeof(rolling_merged));
	merge_rolling_word_hits(hits, scale_x, scale_y, &rolling_merged);

	/* Prefer full-line boxes — word-level hits leave gaps in censored text. */
	for (size_t i = 0; i < hits->count && out_regions->count < IRLSAFETY_MAX_REGIONS; i++) {
		const irlsafety_ocr_hit *hit = &hits->hits[i];
		size_t j;
		bool covered = false;

		if (hit->text[0] == '\0' || !hit_is_line_level(hit))
			continue;

		for (j = 0; j < out_regions->count; j++) {
			if (rect_contains_hit(&out_regions->regions[j], hit, scale_x, scale_y)) {
				covered = true;
				break;
			}
		}

		if (covered)
			continue;

		append_scaled_region(out_regions, hit, scale_x, scale_y, overlay_overlap, false, 1.0f);
	}

	/* Merged rolling/ticker fragments (horizontally adjacent words on the same line). */
	for (size_t i = 0; i < rolling_merged.count && out_regions->count < IRLSAFETY_MAX_REGIONS; i++) {
		const irlsafety_ocr_hit *hit = &rolling_merged.hits[i];
		size_t j;
		bool covered = false;

		for (j = 0; j < out_regions->count; j++) {
			if (rect_contains_hit(&out_regions->regions[j], hit, scale_x, scale_y)) {
				covered = true;
				break;
			}
		}

		if (covered)
			continue;

		append_scaled_region(out_regions, hit, scale_x, scale_y, overlay_overlap, true, 1.0f);
	}

	/* Fall back to isolated word boxes when no line hit exists (icons, single labels). */
	for (size_t i = 0; i < hits->count && out_regions->count < IRLSAFETY_MAX_REGIONS; i++) {
		const irlsafety_ocr_hit *hit = &hits->hits[i];
		size_t j;
		bool covered = false;

		if (hit->text[0] == '\0' || hit_is_line_level(hit))
			continue;

		for (j = 0; j < out_regions->count; j++) {
			if (rect_contains_hit(&out_regions->regions[j], hit, scale_x, scale_y)) {
				covered = true;
				break;
			}
		}

		if (covered)
			continue;

		append_scaled_region(out_regions, hit, scale_x, scale_y, overlay_overlap, false, 1.0f);
	}

	return 0;
}