# IRLSAFETY+ — LOCAL-ONLY training setup. No cloud. No data upload.
param(
    [string]$PluginRoot = ""
)

$ErrorActionPreference = "Stop"

if (-not $PluginRoot) {
    $PluginRoot = Split-Path $PSScriptRoot -Parent
}

$TrainingDir = Join-Path $env:APPDATA "obs-studio\plugin_config\irlsafety-plus\training"
$RepoTraining = Join-Path (Split-Path $PluginRoot -Parent) "training"
if (Test-Path (Join-Path (Split-Path $PluginRoot -Parent) "CMakeLists.txt")) {
    $RepoTraining = Join-Path (Split-Path $PluginRoot -Parent) "training"
}

$ModelsDir = Join-Path $PluginRoot "models"
$TrainingPy = Join-Path $PluginRoot "training"

Write-Host "=== IRLSAFETY+ Local Training Setup ===" -ForegroundColor Cyan
Write-Host "Privacy: all training stays on THIS machine." -ForegroundColor Green
Write-Host ""

foreach ($dir in @($TrainingDir, $ModelsDir)) {
    foreach ($sub in @("images\train", "images\val", "labels\train", "labels\val", "raw")) {
        $path = Join-Path $dir $sub
        if (-not (Test-Path $path)) {
            New-Item -ItemType Directory -Path $path -Force | Out-Null
            Write-Host "Created: $path"
        }
    }
}

if ((Test-Path (Join-Path (Split-Path $PluginRoot -Parent) "CMakeLists.txt")) -and $RepoTraining) {
    foreach ($sub in @("images\train", "images\val", "labels\train", "labels\val", "raw")) {
        $path = Join-Path $RepoTraining $sub
        if (-not (Test-Path $path)) { New-Item -ItemType Directory -Path $path -Force | Out-Null }
    }
}

Write-Host ""
Write-Host "Training folders:" -ForegroundColor Yellow
Write-Host "  OBS captures:  $TrainingDir"
if ($RepoTraining) { Write-Host "  Repo (dev):    $RepoTraining" }
Write-Host "  Models out:    $ModelsDir"
Write-Host ""

$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) {
    Write-Host "Python not found. Install Python 3.10+ from https://www.python.org/downloads/" -ForegroundColor Yellow
    exit 1
}

Write-Host "Python: $($python.Source)"
Write-Host "Installing ultralytics + tools (local only)..."
python -m pip install --upgrade pip
python -m pip install ultralytics onnx pillow huggingface_hub labelImg

Copy-Item (Join-Path $TrainingPy "classes.txt") (Join-Path $TrainingDir "classes.txt") -Force -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "Done! Next:" -ForegroundColor Green
Write-Host "  scripts\fetch-us-bootstrap.ps1   # US signs + plates starter data"
Write-Host "  scripts\label-images.ps1         # label your OBS frames"
Write-Host "  scripts\train-model.ps1          # train + export ONNX"