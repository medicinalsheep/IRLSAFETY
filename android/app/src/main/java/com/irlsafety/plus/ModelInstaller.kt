package com.irlsafety.plus

import android.content.Context
import java.io.File

object ModelInstaller {
    private const val ASSET_PATH = "models/irlsafety-detect.onnx"
    private const val FILE_NAME = "irlsafety-detect.onnx"

    /**
     * Copy bundled ONNX from assets to app-private storage (first launch only).
     * Returns absolute path suitable for native model load, or null on failure.
     */
    fun ensureDetectionModel(context: Context): String? {
        val modelsDir = File(context.filesDir, "models")
        if (!modelsDir.exists() && !modelsDir.mkdirs()) {
            return null
        }

        val outFile = File(modelsDir, FILE_NAME)
        if (outFile.exists() && outFile.length() > 1_000_000L) {
            return outFile.absolutePath
        }

        return try {
            context.assets.open(ASSET_PATH).use { input ->
                outFile.outputStream().use { output ->
                    input.copyTo(output)
                }
            }
            outFile.absolutePath
        } catch (_: Exception) {
            outFile.delete()
            null
        }
    }
}