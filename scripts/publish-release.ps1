# Publish a GitHub Release with the packaged Windows zip.
param(
    [string]$Version = "",
    [string]$ZipPath = ""
)

$ErrorActionPreference = "Stop"
$Root = Split-Path $PSScriptRoot -Parent

if (-not $Version) {
    $buildspec = Get-Content (Join-Path $Root "buildspec.json") -Raw | ConvertFrom-Json
    $Version = $buildspec.version
}

$Tag = "v$Version"
$AssetName = "IRLSAFETY+-v$Version-win64.zip"
if (-not $ZipPath) {
    $ZipPath = Join-Path $Root "release\$AssetName"
}

if (-not (Test-Path $ZipPath)) {
    Write-Error "Zip not found: $ZipPath"
}

$credInput = "protocol=https`nhost=github.com`n`n"
$credOutput = $credInput | git -C $Root credential fill 2>$null
$token = ($credOutput | Select-String '^password=').ToString().Substring(9)

$gh = Join-Path $env:TEMP "gh-cli\bin\gh.exe"
if (-not (Test-Path $gh)) {
    $ghZip = Join-Path $env:TEMP "gh_windows_amd64.zip"
    $ghDir = Join-Path $env:TEMP "gh-cli"
    Invoke-WebRequest -Uri "https://github.com/cli/cli/releases/download/v2.95.0/gh_2.95.0_windows_amd64.zip" -OutFile $ghZip
    Expand-Archive -Path $ghZip -DestinationPath $ghDir -Force
}

$notesFile = Join-Path $env:TEMP "irlsafety-release-notes.md"
$notes = @"
## IRLSAFETY+ $Tag

### Highlights
- **Low-end defaults** - tuned for **4-6 GB VRAM** (GTX 1650 / RX 580 class): Frame Skip **8**, Screen Text **OFF**, DirectML **ON**
- **Performance** - YOLO at 640px; detection runs while OCR busy; hybrid delay + solid-box censor
- **4-class detection model** bundled (irlsafety-detect.onnx: plates, signs, mail labels, IDs)
- **Tray panel** + virtual camera hooks; child OCR ONNX optional toggle
- **Angled cover** - quad-masked censor for tilted packages

### Model (irlsafety_v07)
- 100-epoch warm-start training, mAP50 ~0.98
- Classes: license_plate, street_sign, shipping_label, id_document

### Install
1. Download **$AssetName** below
2. Extract the folder
3. Right-click **install-from-package.bat** and Run as administrator
4. Restart OBS
5. Add filter, enable categories, Control dock -> **Reload Model**
6. Remove and re-add the filter on existing sources to pick up new defaults

### Quick test
- Hold a shipping label or ID prop to camera (see training/props/ in the package)
- Enable **Debug Logging** - look for Object detection region hits in the log
"@
Set-Content -Path $notesFile -Value $notes -Encoding UTF8

$origin = git -C $Root remote get-url origin 2>$null
if ($origin -notmatch 'github\.com[:/]([^/]+)/([^/.]+?)(?:\.git)?$') {
    Write-Error "Could not detect GitHub owner/repo from git remote origin."
}
$Repo = "$($Matches[1])/$($Matches[2])"

$env:GH_TOKEN = $token
& $gh release create $Tag `
    --repo $Repo `
    --title "IRLSAFETY+ $Tag" `
    --notes-file $notesFile `
    $ZipPath

Write-Host "Release published: https://github.com/$Repo/releases/tag/$Tag"