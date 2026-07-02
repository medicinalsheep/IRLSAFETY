/*
 * IRLSAFETY+ — built-in sensitive text pattern detection (OCR hits).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "pii_patterns.h"

#include <ctype.h>
#include <string.h>

#define IRLSAFETY_MIN_REGION_PX 3.0f

static bool is_digit_char(char c)
{
	return c >= '0' && c <= '9';
}

static bool is_alnum_char(char c)
{
	return is_digit_char(c) || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static char to_upper_char(char c)
{
	if (c >= 'a' && c <= 'z')
		return (char)(c - 'a' + 'A');
	return c;
}

static bool luhn_valid(const char *digits, size_t len)
{
	size_t sum = 0;
	bool double_it = false;

	if (len < 13 || len > 19)
		return false;

	for (size_t i = len; i > 0; i--) {
		int d = digits[i - 1] - '0';
		if (double_it) {
			d *= 2;
			if (d > 9)
				d -= 9;
		}
		sum += (size_t)d;
		double_it = !double_it;
	}

	return (sum % 10) == 0;
}

static bool looks_like_credit_card(const char *digits, size_t len)
{
	if (!luhn_valid(digits, len))
		return false;

	if (len == 15 && (digits[0] == '3' && (digits[1] == '4' || digits[1] == '7')))
		return true;
	if (len >= 13 && len <= 19 && digits[0] == '4')
		return true;
	if (len == 16 && digits[0] == '5' && digits[1] >= '1' && digits[1] <= '5')
		return true;
	if (len == 16 && digits[0] == '2' && digits[1] == '2')
		return true;
	if (len == 16 && digits[0] == '6' && digits[1] == '0')
		return true;
	if (len >= 14 && len <= 19 && digits[0] == '6' && digits[1] == '5')
		return true;

	/* Any Luhn-valid 13–19 digit block (covers lesser-known networks). */
	return true;
}

static bool match_credit_card_in_text(const char *text)
{
	char digits[32];
	size_t count = 0;

	if (!text)
		return false;

	for (const char *p = text; *p; p++) {
		if (is_digit_char(*p)) {
			if (count < sizeof(digits) - 1)
				digits[count++] = *p;
			if (count >= 13 && looks_like_credit_card(digits, count))
				return true;
			if (count >= sizeof(digits) - 1)
				count = 0;
		} else if (count >= 13) {
			if (looks_like_credit_card(digits, count))
				return true;
			count = 0;
		} else if (!is_digit_char(*p) && *p != ' ' && *p != '-' && *p != '.') {
			count = 0;
		}
	}

	return count >= 13 && looks_like_credit_card(digits, count);
}

static bool match_ssn_in_text(const char *text)
{
	if (!text)
		return false;

	for (const char *p = text; *p; p++) {
		if (!isdigit((unsigned char)p[0]) || !isdigit((unsigned char)p[1]) || !isdigit((unsigned char)p[2]))
			continue;
		if (p[3] != '-' || !isdigit((unsigned char)p[4]) || !isdigit((unsigned char)p[5]))
			continue;
		if (p[6] != '-' || !isdigit((unsigned char)p[7]) || !isdigit((unsigned char)p[8]) ||
		    !isdigit((unsigned char)p[9]) || !isdigit((unsigned char)p[10]))
			continue;

		/* Reject obvious invalid SSN groups (000, 666, 9xx area). */
		if (p[0] == '0' && p[1] == '0' && p[2] == '0')
			continue;
		if (p[0] == '6' && p[1] == '6' && p[2] == '6')
			continue;
		if (p[0] == '9')
			continue;
		if (p[4] == '0' && p[5] == '0')
			continue;
		if (p[7] == '0' && p[8] == '0' && p[9] == '0' && p[10] == '0')
			continue;

		return true;
	}

	return false;
}

static bool match_phone_in_text(const char *text)
{
	if (!text)
		return false;

	for (const char *p = text; *p; p++) {
		if (!isdigit((unsigned char)*p))
			continue;

		const char *q = p;
		int digits = 0;
		while (*q && digits < 11) {
			if (isdigit((unsigned char)*q))
				digits++;
			else if (*q != ' ' && *q != '-' && *q != '.' && *q != '(' && *q != ')')
				break;
			q++;
		}

		if (digits == 10 || digits == 11) {
			int area_first = p[0] - '0';
			if (digits == 11 && *p == '1')
				area_first = p[1] - '0';
			if (area_first >= 2)
				return true;
		}
	}

	return false;
}

