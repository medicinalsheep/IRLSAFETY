# IRLSAFETY+ — download PP-OCRv4 English ONNX models for child OCR backend.
param(
    [string]$PluginRoot = ""
)

$ErrorActionPreference = "Stop"

if (-not $PluginRoot) {
    $PluginRoot = Split-Path $PSScriptRoot -Parent
}

$ModelsDir = Join-Path $PluginRoot "models"
New-Item -ItemType Directory -Force -Path $ModelsDir | Out-Null

$DetOut = Join-Path $ModelsDir "irlsafety-ocr-det.onnx"
$RecOut = Join-Path $ModelsDir "irlsafety-ocr-rec.onnx"
$DictOut = Join-Path $ModelsDir "en_dict.txt"

$HfBase = "https://huggingface.co/SWHL/RapidOCR/resolve/main"
$DetUrl = "$HfBase/PP-OCRv4/en_PP-OCRv3_det_infer.onnx"
$RecUrl = "$HfBase/PP-OCRv3/en_PP-OCRv3_rec_infer.onnx"
$DictUrl = "https://raw.githubusercontent.com/PaddlePaddle/PaddleOCR/release/2.7/ppocr/utils/en_dict.txt"

function Download-File($Url, $Dest) {
    Write-Host "Downloading $Url ..."
    $maxAttempts = 3
    for ($i = 1; $i -le $maxAttempts; $i++) {
        try {
            Invoke-WebRequest -Uri $Url -OutFile $Dest -UseBasicParsing -TimeoutSec 120
            return
        } catch {
            if ($i -eq $maxAttempts) { throw }
            Write-Host "Retry $i/$maxAttempts ..."
            Start-Sleep -Seconds 2
        }
    }
}

Write-Host "=== IRLSAFETY+ Fetch Child OCR Models ===" -ForegroundColor Cyan
Write-Host "Target: $ModelsDir" -ForegroundColor Yellow
Write-Host ""

if (-not (Test-Path $DetOut)) {
    Download-File $DetUrl $DetOut
} else {
    Write-Host "Skip (exists): irlsafety-ocr-det.onnx"
}

if (-not (Test-Path $RecOut)) {
    Download-File $RecUrl $RecOut
} else {
    Write-Host "Skip (exists): irlsafety-ocr-rec.onnx"
}

if (-not (Test-Path $DictOut)) {
    try {
        Download-File $DictUrl $DictOut
    } catch {
        Write-Host "Dict download failed - child OCR will use built-in ASCII fallback." -ForegroundColor Yellow
    }
} else {
    Write-Host "Skip (exists): en_dict.txt"
}

Write-Host ""
Write-Host "Done. Build with child OCR backend:" -ForegroundColor Green
Write-Host "  cmake -DIRLSAFETY_OCR_BACKEND=child ..."
Write-Host "Or copy these three files into your OBS plugin models folder."
Write-Host "See models/OCR.txt for usage."