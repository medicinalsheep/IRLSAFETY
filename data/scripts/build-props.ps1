# Download official DMV specimens + generate real-format 4x6 shipping labels.
param(
    [string]$PluginRoot = "",
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"

if (-not $PluginRoot) {
    $PluginRoot = Split-Path $PSScriptRoot -Parent
}

if (-not $OutDir) {
    if ($env:IRLSAFETY_TRAINING_ROOT) {
        $OutDir = Join-Path $env:IRLSAFETY_TRAINING_ROOT "props"
    } elseif (Test-Path "Z:\irlsafety-training") {
        $OutDir = "Z:\irlsafety-training\props"
    } else {
        $OutDir = Join-Path $PluginRoot "training\props"
    }
}

$BuildPy = Join-Path $PluginRoot "training\build_training_props.py"
Write-Host "Building real-world training props..." -ForegroundColor Cyan
Write-Host "Output: $OutDir" -ForegroundColor Yellow
python $BuildPy --out-dir $OutDir
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host "Open $OutDir and print driver_licenses + shipping_labels" -ForegroundColor Green