/*
 * IRLSAFETY+ — low-poly region geometry (angled quads from OBB detections).
 * Copyright (c) 2026 medicinalsheep. MIT License.
 */

#include "irlsafety_geometry.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

bool irlsafety_rect_has_rotation(const irlsafety_rect *rect)
{
	if (!rect)
		return false;

	return fabsf(rect->rotation_deg) > 0.5f;
}

void irlsafety_rect_corners(const irlsafety_rect *rect, float corners[8])
{
	float cx;
	float cy;
	float hw;
	float hh;
	float rad;
	float cos_a;
	float sin_a;
	float local[8];
	size_t i;

	if (!rect || !corners) {
		return;
	}

	if (!irlsafety_rect_has_rotation(rect)) {
		corners[0] = rect->x;
		corners[1] = rect->y;
		corners[2] = rect->x + rect->width;
		corners[3] = rect->y;
		corners[4] = rect->x + rect->width;
		corners[5] = rect->y + rect->height;
		corners[6] = rect->x;
		corners[7] = rect->y + rect->height;
		return;
	}

	cx = rect->x + rect->width * 0.5f;
	cy = rect->y + rect->height * 0.5f;
	hw = rect->width * 0.5f;
	hh = rect->height * 0.5f;
	rad = rect->rotation_deg * (float)(M_PI / 180.0);
	cos_a = cosf(rad);
	sin_a = sinf(rad);

	local[0] = -hw;
	local[1] = -hh;
	local[2] = hw;
	local[3] = -hh;
	local[4] = hw;
	local[5] = hh;
	local[6] = -hw;
	local[7] = hh;

	for (i = 0; i < 4; i++) {
		float lx = local[i * 2];
		float ly = local[i * 2 + 1];
		corners[i * 2] = cx + lx * cos_a - ly * sin_a;
		corners[i * 2 + 1] = cy + lx * sin_a + ly * cos_a;
	}
}

static float cross2d(float ax, float ay, float bx, float by)
{
	return ax * by - ay * bx;
}

bool irlsafety_point_in_quad(float px, float py, const float corners[8])
{
	int sign = 0;
	size_t i;

	if (!corners)
		return false;

	for (i = 0; i < 4; i++) {
		size_t j = (i + 1) % 4;
		float ax = corners[i * 2];
		float ay = corners[i * 2 + 1];
		float bx = corners[j * 2];
		float by = corners[j * 2 + 1];
		float c = cross2d(bx - ax, by - ay, px - ax, py - ay);

		if (fabsf(c) < 0.001f)
			continue;

		if (sign == 0)
			sign = c > 0.0f ? 1 : -1;
		else if ((c > 0.0f ? 1 : -1) != sign)
			return false;
	}

	return sign != 0;
}

void irlsafety_quad_bounds(const float corners[8], float *out_x, float *out_y, float *out_w, float *out_h)
{
	float min_x;
	float min_y;
	float max_x;
	float max_y;
	size_t i;

	if (!corners || !out_x || !out_y || !out_w || !out_h)
		return;

	min_x = max_x = corners[0];
	min_y = max_y = corners[1];

	for (i = 1; i < 4; i++) {
		float x = corners[i * 2];
		float y = corners[i * 2 + 1];

		if (x < min_x)
			min_x = x;
		if (x > max_x)
			max_x = x;
		if (y < min_y)
			min_y = y;
		if (y > max_y)
			max_y = y;
	}

	*out_x = min_x;
	*out_y = min_y;
	*out_w = max_x - min_x;
	*out_h = max_y - min_y;
}