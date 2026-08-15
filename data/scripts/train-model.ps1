# IRLSAFETY+ — full 4-class training pipeline (plates, signs, mail, IDs).
param(
    [string]$PluginRoot = "",
    [string]$TrainingDir = "",
    [switch]$Bootstrap,
    [int]$Epochs = 100,
    [switch]$SkipBootstrap,
    [switch]$OBB,
    [switch]$RequireV07,
    [switch]$WarmStart,
    [switch]$IngestStaging,
    [string]$Device = "",
    [string]$RunsLocal = "",
    [int]$MinShipping = 50,
    [int]$MinId = 30
)

$ErrorActionPreference = "Stop"

if (-not $PluginRoot) {
    $PluginRoot = Split-Path $PSScriptRoot -Parent
}

if (-not $TrainingDir) {
    if ($env:IRLSAFETY_TRAINING_ROOT) {
        $TrainingDir = $env:IRLSAFETY_TRAINING_ROOT
    } else {
        $TrainingDir = Join-Path $env:APPDATA "obs-studio\plugin_config\irlsafety-plus\training"
    }
}
$ModelsDir = Join-Path $PluginRoot "models"
$SetupPs1 = Join-Path $PSScriptRoot "setup-training.ps1"
$FetchPs1 = Join-Path $PSScriptRoot "fetch-us-bootstrap.ps1"
$PreparePy = Join-Path $PluginRoot "training\prepare_dataset.py"
$TrainPy = Join-Path $PluginRoot "training\train_irlsafety.py"

if ($env:IRLSAFETY_PYTHON -and (Test-Path $env:IRLSAFETY_PYTHON)) {
    $PythonExe = $env:IRLSAFETY_PYTHON
} elseif (Get-Command py -ErrorAction SilentlyContinue) {
    $PythonExe = (py -3 -c "import sys; print(sys.executable)" 2>$null)
} else {
    $PythonExe = "python"
}
if (-not $PythonExe) { $PythonExe = "python" }

Write-Host "=== IRLSAFETY+ Train Detection Model (v0.7 — plates, signs, mail, IDs) ===" -ForegroundColor Cyan
Write-Host "Workspace: $TrainingDir" -ForegroundColor Yellow
Write-Host ""

& $SetupPs1 -PluginRoot $PluginRoot -TrainingDir $TrainingDir
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$labeled = (Get-ChildItem (Join-Path $TrainingDir "labels\train") -Filter *.txt -ErrorAction SilentlyContinue).Count
if ($Bootstrap -or (-not $SkipBootstrap -and $labeled -lt 20)) {
    Write-Host "Bootstrap: downloading US sign/plate starter data..." -ForegroundColor Yellow
    & $FetchPs1 -PluginRoot $PluginRoot -TrainingDir $TrainingDir
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Bootstrap had issues — add your own labeled images and re-run." -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "Preparing dataset..."
$prepareArgs = @("--training-dir", $TrainingDir, "--min-labeled", "20")
if ($IngestStaging) { $prepareArgs += "--ingest-staging" }
if ($RequireV07) {
    $prepareArgs += @("--require-v07", "--min-shipping", $MinShipping, "--min-id", $MinId)
}
& $PythonExe $PreparePy @prepareArgs
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "Dataset not ready for training." -ForegroundColor Yellow
    Write-Host "  1. Capture frames in OBS (Control dock -> Capture Frame)"
    Write-Host "  2. scripts\ingest-captures.ps1   (moves staging -> train)"
    Write-Host "  3. scripts\label-images.ps1"
    Write-Host "  4. Re-run with -RequireV07 when mail/ID counts are met"
    exit 1
}

Write-Host ""
$trainArgs = @("--training-dir", $TrainingDir, "--models-dir", $ModelsDir, "--epochs", $Epochs)
if ($Device) { $trainArgs += @("--device", $Device) }
if ($WarmStart) { $trainArgs += "--warm-start" }
if ($RunsLocal) { $trainArgs += @("--runs-dir", $RunsLocal) }

if ($OBB) {
    Write-Host "Training YOLOv8n-OBB (angled labels — labels must be OBB format)..."
    $trainArgs += @("--run-name", "irlsafety_obb")
    & $PythonExe $TrainPy @trainArgs --obb
} else {
    Write-Host "Training YOLOv8n 4-class model (warm-start from prior run if -WarmStart)..."
    & $PythonExe $TrainPy @trainArgs
}
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$ObsModels = Join-Path ${env:ProgramFiles} "obs-studio\data\obs-plugins\irlsafety-plus\models"
$Onnx = Join-Path $ModelsDir "irlsafety-detect.onnx"
if ((Test-Path $Onnx) -and (Test-Path $ObsModels)) {
    Copy-Item $Onnx (Join-Path $ObsModels "irlsafety-detect.onnx") -Force
    Write-Host "Installed to OBS models folder." -ForegroundColor Green
}

Write-Host ""
Write-Host "Done! In OBS: enable Mail + IDs, Angled Cover ON, Reload Model." -ForegroundColor Green
Write-Host "For angled OBB pass: train-model.ps1 -OBB -RequireV07 (after OBB labels)" -ForegroundColor Yellow