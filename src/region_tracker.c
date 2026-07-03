/*
 * IRLSAFETY+ — persistent overlay region tracking across detection cycles.
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include "region_tracker.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>

static float rect_area(const irlsafety_rect *rect)
{
	if (!rect || rect->width <= 0.0f || rect->height <= 0.0f)
		return 0.0f;
	return rect->width * rect->height;
}

static float rect_intersection_area(const irlsafety_rect *a, const irlsafety_rect *b)
{
	float x0 = a->x > b->x ? a->x : b->x;
	float y0 = a->y > b->y ? a->y : b->y;
	float x1 = (a->x + a->width) < (b->x + b->width) ? (a->x + a->width) : (b->x + b->width);
	float y1 = (a->y + a->height) < (b->y + b->height) ? (a->y + a->height) : (b->y + b->height);
	float w = x1 - x0;
	float h = y1 - y0;

	if (w <= 0.0f || h <= 0.0f)
		return 0.0f;
	return w * h;
}

static float rect_iou(const irlsafety_rect *a, const irlsafety_rect *b)
{
	float intersection = rect_intersection_area(a, b);
	float union_area = rect_area(a) + rect_area(b) - intersection;

	if (union_area <= 0.0f)
		return 0.0f;
	return intersection / union_area;
}

static bool rect_on_screen(const irlsafety_rect *rect, uint32_t frame_width, uint32_t frame_height)
{
	float x1 = rect->x + rect->width;
	float y1 = rect->y + rect->height;

	if (x1 <= 0.0f || y1 <= 0.0f)
		return false;
	if (rect->x >= (float)frame_width || rect->y >= (float)frame_height)
		return false;
	return true;
}

static float tracker_speed(const irlsafety_region_tracker *tracker, size_t index)
{
	if (!tracker || index >= tracker->count)
		return 0.0f;
	return fabsf(tracker->velocity_x[index]) + fabsf(tracker->velocity_y[index]);
}

static void tracker_blend_velocity(irlsafety_region_tracker *tracker, size_t index, float dx, float dy,
				   int frames_since_update)
{
	float inv_frames;
	float instant_vx;
	float instant_vy;
	float old_vx;
	float old_vy;

	if (!tracker || index >= tracker->count || frames_since_update < 1)
		return;

	inv_frames = 1.0f / (float)frames_since_update;
	instant_vx = dx * inv_frames;
	instant_vy = dy * inv_frames;
	old_vx = tracker->velocity_x[index];
	old_vy = tracker->velocity_y[index];

	tracker->velocity_x[index] =
		old_vx * IRLSAFETY_TRACKER_VELOCITY_BLEND + instant_vx * (1.0f - IRLSAFETY_TRACKER_VELOCITY_BLEND);
	tracker->velocity_y[index] =
		old_vy * IRLSAFETY_TRACKER_VELOCITY_BLEND + instant_vy * (1.0f - IRLSAFETY_TRACKER_VELOCITY_BLEND);
}

static irlsafety_rect tracker_predicted_rect(const irlsafety_region_tracker *tracker, size_t index)
{
	irlsafety_rect predicted;

	if (!tracker || index >= tracker->count)
		return (irlsafety_rect){0};

	predicted = tracker->regions[index];
	if (tracker->miss_cycles[index] > 0) {
		predicted.x += tracker->velocity_x[index] * (float)tracker->miss_cycles[index];
		predicted.y += tracker->velocity_y[index] * (float)tracker->miss_cycles[index];
	}
	return predicted;
}

void irlsafety_region_tracker_clear(irlsafety_region_tracker *tracker)
{
	if (!tracker)
		return;

	memset(tracker, 0, sizeof(*tracker));
}

void irlsafety_region_tracker_scale(irlsafety_region_tracker *tracker, uint32_t new_width, uint32_t new_height)
{
	float scale_x;
	float scale_y;
	size_t i;

	if (!tracker || tracker->count == 0)
		return;

	if (tracker->frame_width == 0 || tracker->frame_height == 0) {
		irlsafety_region_tracker_clear(tracker);
		return;
	}

	scale_x = (float)new_width / (float)tracker->frame_width;
	scale_y = (float)new_height / (float)tracker->frame_height;

	for (i = 0; i < tracker->count; i++) {
		tracker->regions[i].x *= scale_x;
		tracker->regions[i].y *= scale_y;
		tracker->regions[i].width *= scale_x;
		tracker->regions[i].height *= scale_y;
		tracker->velocity_x[i] *= scale_x;
		tracker->velocity_y[i] *= scale_y;
	}

	tracker->frame_width = new_width;
	tracker->frame_height = new_height;
}

void irlsafety_region_tracker_update(irlsafety_region_tracker *tracker, const irlsafety_region_list *detected,
				     uint32_t frame_width, uint32_t frame_height, int frames_since_update)
{
	bool matched[IRLSAFETY_MAX_REGIONS];
	size_t write = 0;
	size_t i;
	size_t j;

	if (!tracker)
		return;

	if (frames_since_update < 1)
		frames_since_update = 1;

	if (tracker->frame_width != 0 && tracker->frame_height != 0 &&
	    (tracker->frame_width != frame_width || tracker->frame_height != frame_height))
		irlsafety_region_tracker_scale(tracker, frame_width, frame_height);

	memset(matched, 0, sizeof(matched));

	if (detected) {
		for (i = 0; i < detected->count; i++) {
			const irlsafety_rect *fresh = &detected->regions[i];
			float best_iou = 0.0f;
			int best_index = -1;
			float iou_threshold = IRLSAFETY_TRACKER_IOU_MATCH;

			if (!rect_on_screen(fresh, frame_width, frame_height))
				continue;

			for (j = 0; j < tracker->count; j++) {
				irlsafety_rect candidate = tracker_predicted_rect(tracker, j);
				float iou = rect_iou(fresh, &candidate);
				float speed = tracker_speed(tracker, j);
				float threshold = speed > IRLSAFETY_TRACKER_MIN_SPEED ? IRLSAFETY_TRACKER_IOU_MATCH_FAST
											: iou_threshold;

				if (iou > best_iou) {
					best_iou = iou;
					best_index = (int)j;
					iou_threshold = threshold;
				}
			}

			if (best_index >= 0 && best_iou >= iou_threshold) {
				float dx = fresh->x - tracker->regions[best_index].x;
				float dy = fresh->y - tracker->regions[best_index].y;

				tracker_blend_velocity(tracker, (size_t)best_index, dx, dy, frames_since_update);
				tracker->regions[best_index] = *fresh;
				tracker->miss_cycles[best_index] = 0;
				matched[best_index] = true;
			} else if (tracker->count < IRLSAFETY_MAX_REGIONS) {
				size_t slot = tracker->count;

				tracker->regions[slot] = *fresh;
				tracker->velocity_x[slot] = 0.0f;
				tracker->velocity_y[slot] = 0.0f;
				tracker->miss_cycles[slot] = 0;
				matched[slot] = true;
				tracker->count++;
			}
		}
	}

	for (i = 0; i < tracker->count; i++) {
		int evict_after = IRLSAFETY_TRACKER_MISS_EVICT;

		if (!matched[i])
			tracker->miss_cycles[i]++;

		if (tracker_speed(tracker, i) > IRLSAFETY_TRACKER_MIN_SPEED)
			evict_after = IRLSAFETY_TRACKER_MISS_EVICT_MOVING;

		if (tracker->miss_cycles[i] >= evict_after)
			continue;

		if (write != i) {
			tracker->regions[write] = tracker->regions[i];
			tracker->velocity_x[write] = tracker->velocity_x[i];
			tracker->velocity_y[write] = tracker->velocity_y[i];
			tracker->miss_cycles[write] = tracker->miss_cycles[i];
		}
		write++;
	}

	tracker->count = write;
	tracker->frame_width = frame_width;
	tracker->frame_height = frame_height;
}

void irlsafety_region_tracker_hold_predict(irlsafety_region_tracker *tracker, int frames_since_update)
{
	size_t i;

	if (!tracker || tracker->count == 0)
		return;

	if (frames_since_update < 1)
		frames_since_update = 1;

	for (i = 0; i < tracker->count; i++) {
		tracker->regions[i].x += tracker->velocity_x[i] * (float)frames_since_update;
		tracker->regions[i].y += tracker->velocity_y[i] * (float)frames_since_update;
	}
}

void irlsafety_region_tracker_copy_regions(const irlsafety_region_tracker *tracker, irlsafety_region_list *out)
{
	size_t i;

	if (!tracker || !out)
		return;

	out->count = 0;
	for (i = 0; i < tracker->count && out->count < IRLSAFETY_MAX_REGIONS; i++) {
		if (!rect_on_screen(&tracker->regions[i], tracker->frame_width, tracker->frame_height))
			continue;
		out->regions[out->count++] = tracker->regions[i];
	}
}