static bool match_tracking_in_text(const char *text)
{
	if (!text || text[0] == '\0')
		return false;

	/* UPS: 1Z + 16 alphanumeric */
	for (const char *p = text; *p; p++) {
		if (to_upper_char(p[0]) == '1' && to_upper_char(p[1]) == 'Z') {
			int alnum = 0;
			for (const char *q = p + 2; *q && alnum < 16; q++) {
				if (is_alnum_char(*q))
					alnum++;
				else if (*q != ' ')
					break;
			}
			if (alnum == 16)
				return true;
		}
	}

	/* Amazon / carrier TBA-style */
	if (strstr(text, "TBA") || strstr(text, "tba")) {
		int digits = 0;
		for (const char *p = text; *p; p++)
			if (is_digit_char(*p))
				digits++;
		if (digits >= 10)
			return true;
	}

	/* Long numeric tracking (USPS 20–22, FedEx 12–15) */
	{
		char digits[32];
		size_t count = 0;

		for (const char *p = text; *p; p++) {
			if (is_digit_char(*p)) {
				if (count < sizeof(digits) - 1)
					digits[count++] = *p;
			} else if (count >= 12) {
				if (count >= 12 && count <= 22)
					return true;
				count = 0;
			} else if (*p != ' ' && *p != '-') {
				count = 0;
			}
		}
		if (count >= 12 && count <= 22)
			return true;
	}

	return false;
}

static bool match_long_id_sequence(const char *text)
{
	char digits[64];
	size_t count = 0;

	if (!text)
		return false;

	for (const char *p = text; *p; p++) {
		if (is_digit_char(*p)) {
			if (count < sizeof(digits) - 1)
				digits[count++] = *p;
		} else if (count >= 10) {
			return true;
		} else if (*p != ' ' && *p != '-' && *p != '.') {
			count = 0;
		}
	}

	return count >= 10;
}

irlsafety_sensitive_pattern_kind irlsafety_sensitive_pattern_classify(const char *text)
{
	if (!text || text[0] == '\0')
		return IRLSAFETY_PATTERN_NONE;

	if (match_credit_card_in_text(text))
		return IRLSAFETY_PATTERN_CREDIT_CARD;
	if (match_ssn_in_text(text))
		return IRLSAFETY_PATTERN_SSN;
	if (match_tracking_in_text(text))
		return IRLSAFETY_PATTERN_TRACKING_NUMBER;
	if (match_phone_in_text(text))
		return IRLSAFETY_PATTERN_PHONE;
	if (match_long_id_sequence(text))
		return IRLSAFETY_PATTERN_LONG_ID_SEQUENCE;

	return IRLSAFETY_PATTERN_NONE;
}

bool irlsafety_text_has_sensitive_pattern(const char *text)
{
	return irlsafety_sensitive_pattern_classify(text) != IRLSAFETY_PATTERN_NONE;
}

static void expand_rect(irlsafety_rect *rect, float overlap_ratio)
{
	float pad_x;
	float pad_y;

	if (!rect || overlap_ratio <= 0.0f)
		return;

	pad_x = rect->width * overlap_ratio;
	pad_y = rect->height * overlap_ratio;
	rect->x -= pad_x * 0.5f;
	rect->y -= pad_y * 0.5f;
	rect->width += pad_x;
	rect->height += pad_y;

	if (rect->width < IRLSAFETY_MIN_REGION_PX)
		rect->width = IRLSAFETY_MIN_REGION_PX;
	if (rect->height < IRLSAFETY_MIN_REGION_PX)
		rect->height = IRLSAFETY_MIN_REGION_PX;
}

static void add_hit_region(const irlsafety_ocr_hit *hit, float scale_x, float scale_y, float overlap_ratio,
			   irlsafety_region_list *out_regions)
{
	irlsafety_rect rect;

	if (!hit || !out_regions || out_regions->count >= IRLSAFETY_MAX_REGIONS)
		return;

	rect.x = hit->x * scale_x;
	rect.y = hit->y * scale_y;
	rect.width = hit->width * scale_x;
	rect.height = hit->height * scale_y;
	rect.confidence = 1.0f;

	expand_rect(&rect, overlap_ratio);
	out_regions->regions[out_regions->count++] = rect;
}

int irlsafety_match_sensitive_pattern_hits(const irlsafety_ocr_hit_list *hits, float scale_x, float scale_y,
					   float overlay_overlap, irlsafety_region_list *out_regions)
{
	size_t i;

	if (!hits || !out_regions)
		return -1;

	for (i = 0; i < hits->count; i++) {
		const irlsafety_ocr_hit *hit = &hits->hits[i];

		if (hit->text[0] == '\0')
			continue;

		if (!irlsafety_text_has_sensitive_pattern(hit->text))
			continue;

		add_hit_region(hit, scale_x, scale_y, overlay_overlap, out_regions);
	}

	return 0;
}