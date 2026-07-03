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
    } elseif (Test-Path "Z:\irlsafety-training") {
        $TrainingDir = "Z:\irlsafety-training"
    } else {
        $TrainingDir = Join-Path $env:APPDATA "obs-studio\plugin_config\irlsafety-plus\training"
    }
}

$PreparePy = Join-Path $PluginRoot "training\prepare_dataset.py"
Write-Host "Ingesting staging captures -> images/train" -ForegroundColor Cyan
Write-Host "Workspace: $TrainingDir" -ForegroundColor Yellow
python $PreparePy --training-dir $TrainingDir --ingest-staging --min-labeled 1