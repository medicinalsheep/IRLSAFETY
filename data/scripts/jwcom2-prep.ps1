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
    } elseif (Test-Path "Z:\irlsafety-training") {
        $TrainingDir = "Z:\irlsafety-training"
    } else {
        $TrainingDir = "\\192.168.1.11\R\irlsafety-training"
    }
}

$env:IRLSAFETY_TRAINING_ROOT = $TrainingDir
$PreparePy = Join-Path $PluginRoot "training\prepare_dataset.py"

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
    python $PreparePy --training-dir $TrainingDir --ingest-staging --min-labeled 1
} else {
    $prepareArgs = @("--training-dir", $TrainingDir, "--min-labeled", "1")
    if ($RequireV07) {
        $prepareArgs += @("--require-v07", "--min-shipping", $MinShipping, "--min-id", $MinId)
    }
    python $PreparePy @prepareArgs
}

if ($LASTEXITCODE -ne 0 -and $RequireV07) {
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Yellow
    Write-Host "  JWCOM4: Capture Frame (saves to images/staging on RAM disk if IRLSAFETY_TRAINING_ROOT is set)"
    Write-Host "  JWCOM2: .\jwcom2-prep.ps1 -Ingest"
    Write-Host "  JWCOM2: .\jwcom2-label.ps1"
    Write-Host "  JWCOM2: .\jwcom2-train.ps1 -RequireV07 -WarmStart -Device 0"
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "Ready to label or train." -ForegroundColor Green