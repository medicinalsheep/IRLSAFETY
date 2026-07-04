/*
 * IRLSAFETY+ — CameraX YUV_420_888 → BGRA for libirlsafety (P11).
 * Copyright (c) 2026 IRLSAFETY+ Contributors. MIT License.
 */

#include <cstdint>
#include <vector>

static inline uint8_t clamp_u8(int v)
{
	if (v < 0)
		return 0;
	if (v > 255)
		return 255;
	return static_cast<uint8_t>(v);
}

void irlsafety_android_yuv420888_to_bgra(const uint8_t *y, int y_row_stride, int y_pixel_stride, const uint8_t *u,
					 int u_row_stride, int u_pixel_stride, const uint8_t *v, int v_row_stride,
					 int v_pixel_stride, int width, int height, std::vector<uint8_t> &out_bgra)
{
	const size_t needed = static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;
	out_bgra.resize(needed);

	for (int row = 0; row < height; row++) {
		const int uv_row = row / 2;

		for (int col = 0; col < width; col++) {
			const int uv_col = col / 2;
			const uint8_t Y = y[row * y_row_stride + col * y_pixel_stride];
			const uint8_t U = u[uv_row * u_row_stride + uv_col * u_pixel_stride];
			const uint8_t V = v[uv_row * v_row_stride + uv_col * v_pixel_stride];

			const int c = static_cast<int>(Y) - 16;
			const int d = static_cast<int>(U) - 128;
			const int e = static_cast<int>(V) - 128;
			const int r = (298 * c + 409 * e + 128) >> 8;
			const int g = (298 * c - 100 * d - 208 * e + 128) >> 8;
			const int b = (298 * c + 516 * d + 128) >> 8;

			const size_t offset = (static_cast<size_t>(row) * static_cast<size_t>(width) + static_cast<size_t>(col)) * 4u;
			out_bgra[offset + 0] = clamp_u8(b);
			out_bgra[offset + 1] = clamp_u8(g);
			out_bgra[offset + 2] = clamp_u8(r);
			out_bgra[offset + 3] = 255;
		}
	}
}