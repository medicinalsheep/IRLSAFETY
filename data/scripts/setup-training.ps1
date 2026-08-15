# IRLSAFETY+ — LOCAL-ONLY training setup. No cloud. No data upload.
param(
    [string]$PluginRoot = "",
    [string]$TrainingDir = ""
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
$TrainingPy = Join-Path $PluginRoot "training"

Write-Host "=== IRLSAFETY+ Local Training Setup ===" -ForegroundColor Cyan
Write-Host "Privacy: all training stays on THIS machine." -ForegroundColor Green
Write-Host ""

if (-not (Test-Path $ModelsDir)) {
    New-Item -ItemType Directory -Path $ModelsDir -Force | Out-Null
}
foreach ($sub in @("images\train", "images\val", "images\staging", "labels\train", "labels\val", "labels\staging", "raw")) {
    $path = Join-Path $TrainingDir $sub
    if (-not (Test-Path $path)) {
        New-Item -ItemType Directory -Path $path -Force | Out-Null
        Write-Host "Created: $path"
    }
}

Write-Host ""
Write-Host "Training folders:" -ForegroundColor Yellow
Write-Host "  Dataset:     $TrainingDir"
Write-Host "  Models out:  $ModelsDir"
Write-Host ""

function Resolve-IrlPython {
    if ($env:IRLSAFETY_PYTHON -and (Test-Path $env:IRLSAFETY_PYTHON)) {
        return $env:IRLSAFETY_PYTHON
    }
    $py = Get-Command py -ErrorAction SilentlyContinue
    if ($py) {
        foreach ($ver in @("-3.12", "-3.11", "-3.10", "-3")) {
            $candidate = & $py.Source $ver -c "import sys; print(sys.executable)" 2>$null
            if ($LASTEXITCODE -eq 0 -and $candidate) { return $candidate.Trim() }
        }
    }
    $known = Join-Path $env:LOCALAPPDATA "Programs\Python\Python312\python.exe"
    if (Test-Path $known) { return $known }
    $cmd = Get-Command python -ErrorAction SilentlyContinue
    if ($cmd -and $cmd.Source -notmatch "Inkscape") { return $cmd.Source }
    return $null
}

$python = Resolve-IrlPython
if (-not $python) {
    Write-Host "Python 3.10+ not found. Install from https://www.python.org/downloads/ and check Add python.exe to PATH." -ForegroundColor Yellow
    exit 1
}

Write-Host "Python: $python"
Write-Host "Installing ultralytics + tools (local only)..."
& $python -m pip install --upgrade pip
& $python -m pip install ultralytics onnx pillow huggingface_hub labelImg

Copy-Item (Join-Path $TrainingPy "classes.txt") (Join-Path $TrainingDir "classes.txt") -Force -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "Done! Next:" -ForegroundColor Green
Write-Host "  scripts\fetch-us-bootstrap.ps1   # US signs + plates starter data"
Write-Host "  scripts\label-images.ps1         # label your OBS frames"
Write-Host "  scripts\train-model.ps1          # train + export ONNX"