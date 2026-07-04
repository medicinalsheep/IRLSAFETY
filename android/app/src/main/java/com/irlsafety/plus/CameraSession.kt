package com.irlsafety.plus

import android.content.Context
import androidx.camera.core.CameraSelector
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.ImageProxy
import androidx.camera.core.Preview
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.core.content.ContextCompat
import androidx.lifecycle.LifecycleOwner
import java.nio.ByteBuffer
import java.util.concurrent.Executor

class CameraSession(
    private val context: Context,
    private val lifecycleOwner: LifecycleOwner,
    private val previewView: PreviewView,
    private val pipelineHandle: Long,
    private val analysisExecutor: Executor,
    private val onFrameProcessed: (overlayCount: Int, overlayData: FloatArray?) -> Unit
) {
    private var cameraProvider: ProcessCameraProvider? = null

    fun start() {
        val future = ProcessCameraProvider.getInstance(context)
        future.addListener({
            cameraProvider = future.get()
            bindUseCases()
        }, ContextCompat.getMainExecutor(context))
    }

    fun stop() {
        cameraProvider?.unbindAll()
        cameraProvider = null
    }

    private fun bindUseCases() {
        val provider = cameraProvider ?: return

        val preview = Preview.Builder().build().also {
            it.surfaceProvider = previewView.surfaceProvider
        }

        val analysis = ImageAnalysis.Builder()
            .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
            .build()

        analysis.setAnalyzer(analysisExecutor) { image ->
            try {
                processFrame(image)
            } finally {
                image.close()
            }
        }

        provider.unbindAll()
        provider.bindToLifecycle(
            lifecycleOwner,
            CameraSelector.DEFAULT_BACK_CAMERA,
            preview,
            analysis
        )
    }

    private fun processFrame(image: ImageProxy) {
        if (pipelineHandle == 0L) {
            return
        }

        val planes = image.planes
        if (planes.size < 3) {
            return
        }

        val yPlane = planes[0]
        val uPlane = planes[1]
        val vPlane = planes[2]

        val overlayCount = IRLSafetyNative.nativeProcessCameraFrame(
            pipelineHandle,
            image.width,
            image.height,
            planeToByteArray(yPlane.buffer),
            yPlane.rowStride,
            yPlane.pixelStride,
            planeToByteArray(uPlane.buffer),
            uPlane.rowStride,
            uPlane.pixelStride,
            planeToByteArray(vPlane.buffer),
            vPlane.rowStride,
            vPlane.pixelStride
        )

        onFrameProcessed(overlayCount, IRLSafetyNative.nativeGetOverlayRects(pipelineHandle))
    }

    private fun planeToByteArray(buffer: ByteBuffer): ByteArray {
        val data = ByteArray(buffer.remaining())
        buffer.get(data)
        buffer.rewind()
        return data
    }
}