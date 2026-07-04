#pragma once

#include <cstdint>
#include <vector>

void irlsafety_android_yuv420888_to_bgra(const uint8_t *y, int y_row_stride, int y_pixel_stride, const uint8_t *u,
					 int u_row_stride, int u_pixel_stride, const uint8_t *v, int v_row_stride,
					 int v_pixel_stride, int width, int height, std::vector<uint8_t> &out_bgra);