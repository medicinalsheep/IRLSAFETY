# IRLSAFETY+ — v0.7 session prep: audit dataset, ingest captures, show what's missing.
param(
    [string]$PluginRoot = "",
    [string]$TrainingDir = "",
    [switch]$Ingest,
    [switch]$RequireV07,
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

$env:IRLSAFETY_TRAINING_ROOT = $TrainingDir
$PreparePy = Join-Path $PluginRoot "training\prepare_dataset.py"
if ($env:IRLSAFETY_PYTHON -and (Test-Path $env:IRLSAFETY_PYTHON)) {
    $PythonExe = $env:IRLSAFETY_PYTHON
} elseif (Get-Command py -ErrorAction SilentlyContinue) {
    $PythonExe = (py -3 -c "import sys; print(sys.executable)" 2>$null)
} else {
    $PythonExe = "python"
}
if (-not $PythonExe) { $PythonExe = "python" }

Write-Host ""
Write-Host "=== IRLSAFETY+ v0.7 Session Prep ===" -ForegroundColor Cyan
Write-Host "Workspace: $TrainingDir" -ForegroundColor Yellow
Write-Host ""

$staging = Join-Path $TrainingDir "images\staging"
$stagingCount = 0
if (Test-Path $staging) {
    $stagingCount = (Get-ChildItem $staging -File -ErrorAction SilentlyContinue).Count
}
Write-Host "Staging captures (unlabeled): $stagingCount" -ForegroundColor $(if ($stagingCount -gt 0) { "Green" } else { "Gray" })

if ($Ingest -and $stagingCount -gt 0) {
    Write-Host "Ingesting staging -> train..." -ForegroundColor Yellow
    & $PythonExe $PreparePy --training-dir $TrainingDir --ingest-staging --min-labeled 1
} else {
    $prepareArgs = @("--training-dir", $TrainingDir, "--min-labeled", "1")
    if ($RequireV07) {
        $prepareArgs += @("--require-v07", "--min-shipping", $MinShipping, "--min-id", $MinId)
    }
    & $PythonExe $PreparePy @prepareArgs
}

if ($LASTEXITCODE -ne 0 -and $RequireV07) {
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Yellow
    Write-Host "  1. OBS Control dock -> Capture Frame (images/staging)"
    Write-Host "  2. scripts\label-images.ps1"
    Write-Host "  3. data\scripts\ingest-captures.ps1   (or this script -Ingest)"
    Write-Host "  4. scripts\train-model.ps1 -RequireV07 -WarmStart -Device 0"
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "Ready to label or train." -ForegroundColor Green