# Launch LabelImg for IRLSAFETY+ training (YOLO format).
param(
    [string]$PluginRoot = ""
)

$ErrorActionPreference = "Stop"

if (-not $PluginRoot) {
    $PluginRoot = Split-Path $PSScriptRoot -Parent
}

if ($env:IRLSAFETY_TRAINING_ROOT) {
    $TrainingDir = $env:IRLSAFETY_TRAINING_ROOT
} elseif (Test-Path "Z:\irlsafety-training") {
    $TrainingDir = "Z:\irlsafety-training"
} else {
    $TrainingDir = Join-Path $env:APPDATA "obs-studio\plugin_config\irlsafety-plus\training"
}

$StagingImages = Join-Path $TrainingDir "images\staging"
$StagingLabels = Join-Path $TrainingDir "labels\staging"
$TrainImages = Join-Path $TrainingDir "images\train"
$TrainLabels = Join-Path $TrainingDir "labels\train"
$ClassesFile = Join-Path $PluginRoot "training\classes.txt"

$stagingCount = 0
if (Test-Path $StagingImages) {
    $stagingCount = (Get-ChildItem $StagingImages -File -ErrorAction SilentlyContinue).Count
}

if ($stagingCount -gt 0) {
    $ImagesDir = $StagingImages
    $LabelsDir = $StagingLabels
    Write-Host "Labeling staging captures ($stagingCount new frame(s))." -ForegroundColor Yellow
    Write-Host "When done: scripts\ingest-captures.ps1  (moves labeled pairs into train)" -ForegroundColor Yellow
} else {
    $ImagesDir = $TrainImages
    $LabelsDir = $TrainLabels
}

foreach ($d in @($ImagesDir, $LabelsDir, $TrainImages, $TrainLabels, $StagingImages, $StagingLabels)) {
    if (-not (Test-Path $d)) { New-Item -ItemType Directory -Path $d -Force | Out-Null }
}

Write-Host "LabelImg — v0.7 classes (exact names):" -ForegroundColor Cyan
Get-Content $ClassesFile | Where-Object { $_ -and -not $_.StartsWith("#") } | ForEach-Object { Write-Host "  $_" }

Write-Host ""
Write-Host "Training dir: $TrainingDir"
Write-Host "Images:       $ImagesDir"
Write-Host "Labels:       $LabelsDir"
Write-Host "Format: YOLO | Hotkey W = box | Ctrl+S = save"
Write-Host ""

$labelImg = Get-Command labelImg -ErrorAction SilentlyContinue
if ($labelImg) {
    Start-Process -FilePath $labelImg.Source -ArgumentList $ImagesDir, $ClassesFile, $LabelsDir
} else {
    Write-Host "Starting via python -m labelImg..."
    python -m labelImg $ImagesDir $ClassesFile $LabelsDir
}