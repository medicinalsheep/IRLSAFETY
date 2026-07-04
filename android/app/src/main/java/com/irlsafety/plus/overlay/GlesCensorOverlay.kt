package com.irlsafety.plus.overlay

import android.content.Context
import android.graphics.PixelFormat
import android.opengl.GLSurfaceView
import com.irlsafety.plus.OverlayRect

class GlesCensorOverlay(context: Context) : GLSurfaceView(context) {
    private val renderer = CensorOverlayRenderer()

    init {
        setEGLContextClientVersion(2)
        setEGLConfigChooser(8, 8, 8, 8, 16, 0)
        holder.setFormat(PixelFormat.TRANSLUCENT)
        setZOrderOnTop(true)
        setRenderer(renderer)
        renderMode = RENDERMODE_WHEN_DIRTY
    }

    fun updateRegions(viewRects: List<OverlayRect>, argbColor: Int = 0xFF000000.toInt()) {
        renderer.setViewRects(viewRects, argbColor)
        requestRender()
    }
}