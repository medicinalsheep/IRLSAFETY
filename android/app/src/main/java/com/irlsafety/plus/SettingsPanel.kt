package com.irlsafety.plus

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Slider
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlin.math.roundToInt

@Composable
fun SettingsPanel(
    settings: DetectionSettings,
    onSettingsChange: (DetectionSettings) -> Unit,
    modifier: Modifier = Modifier
) {
    val scrollState = rememberScrollState()

    Column(
        modifier = modifier
            .fillMaxWidth()
            .verticalScroll(scrollState),
        verticalArrangement = Arrangement.spacedBy(4.dp)
    ) {
        Text(
            text = "Detection settings",
            color = Color(0xFF3ECF8E),
            fontSize = 16.sp,
            modifier = Modifier.padding(bottom = 4.dp)
        )

        SettingSwitch(
            label = "Protection enabled",
            checked = settings.enableAll,
            onCheckedChange = { onSettingsChange(settings.copy(enableAll = it)) }
        )

        HorizontalDivider(color = Color(0xFF2A2D35), modifier = Modifier.padding(vertical = 6.dp))

        SettingSwitch(
            label = "License plates",
            checked = settings.licensePlates,
            enabled = settings.enableAll,
            onCheckedChange = { onSettingsChange(settings.copy(licensePlates = it)) }
        )
        SettingSwitch(
            label = "Street signs",
            checked = settings.streetSigns,
            enabled = settings.enableAll,
            onCheckedChange = { onSettingsChange(settings.copy(streetSigns = it)) }
        )
        SettingSwitch(
            label = "Shipping labels",
            checked = settings.shippingLabels,
            enabled = settings.enableAll,
            onCheckedChange = { onSettingsChange(settings.copy(shippingLabels = it)) }
        )
        SettingSwitch(
            label = "ID documents",
            checked = settings.idDocuments,
            enabled = settings.enableAll,
            onCheckedChange = { onSettingsChange(settings.copy(idDocuments = it)) }
        )

        HorizontalDivider(color = Color(0xFF2A2D35), modifier = Modifier.padding(vertical = 6.dp))

        SettingSlider(
            label = "Confidence",
            valueText = String.format("%.0f%%", settings.confidenceThreshold * 100f),
            value = settings.confidenceThreshold,
            valueRange = 0.15f..0.85f,
            enabled = settings.enableAll,
            onValueChange = { onSettingsChange(settings.copy(confidenceThreshold = it)) }
        )

        SettingSlider(
            label = "Frame skip",
            valueText = "every ${settings.frameSkip} frame(s)",
            value = settings.frameSkip.toFloat(),
            valueRange = 1f..15f,
            steps = 13,
            enabled = settings.enableAll,
            onValueChange = { onSettingsChange(settings.copy(frameSkip = it.roundToInt())) }
        )

        SettingSwitch(
            label = "Prefer GPU (NNAPI)",
            checked = settings.preferGpu,
            enabled = settings.enableAll,
            onCheckedChange = { onSettingsChange(settings.copy(preferGpu = it)) }
        )

        Text(
            text = "Analysis resolution",
            color = if (settings.enableAll) MaterialTheme.colorScheme.onSurface else Color(0xFF6B7280),
            fontSize = 14.sp,
            modifier = Modifier.padding(top = 4.dp)
        )
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            AnalysisResolution.entries.forEach { res ->
                val selected = settings.analysisResolution == res
                TextButton(
                    onClick = { onSettingsChange(settings.copy(analysisResolution = res)) },
                    enabled = settings.enableAll,
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 4.dp),
                    colors = ButtonDefaults.textButtonColors(
                        contentColor = if (selected) Color(0xFF3ECF8E) else Color(0xFF8AB4F8)
                    )
                ) {
                    Text(
                        text = if (selected) "● ${res.label}" else res.label,
                        fontSize = 13.sp
                    )
                }
            }
        }
        Text(
            text = "720p is cooler on mid-range phones; 1080p for finer boxes.",
            color = Color(0xFF6B7280),
            fontSize = 11.sp,
            modifier = Modifier.padding(bottom = 4.dp)
        )

        HorizontalDivider(color = Color(0xFF2A2D35), modifier = Modifier.padding(vertical = 6.dp))

        SettingSwitch(
            label = "Front camera",
            checked = settings.useFrontCamera,
            onCheckedChange = { onSettingsChange(settings.copy(useFrontCamera = it)) }
        )
    }
}

@Composable
private fun SettingSwitch(
    label: String,
    checked: Boolean,
    enabled: Boolean = true,
    onCheckedChange: (Boolean) -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 2.dp),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            text = label,
            color = if (enabled) MaterialTheme.colorScheme.onSurface else Color(0xFF6B7280),
            fontSize = 14.sp
        )
        Switch(
            checked = checked,
            onCheckedChange = onCheckedChange,
            enabled = enabled
        )
    }
}

@Composable
private fun SettingSlider(
    label: String,
    valueText: String,
    value: Float,
    valueRange: ClosedFloatingPointRange<Float>,
    steps: Int = 0,
    enabled: Boolean = true,
    onValueChange: (Float) -> Unit
) {
    Column(modifier = Modifier.fillMaxWidth()) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(
                text = label,
                color = if (enabled) MaterialTheme.colorScheme.onSurface else Color(0xFF6B7280),
                fontSize = 14.sp
            )
            Text(
                text = valueText,
                color = Color(0xFF8AB4F8),
                fontSize = 13.sp
            )
        }
        Slider(
            value = value,
            onValueChange = onValueChange,
            valueRange = valueRange,
            steps = steps,
            enabled = enabled
        )
    }
}