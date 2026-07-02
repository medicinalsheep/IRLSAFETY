# IRLSAFETY+ — full US plate/sign training pipeline (local only).
param(
    [string]$PluginRoot = "",
    [string]$TrainingDir = "",
    [switch]$Bootstrap,
    [int]$Epochs = 80,
    [switch]$SkipBootstrap
)

$ErrorActionPreference = "Stop"

if (-not $PluginRoot) {
    $PluginRoot = Split-Path $PSScriptRoot -Parent
}

if (-not $TrainingDir) {
    if ($env:IRLSAFETY_TRAINING_ROOT) {
        $TrainingDir = $env:IRLSAFETY_TRAINING_ROOT
    } elseif (Test-Path "Z:\irlsafety-training") {
        $TrainingDir = "Z:\irlsafety-training"
    } else {
        $TrainingDir = Join-Path $env:APPDATA "obs-studio\plugin_config\irlsafety-plus\training"
    }
}
$ModelsDir = Join-Path $PluginRoot "models"
$SetupPs1 = Join-Path $PSScriptRoot "setup-training.ps1"
$FetchPs1 = Join-Path $PSScriptRoot "fetch-us-bootstrap.ps1"
$PreparePy = Join-Path $PluginRoot "training\prepare_dataset.py"
$TrainPy = Join-Path $PluginRoot "training\train_irlsafety.py"

Write-Host "=== IRLSAFETY+ Train US Detection Model ===" -ForegroundColor Cyan
Write-Host "Workspace: $TrainingDir" -ForegroundColor Yellow
Write-Host ""

& $SetupPs1 -PluginRoot $PluginRoot
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$labeled = (Get-ChildItem (Join-Path $TrainingDir "labels\train") -Filter *.txt -ErrorAction SilentlyContinue).Count
if ($Bootstrap -or (-not $SkipBootstrap -and $labeled -lt 20)) {
    Write-Host "Bootstrap: downloading US sign/plate starter data..." -ForegroundColor Yellow
    & $FetchPs1 -PluginRoot $PluginRoot
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Bootstrap had issues — add your own labeled images and re-run." -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "Preparing dataset..."
python $PreparePy --training-dir $TrainingDir --min-labeled 20
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "Not enough labeled data yet." -ForegroundColor Yellow
    Write-Host "  1. Capture frames in OBS (Control dock -> Capture Frame)"
    Write-Host "  2. scripts\label-images.ps1"
    Write-Host "  3. Re-run scripts\train-model.ps1"
    exit 1
}

Write-Host ""
Write-Host "Training YOLOv8n (US plates + signs)..."
python $TrainPy --training-dir $TrainingDir --models-dir $ModelsDir --epochs $Epochs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$ObsModels = Join-Path ${env:ProgramFiles} "obs-studio\data\obs-plugins\irlsafety-plus\models"
$Onnx = Join-Path $ModelsDir "irlsafety-detect.onnx"
if ((Test-Path $Onnx) -and (Test-Path $ObsModels)) {
    Copy-Item $Onnx (Join-Path $ObsModels "irlsafety-detect.onnx") -Force
    Write-Host "Installed to OBS models folder." -ForegroundColor Green
}

Write-Host ""
Write-Host "Done! In OBS: enable License Plates + Street Signs, Reload Model in Control dock." -ForegroundColor Green