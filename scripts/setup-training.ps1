# IRLSAFETY+ — LOCAL-ONLY training setup. No cloud. No data upload.
param(
    [string]$Root = "$PSScriptRoot\.."
)

$ErrorActionPreference = "Stop"
$trainingDir = Join-Path $Root "training"
$imagesDir = Join-Path $trainingDir "images"
$labelsDir = Join-Path $trainingDir "labels"
$modelsDir = Join-Path $Root "data\models"

Write-Host "=== IRLSAFETY+ Local Training Setup ===" -ForegroundColor Cyan
Write-Host "Privacy: all training stays on THIS machine. No cloud upload." -ForegroundColor Green
Write-Host ""

foreach ($dir in @($trainingDir, $imagesDir, $labelsDir, $modelsDir)) {
    if (-not (Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
        Write-Host "Created: $dir"
    }
}

New-Item -ItemType Directory -Path (Join-Path $imagesDir "train") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $imagesDir "val") -Force | Out-Null

Write-Host ""
Write-Host "Checking Python..."
$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) {
    Write-Host "Python not found. Install Python 3.10+ from https://www.python.org/downloads/" -ForegroundColor Yellow
    Write-Host "Re-run this script after installing (check 'Add Python to PATH')."
    exit 1
}

Write-Host "Python: $($python.Source)"
Write-Host ""
Write-Host "Installing ultralytics (local YOLO training only)..."
python -m pip install --upgrade pip
python -m pip install ultralytics onnx labelImg

$datasetYaml = @"
# IRLSAFETY+ — local dataset only. Do not upload these images anywhere.
path: $trainingDir
train: images/train
val: images/val

names:
  0: license_plate
  1: street_sign
  2: document
  3: face
"@

$yamlPath = Join-Path $trainingDir "dataset.yaml"
Set-Content -Path $yamlPath -Value $datasetYaml -Encoding UTF8
Write-Host "Wrote: $yamlPath"

Write-Host ""
Write-Host "Done! Next steps (100% local):" -ForegroundColor Green
Write-Host "  1. Read data\models\TRAINING.txt"
Write-Host "  2. Screenshot YOUR clips → training\images\train"
Write-Host "  3. Label offline:  labelImg   (open train folder, YOLO format)"
Write-Host "  4. Train:  cd training"
Write-Host "            yolo detect train data=dataset.yaml model=yolov8n.pt epochs=100 imgsz=640"
Write-Host "  5. Export: yolo export model=runs\detect\train\weights\best.pt format=onnx imgsz=640 simplify=True"
Write-Host "  6. Copy ONNX → data\models\irlsafety-detect.onnx → OBS models folder"
Write-Host ""
Write-Host "Do NOT upload stream frames to Roboflow or any cloud labeling service" -ForegroundColor Yellow
Write-Host "if you care about PII privacy — use LabelImg on this PC instead."