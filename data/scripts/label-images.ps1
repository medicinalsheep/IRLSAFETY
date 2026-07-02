# Launch LabelImg for IRLSAFETY+ training (YOLO format).
param(
    [string]$PluginRoot = ""
)

$ErrorActionPreference = "Stop"

if (-not $PluginRoot) {
    $PluginRoot = Split-Path $PSScriptRoot -Parent
}

$TrainingDir = Join-Path $env:APPDATA "obs-studio\plugin_config\irlsafety-plus\training"
$ImagesDir = Join-Path $TrainingDir "images\train"
$LabelsDir = Join-Path $TrainingDir "labels\train"
$ClassesFile = Join-Path $PluginRoot "training\classes.txt"

foreach ($d in @($ImagesDir, $LabelsDir)) {
    if (-not (Test-Path $d)) { New-Item -ItemType Directory -Path $d -Force | Out-Null }
}

Write-Host "LabelImg — US classes (use these exact names):" -ForegroundColor Cyan
Get-Content $ClassesFile | ForEach-Object { Write-Host "  $_" }

Write-Host ""
Write-Host "Images: $ImagesDir"
Write-Host "Save labels to: $LabelsDir"
Write-Host "Format: YOLO | Hotkey W = box | Ctrl+S = save"
Write-Host ""

$labelImg = Get-Command labelImg -ErrorAction SilentlyContinue
if ($labelImg) {
    Start-Process -FilePath $labelImg.Source -ArgumentList $ImagesDir, $ClassesFile, $LabelsDir
} else {
    Write-Host "Starting via python -m labelImg..."
    python -m labelImg $ImagesDir $ClassesFile $LabelsDir
}