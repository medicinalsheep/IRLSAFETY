package com.irlsafety.plus

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.camera.view.PreviewView
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableLongStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.content.ContextCompat
import kotlinx.coroutines.delay
import java.util.concurrent.Executors

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContent {
            MaterialTheme(colorScheme = darkColorScheme()) {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    AlphaShell(lifecycleOwner = this)
                }
            }
        }
    }
}

@Composable
private fun AlphaShell(lifecycleOwner: androidx.lifecycle.LifecycleOwner) {
    val context = LocalContext.current

    var pipelineHandle by remember { mutableLongStateOf(0L) }
    var statusLine by remember { mutableStateOf("Starting…") }
    var modelLine by remember { mutableStateOf("Loading model…") }
    var overlayCount by remember { mutableIntStateOf(0) }
    val modelPath = remember { ModelInstaller.ensureDetectionModel(context) }
    var hasCameraPermission by remember {
        mutableStateOf(
            ContextCompat.checkSelfPermission(context, Manifest.permission.CAMERA) ==
                PackageManager.PERMISSION_GRANTED
        )
    }

    val permissionLauncher = rememberLauncherForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { granted ->
        hasCameraPermission = granted
    }

    val previewView = remember {
        PreviewView(context).apply {
            implementationMode = PreviewView.ImplementationMode.COMPATIBLE
            scaleType = PreviewView.ScaleType.FILL_CENTER
        }
    }

    val analysisExecutor = remember { Executors.newSingleThreadExecutor() }

    DisposableEffect(modelPath) {
        if (modelPath.isNullOrBlank()) {
            modelLine = "Model missing — rebuild APK with irlsafety-detect.onnx asset."
            pipelineHandle = 0L
        } else {
            modelLine = "Model: ${modelPath.substringAfterLast('/')}"
            pipelineHandle = IRLSafetyNative.nativeCreatePipeline(modelPath)
            statusLine = IRLSafetyNative.nativePipelineStatus(pipelineHandle)
        }
        onDispose {
            if (pipelineHandle != 0L) {
                IRLSafetyNative.nativeDestroyPipeline(pipelineHandle)
                pipelineHandle = 0L
            }
        }
    }

    LaunchedEffect(hasCameraPermission) {
        if (!hasCameraPermission) {
            permissionLauncher.launch(Manifest.permission.CAMERA)
        }
    }

    DisposableEffect(pipelineHandle, hasCameraPermission) {
        val session = if (hasCameraPermission && pipelineHandle != 0L) {
            CameraSession(
                context = context,
                lifecycleOwner = lifecycleOwner,
                previewView = previewView,
                pipelineHandle = pipelineHandle,
                analysisExecutor = analysisExecutor,
                onFrameProcessed = { count ->
                    overlayCount = count
                }
            ).also { it.start() }
        } else {
            null
        }

        onDispose {
            session?.stop()
            analysisExecutor.shutdown()
        }
    }

    LaunchedEffect(pipelineHandle) {
        while (pipelineHandle != 0L) {
            statusLine = IRLSafetyNative.nativePipelineStatus(pipelineHandle)
            delay(500)
        }
    }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp)
    ) {
        Text(
            text = "IRLSAFETY+",
            fontSize = 24.sp,
            fontWeight = FontWeight.Bold,
            color = Color(0xFF3ECF8E)
        )
        Text(
            text = IRLSafetyNative.nativeGetApiVersion(),
            color = Color(0xFF8AB4F8),
            fontSize = 13.sp
        )

        if (hasCameraPermission) {
            AndroidView(
                factory = { previewView },
                modifier = Modifier
                    .fillMaxWidth()
                    .aspectRatio(9f / 16f)
            )
        } else {
            Text(
                text = "Camera permission required for live preview.",
                color = Color(0xFFFF8A80),
                fontSize = 14.sp
            )
        }

        Text(
            text = modelLine,
            color = Color(0xFF8AB4F8),
            fontSize = 13.sp
        )
        Text(
            text = statusLine,
            color = Color(0xFFE8EAED),
            fontSize = 14.sp
        )
        Text(
            text = "Overlays (last frame): $overlayCount",
            color = Color(0xFFE8EAED),
            fontSize = 14.sp
        )
        Text(
            text = "P12 — ONNX Runtime + irlsafety-detect.onnx on-device.\n" +
                "Next: GLES censorship boxes (P13), settings UI (P14).",
            color = Color(0xFF9AA0A6),
            fontSize = 12.sp,
            lineHeight = 17.sp
        )
    }
}

private fun darkColorScheme() = androidx.compose.material3.darkColorScheme(
    background = Color(0xFF12141A),
    surface = Color(0xFF12141A),
    onBackground = Color(0xFFE8EAED),
    onSurface = Color(0xFFE8EAED),
    primary = Color(0xFF3ECF8E)
)