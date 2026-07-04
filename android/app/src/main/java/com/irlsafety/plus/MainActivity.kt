package com.irlsafety.plus

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableLongStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContent {
            MaterialTheme(colorScheme = darkColorScheme()) {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    AlphaShell()
                }
            }
        }
    }
}

@androidx.compose.runtime.Composable
private fun AlphaShell() {
    var pipelineHandle by remember { mutableLongStateOf(0L) }
    var statusLine by remember { mutableStateOf("Starting…") }

    DisposableEffect(Unit) {
        pipelineHandle = IRLSafetyNative.nativeCreatePipeline()
        statusLine = IRLSafetyNative.nativePipelineStatus(pipelineHandle)
        onDispose {
            if (pipelineHandle != 0L) {
                IRLSafetyNative.nativeDestroyPipeline(pipelineHandle)
                pipelineHandle = 0L
            }
        }
    }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(24.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        Text(
            text = "IRLSAFETY+",
            fontSize = 28.sp,
            fontWeight = FontWeight.Bold,
            color = Color(0xFF3ECF8E)
        )
        Text(
            text = IRLSafetyNative.nativeGetApiVersion(),
            color = Color(0xFF8AB4F8),
            fontSize = 14.sp
        )
        Text(
            text = statusLine,
            color = Color(0xFFE8EAED),
            fontSize = 15.sp
        )
        Text(
            text = "P10 scaffold — libirlsafety linked via NDK.\n" +
                "Next: CameraX preview (P11), ONNX + model (P12), GLES boxes (P13).",
            color = Color(0xFF9AA0A6),
            fontSize = 13.sp,
            lineHeight = 18.sp
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