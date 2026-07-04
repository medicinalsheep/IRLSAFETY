package com.irlsafety.plus

import android.content.Context

object SettingsStore {
    private const val PREFS = "irlsafety_detection_settings"

    fun load(context: Context): DetectionSettings {
        val prefs = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
        return DetectionSettings(
            enableAll = prefs.getBoolean("enable_all", true),
            licensePlates = prefs.getBoolean("cat_license_plates", true),
            streetSigns = prefs.getBoolean("cat_street_signs", true),
            shippingLabels = prefs.getBoolean("cat_shipping_labels", true),
            idDocuments = prefs.getBoolean("cat_id_documents", true),
            confidenceThreshold = prefs.getFloat("confidence_threshold", 0.35f),
            frameSkip = prefs.getInt("frame_skip", 6).coerceIn(1, 20),
            preferGpu = prefs.getBoolean("prefer_gpu", true)
        )
    }

    fun save(context: Context, settings: DetectionSettings) {
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
            .edit()
            .putBoolean("enable_all", settings.enableAll)
            .putBoolean("cat_license_plates", settings.licensePlates)
            .putBoolean("cat_street_signs", settings.streetSigns)
            .putBoolean("cat_shipping_labels", settings.shippingLabels)
            .putBoolean("cat_id_documents", settings.idDocuments)
            .putFloat("confidence_threshold", settings.confidenceThreshold)
            .putInt("frame_skip", settings.frameSkip.coerceIn(1, 20))
            .putBoolean("prefer_gpu", settings.preferGpu)
            .apply()
    }
}