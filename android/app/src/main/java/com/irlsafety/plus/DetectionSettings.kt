package com.irlsafety.plus

/** Analysis target height for CameraX ImageAnalysis (width follows 16:9). */
enum class AnalysisResolution(val height: Int, val label: String) {
    P720(720, "720p"),
    P1080(1080, "1080p");

    companion object {
        fun fromHeight(height: Int): AnalysisResolution =
            if (height >= 1080) P1080 else P720
    }
}

data class DetectionSettings(
    val enableAll: Boolean = true,
    val licensePlates: Boolean = true,
    val streetSigns: Boolean = true,
    val shippingLabels: Boolean = true,
    val idDocuments: Boolean = true,
    val confidenceThreshold: Float = 0.35f,
    /** Aligned with libirlsafety / Windows low-end default (v0.9.4+). */
    val frameSkip: Int = 8,
    val preferGpu: Boolean = true,
    val useFrontCamera: Boolean = false,
    /** P16: 720p (default, cooler) or 1080p analysis resolution. */
    val analysisResolution: AnalysisResolution = AnalysisResolution.P720
)