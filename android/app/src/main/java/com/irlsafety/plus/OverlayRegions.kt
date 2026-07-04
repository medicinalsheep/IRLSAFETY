package com.irlsafety.plus

import androidx.camera.view.PreviewView

data class OverlayRect(
    val x: Float,
    val y: Float,
    val width: Float,
    val height: Float
)

data class OverlayFrame(
    val frameWidth: Int,
    val frameHeight: Int,
    val rects: List<OverlayRect>
) {
    companion object {
        private const val HEADER_FLOATS = 3

        fun fromPacked(data: FloatArray): OverlayFrame? {
            if (data.size < HEADER_FLOATS) {
                return null
            }

            val frameWidth = data[0].toInt()
            val frameHeight = data[1].toInt()
            val count = data[2].toInt().coerceAtLeast(0)
            val expected = HEADER_FLOATS + count * 4
            if (frameWidth <= 0 || frameHeight <= 0 || data.size < expected) {
                return null
            }

            val rects = ArrayList<OverlayRect>(count)
            var index = HEADER_FLOATS
            repeat(count) {
                rects.add(
                    OverlayRect(
                        x = data[index++],
                        y = data[index++],
                        width = data[index++],
                        height = data[index++]
                    )
                )
            }
            return OverlayFrame(frameWidth, frameHeight, rects)
        }
    }
}

object OverlayCoordinateMapper {
    fun toViewRects(
        frame: OverlayFrame,
        viewWidth: Int,
        viewHeight: Int,
        scaleType: PreviewView.ScaleType
    ): List<OverlayRect> {
        if (viewWidth <= 0 || viewHeight <= 0 || frame.rects.isEmpty()) {
            return emptyList()
        }

        val frameWidth = frame.frameWidth.toFloat()
        val frameHeight = frame.frameHeight.toFloat()
        val viewW = viewWidth.toFloat()
        val viewH = viewHeight.toFloat()

        val (scale, offsetX, offsetY) = when (scaleType) {
            PreviewView.ScaleType.FIT_CENTER, PreviewView.ScaleType.FIT_START, PreviewView.ScaleType.FIT_END -> {
                val fitScale = minOf(viewW / frameWidth, viewH / frameHeight)
                val displayedW = frameWidth * fitScale
                val displayedH = frameHeight * fitScale
                Triple(fitScale, (viewW - displayedW) * 0.5f, (viewH - displayedH) * 0.5f)
            }
            else -> {
                val fillScale = maxOf(viewW / frameWidth, viewH / frameHeight)
                val displayedW = frameWidth * fillScale
                val displayedH = frameHeight * fillScale
                Triple(fillScale, (viewW - displayedW) * 0.5f, (viewH - displayedH) * 0.5f)
            }
        }

        return frame.rects.map { rect ->
            OverlayRect(
                x = offsetX + rect.x * scale,
                y = offsetY + rect.y * scale,
                width = rect.width * scale,
                height = rect.height * scale
            )
        }
    }
}