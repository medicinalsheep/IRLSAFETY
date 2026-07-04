package com.irlsafety.plus

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.ViewGroup
import android.widget.FrameLayout
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.camera.view.PreviewView
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
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
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.content.ContextCompat
import com.irlsafety.plus.overlay.GlesCensorOverlay

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContent {
            MaterialTheme(colorScheme = darkColorScheme()) {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    CameraShell(lifecycleOwner = this)
                }
            }
        }
    }
}

@Composable
private fun CameraShell(lifecycleOwner: androidx.lifecycle.LifecycleOwner) {
    val context = LocalContext.current
    val scrollState = rememberScrollState()

    var pipelineHandle by remember { mutableLongStateOf(0L) }
    var overlayFrame by remember { mutableStateOf<OverlayFrame?>(null) }
    var previewBoxWidth by remember { mutableIntStateOf(0) }
    var previewBoxHeight by remember { mutableIntStateOf(0) }
    var detectionSettings by remember { mutableStateOf(SettingsStore.load(context)) }
    val modelPath = remember { ModelInstaller.ensureDetectionModel(context) }
    val mainHandler = remember { Handler(Looper.getMainLooper()) }
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

    val censorOverlay = remember { GlesCensorOverlay(context) }
    val analysisExecutor = remember { java.util.concurrent.Executors.newSingleThreadExecutor() }

    fun applySettings(handle: Long, settings: DetectionSettings) {
        if (handle == 0L) return
        IRLSafetyNative.nativeApplySettings(
            handle = handle,
            enableAll = settings.enableAll,
            licensePlates = settings.licensePlates,
            streetSigns = settings.streetSigns,
            shippingLabels = settings.shippingLabels,
            idDocuments = settings.idDocuments,
            confidenceThreshold = settings.confidenceThreshold,
            frameSkip = settings.frameSkip,
            preferGpu = settings.preferGpu
        )
    }

    DisposableEffect(modelPath) {
        pipelineHandle = if (modelPath.isNullOrBlank()) {
            0L
        } else {
            IRLSafetyNative.nativeCreatePipeline(modelPath).also { handle ->
                applySettings(handle, detectionSettings)
            }
        }
        onDispose {
            if (pipelineHandle != 0L) {
                IRLSafetyNative.nativeDestroyPipeline(pipelineHandle)
                pipelineHandle = 0L
            }
        }
    }

    LaunchedEffect(detectionSettings, pipelineHandle) {
        if (pipelineHandle == 0L) return@LaunchedEffect
        SettingsStore.save(context, detectionSettings)
        applySettings(pipelineHandle, detectionSettings)
    }

    LaunchedEffect(hasCameraPermission) {
        if (!hasCameraPermission) {
            permissionLauncher.launch(Manifest.permission.CAMERA)
        }
    }

    DisposableEffect(pipelineHandle, hasCameraPermission, detectionSettings.useFrontCamera) {
        val session = if (hasCameraPermission && pipelineHandle != 0L) {
            CameraSession(
                context = context,
                lifecycleOwner = lifecycleOwner,
                previewView = previewView,
                pipelineHandle = pipelineHandle,
                useFrontCamera = detectionSettings.useFrontCamera,
                analysisExecutor = analysisExecutor,
                onFrameProcessed = { _, overlayData ->
                    mainHandler.post {
                        overlayFrame = overlayData?.let { OverlayFrame.fromPacked(it) }
                    }
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

    LaunchedEffect(overlayFrame, previewBoxWidth, previewBoxHeight) {
        val frame = overlayFrame
        if (frame == null || previewBoxWidth <= 0 || previewBoxHeight <= 0) {
            censorOverlay.updateRegions(emptyList())
            return@LaunchedEffect
        }

        val mapped = OverlayCoordinateMapper.toViewRects(
            frame = frame,
            viewWidth = previewBoxWidth,
            viewHeight = previewBoxHeight,
            scaleType = previewView.scaleType
        )
        censorOverlay.updateRegions(mapped)
    }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .verticalScroll(scrollState)
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        Text(
            text = "IRLSAFETY+",
            fontSize = 24.sp,
            fontWeight = FontWeight.Bold,
            color = Color(0xFF3ECF8E)
        )
        Text(
            text = BuildConfig.VERSION_NAME,
            color = Color(0xFF8AB4F8),
            fontSize = 13.sp
        )

        if (modelPath.isNullOrBlank()) {
            Text(
                text = "Detection model not found. Rebuild the app from source.",
                color = Color(0xFFFF8A80),
                fontSize = 14.sp
            )
        } else if (hasCameraPermission) {
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .aspectRatio(9f / 16f)
                    .onSizeChanged { size ->
                        previewBoxWidth = size.width
                        previewBoxHeight = size.height
                    }
            ) {
                AndroidView(
                    factory = { previewView },
                    modifier = Modifier.fillMaxSize()
                )
                AndroidView(
                    factory = {
                        FrameLayout(context).apply {
                            layoutParams = ViewGroup.LayoutParams(
                                ViewGroup.LayoutParams.MATCH_PARENT,
                                ViewGroup.LayoutParams.MATCH_PARENT
                            )
                            addView(
                                censorOverlay,
                                FrameLayout.LayoutParams(
                                    FrameLayout.LayoutParams.MATCH_PARENT,
                                    FrameLayout.LayoutParams.MATCH_PARENT
                                )
                            )
                        }
                    },
                    modifier = Modifier.fillMaxSize()
                )
            }
        } else {
            Text(
                text = "Camera permission is required.",
                color = Color(0xFFFF8A80),
                fontSize = 14.sp
            )
        }

        SettingsPanel(
            settings = detectionSettings,
            onSettingsChange = { detectionSettings = it }
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