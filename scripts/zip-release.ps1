# Zip a packaged IRLSAFETY+ folder for GitHub Releases upload.
param(
    [string]$Version = ""
)

$Root = Split-Path $PSScriptRoot -Parent
if (-not $Version) {
    $buildspec = Get-Content (Join-Path $Root "buildspec.json") -Raw | ConvertFrom-Json
    $Version = $buildspec.version
}

$PkgName = "IRLSAFETY+-v$Version-win64"
$PkgDir = Join-Path $Root "release\$PkgName"
$ZipPath = Join-Path $Root "release\$PkgName.zip"

if (-not (Test-Path $PkgDir)) {
    Write-Error "Package folder not found: $PkgDir`nRun scripts\package-windows.bat first."
    exit 1
}

if (Test-Path $ZipPath) {
    Remove-Item $ZipPath -Force
}

Compress-Archive -Path $PkgDir -DestinationPath $ZipPath -CompressionLevel Optimal
Write-Host "Release zip ready: $ZipPath"
Write-Host "Upload to GitHub Releases as asset for tag v$Version"
