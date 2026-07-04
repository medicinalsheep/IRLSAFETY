package com.irlsafety.plus.overlay

import android.opengl.GLES20
import android.opengl.GLSurfaceView
import com.irlsafety.plus.OverlayRect
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.FloatBuffer
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

internal class CensorOverlayRenderer : GLSurfaceView.Renderer {
    private var program = 0
    private var colorHandle = 0
    private var positionHandle = 0
    private var viewWidth = 1
    private var viewHeight = 1
    private var rects: List<OverlayRect> = emptyList()
    private var colorArgb = 0xFF000000.toInt()

    @Synchronized
    fun setViewRects(viewRects: List<OverlayRect>, argbColor: Int) {
        rects = viewRects
        colorArgb = argbColor
    }

    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        GLES20.glClearColor(0f, 0f, 0f, 0f)
        GLES20.glEnable(GLES20.GL_BLEND)
        GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA)
        program = buildProgram(VERTEX_SHADER, FRAGMENT_SHADER)
        positionHandle = GLES20.glGetAttribLocation(program, "aPosition")
        colorHandle = GLES20.glGetUniformLocation(program, "uColor")
    }

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        viewWidth = width.coerceAtLeast(1)
        viewHeight = height.coerceAtLeast(1)
        GLES20.glViewport(0, 0, viewWidth, viewHeight)
    }

    override fun onDrawFrame(gl: GL10?) {
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT)
        if (rects.isEmpty() || program == 0) {
            return
        }

        GLES20.glUseProgram(program)
        val a = ((colorArgb ushr 24) and 0xFF) / 255f
        val r = ((colorArgb ushr 16) and 0xFF) / 255f
        val g = ((colorArgb ushr 8) and 0xFF) / 255f
        val b = (colorArgb and 0xFF) / 255f
        GLES20.glUniform4f(colorHandle, r, g, b, a)

        for (rect in rects) {
            drawRect(rect)
        }
    }

    private fun drawRect(rect: OverlayRect) {
        val left = rect.x
        val top = rect.y
        val right = rect.x + rect.width
        val bottom = rect.y + rect.height

        val ndc = floatArrayOf(
            toNdcX(left), toNdcY(top),
            toNdcX(right), toNdcY(top),
            toNdcX(left), toNdcY(bottom),
            toNdcX(right), toNdcY(top),
            toNdcX(right), toNdcY(bottom),
            toNdcX(left), toNdcY(bottom)
        )

        val buffer = ndc.toFloatBuffer()
        GLES20.glEnableVertexAttribArray(positionHandle)
        GLES20.glVertexAttribPointer(positionHandle, 2, GLES20.GL_FLOAT, false, 0, buffer)
        GLES20.glDrawArrays(GLES20.GL_TRIANGLES, 0, 6)
        GLES20.glDisableVertexAttribArray(positionHandle)
    }

    private fun toNdcX(px: Float): Float = (px / viewWidth.toFloat()) * 2f - 1f

    private fun toNdcY(px: Float): Float = 1f - (px / viewHeight.toFloat()) * 2f

    private fun FloatArray.toFloatBuffer(): FloatBuffer =
        ByteBuffer.allocateDirect(size * 4).order(ByteOrder.nativeOrder()).asFloatBuffer().apply {
            put(this@toFloatBuffer)
            position(0)
        }

    private fun buildProgram(vertexSource: String, fragmentSource: String): Int {
        val vertexShader = compileShader(GLES20.GL_VERTEX_SHADER, vertexSource)
        val fragmentShader = compileShader(GLES20.GL_FRAGMENT_SHADER, fragmentSource)
        val linkedProgram = GLES20.glCreateProgram()
        GLES20.glAttachShader(linkedProgram, vertexShader)
        GLES20.glAttachShader(linkedProgram, fragmentShader)
        GLES20.glLinkProgram(linkedProgram)
        GLES20.glDeleteShader(vertexShader)
        GLES20.glDeleteShader(fragmentShader)
        return linkedProgram
    }

    private fun compileShader(type: Int, source: String): Int {
        val shader = GLES20.glCreateShader(type)
        GLES20.glShaderSource(shader, source)
        GLES20.glCompileShader(shader)
        return shader
    }

    companion object {
        private const val VERTEX_SHADER = """
            attribute vec2 aPosition;
            void main() {
                gl_Position = vec4(aPosition, 0.0, 1.0);
            }
        """

        private const val FRAGMENT_SHADER = """
            precision mediump float;
            uniform vec4 uColor;
            void main() {
                gl_FragColor = uColor;
            }
        """
    }
}