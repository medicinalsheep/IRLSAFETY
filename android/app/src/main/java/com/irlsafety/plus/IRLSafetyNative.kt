package com.irlsafety.plus

object IRLSafetyNative {
    init {
        System.loadLibrary("irlsafety_jni")
    }

    external fun nativeGetApiVersion(): String
    external fun nativeCreatePipeline(): Long
    external fun nativeDestroyPipeline(handle: Long)
    external fun nativePipelineStatus(handle: Long): String
}