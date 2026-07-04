package com.irlsafety.plus

object IRLSafetyNative {
    init {
        System.loadLibrary("irlsafety_jni")
    }

    external fun nativeGetApiVersion(): String
    external fun nativeCreatePipeline(modelPath: String): Long
    external fun nativeDestroyPipeline(handle: Long)
    external fun nativePipelineStatus(handle: Long): String

    /**
     * Convert a CameraX YUV_420_888 frame and run detection.
     * Returns overlay region count for the processed frame.
     */
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