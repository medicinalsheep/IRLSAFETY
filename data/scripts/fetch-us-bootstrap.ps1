# Download US-focused bootstrap images (LISA signs + optional HF plates).
param(
    [string]$PluginRoot = "",
    [string]$TrainingDir = "",
    [switch]$SkipLisa,
    [switch]$SkipPlates
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
$FetchPy = Join-Path $PluginRoot "training\fetch_us_bootstrap.py"

if (-not (Test-Path $FetchPy)) {
    Write-Error "Missing $FetchPy"
}

$args = @("--training-dir", $TrainingDir)
if ($SkipLisa) { $args += "--skip-lisa" }
if ($SkipPlates) { $args += "--skip-plates" }

Write-Host "Fetching US bootstrap data into:" $TrainingDir
python $FetchPy @args
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

python (Join-Path $PluginRoot "training\prepare_dataset.py") --training-dir $TrainingDir --min-labeled 10
exit $LASTEXITCODE