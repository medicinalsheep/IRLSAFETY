# Quick sanity check: Windows OCR can read text from a generated image.
Add-Type -AssemblyName System.Drawing

$bmp = New-Object System.Drawing.Bitmap 640, 120
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.Clear([System.Drawing.Color]::White)
$font = New-Object System.Drawing.Font("Arial", 36)
$g.DrawString("HELLO WORLD", $font, [System.Drawing.Brushes]::Black, 20, 30)
$g.Dispose()

$path = Join-Path $env:TEMP "irlsafety-ocr-test.png"
$bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()

Add-Type -AssemblyName System.Runtime.WindowsRuntime
$null = [Windows.Storage.StorageFile, Windows.Storage, ContentType=WindowsRuntime]
$null = [Windows.Media.Ocr.OcrEngine, Windows.Media.Ocr, ContentType=WindowsRuntime]
$null = [Windows.Graphics.Imaging.BitmapDecoder, Windows.Graphics.Imaging, ContentType=WindowsRuntime]

[Windows.Storage.StorageFile, Windows.Storage, ContentType=WindowsRuntime] | Out-Null
$asyncOp = [Windows.Storage.StorageFile]::GetFileFromPathAsync($path)
$file = $asyncOp.GetAwaiter().GetResult()
$stream = $file.OpenAsync([Windows.Storage.FileAccessMode]::Read).GetAwaiter().GetResult()
$decoder = [Windows.Graphics.Imaging.BitmapDecoder]::CreateAsync($stream).GetAwaiter().GetResult()
$bitmap = $decoder.GetSoftwareBitmapAsync().GetAwaiter().GetResult()
$engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromUserProfileLanguages()
if (-not $engine) { Write-Error "Windows OCR engine unavailable"; exit 1 }
$result = $engine.RecognizeAsync($bitmap).GetAwaiter().GetResult()
$text = $result.Text
Write-Host "Windows OCR read: '$text'"
if ($text -match "HELLO") { Write-Host "PASS"; exit 0 }
Write-Host "FAIL - OCR did not detect HELLO WORLD"
exit 1