# Download or guide setup for IRLSAFETY+ YOLOv8 detection ONNX model.
param(
    [string]$OutFile = "$PSScriptRoot\..\data\models\irlsafety-detect.onnx"
)

$ErrorActionPreference = "Stop"
$outDir = Split-Path -Parent $OutFile
if (-not (Test-Path $outDir)) {
    New-Item -ItemType Directory -Path $outDir -Force | Out-Null
}

Write-Host "IRLSAFETY+ detection model setup"
Write-Host "Target: $OutFile"
Write-Host ""

# Public YOLOv8n COCO baseline (smoke-test only — NOT trained for plates/signs).
$fallbackUrl = "https://github.com/ultralytics/assets/releases/download/v8.3.0/yolov8n.onnx"

if (Test-Path $OutFile) {
    Write-Host "Model already exists: $OutFile"
    exit 0
}

Write-Host "No bundled plate/sign weights yet — downloading YOLOv8n baseline for pipeline smoke-test..."
Write-Host "For real license plate / street sign detection, train or export a custom model (see data\models\README.txt)."
Write-Host ""

try {
    Invoke-WebRequest -Uri $fallbackUrl -OutFile $OutFile -UseBasicParsing
    Write-Host "Saved: $OutFile"
    Write-Host ""
    Write-Host "Next: rebuild/package, install to OBS, enable License Plates, test with Debug Logging."
} catch {
    Write-Host "Download failed: $_"
    Write-Host ""
    Write-Host "Manual export:"
    Write-Host "  pip install ultralytics"
    Write-Host "  yolo export model=yolov8n.pt format=onnx imgsz=640 simplify=True"
    Write-Host "  copy yolov8n.onnx to data\models\irlsafety-detect.onnx"
    exit 1
}