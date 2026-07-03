/*
 * IRLSAFETY+ — low-poly region geometry (angled quads from OBB detections).
 * Copyright (c) 2026 medicinalsheep. MIT License.
 */

#pragma once

#include "irlsafety_types.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Four corners (x0,y0, x1,y1, x2,y2, x3,y3) — low-poly quad for angled cover. */
void irlsafety_rect_corners(const irlsafety_rect *rect, float corners[8]);

bool irlsafety_rect_has_rotation(const irlsafety_rect *rect);

bool irlsafety_point_in_quad(float px, float py, const float corners[8]);

void irlsafety_quad_bounds(const float corners[8], float *out_x, float *out_y, float *out_w, float *out_h);

#ifdef __cplusplus
}
#endif