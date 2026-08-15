# Move OBS staging captures into images/train for labeling.
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

$PreparePy = Join-Path $PluginRoot "training\prepare_dataset.py"
if ($env:IRLSAFETY_PYTHON -and (Test-Path $env:IRLSAFETY_PYTHON)) {
    $PythonExe = $env:IRLSAFETY_PYTHON
} elseif (Get-Command py -ErrorAction SilentlyContinue) {
    $PythonExe = (py -3 -c "import sys; print(sys.executable)" 2>$null)
} else {
    $PythonExe = "python"
}
if (-not $PythonExe) { $PythonExe = "python" }
Write-Host "Ingesting staging captures -> images/train" -ForegroundColor Cyan
Write-Host "Workspace: $TrainingDir" -ForegroundColor Yellow
& $PythonExe $PreparePy --training-dir $TrainingDir --ingest-staging --min-labeled 1