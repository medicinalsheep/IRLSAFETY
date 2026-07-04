package com.irlsafety.plus

object IRLSafetyNative {
    init {
        System.loadLibrary("irlsafety_jni")
    }

    external fun nativeCreatePipeline(modelPath: String): Long
    external fun nativeDestroyPipeline(handle: Long)
    external fun nativePipelineStatus(handle: Long): String

    external fun nativeApplySettings(
        handle: Long,
        enableAll: Boolean,
        licensePlates: Boolean,
        streetSigns: Boolean,
        shippingLabels: Boolean,
        idDocuments: Boolean,
        confidenceThreshold: Float,
        frameSkip: Int,
        preferGpu: Boolean
    )

    /**
     * Convert a CameraX YUV_420_888 frame and run detection.
     * Returns overlay region count for the processed frame.
     */
    /** Packed overlay rects: [frameW, frameH, count, x, y, w, h, ...] */
    external fun nativeGetOverlayRects(handle: Long): FloatArray?

    external fun nativeProcessCameraFrame(
        handle: Long,
        width: Int,
        height: Int,
        yPlane: ByteArray,
        yRowStride: Int,
        yPixelStride: Int,
        uPlane: ByteArray,
        uRowStride: Int,
        uPixelStride: Int,
        vPlane: ByteArray,
        vRowStride: Int,
        vPixelStride: Int
    ): Int
}