package com.irlsafety.plus

data class DetectionSettings(
    val enableAll: Boolean = true,
    val licensePlates: Boolean = true,
    val streetSigns: Boolean = true,
    val shippingLabels: Boolean = true,
    val idDocuments: Boolean = true,
    val confidenceThreshold: Float = 0.35f,
    val frameSkip: Int = 6,
    val preferGpu: Boolean = true
)