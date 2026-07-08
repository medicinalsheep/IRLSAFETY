# Install a CI-built Windows zip (irlsafety-plus-*-windows-x64.zip) into OBS Studio.
# Layout from OBS plugin template:
#   irlsafety-plus/bin/64bit/*.dll
#   irlsafety-plus/data/...
# Maps to:
#   %ProgramFiles%\obs-studio\obs-plugins\64bit\
#   %ProgramFiles%\obs-studio\data\obs-plugins\irlsafety-plus\

param(
    [Parameter(Mandatory = $true)]
    [string]$ZipPath,
    [string]$ObsRoot = "$env:ProgramFiles\obs-studio"
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path $ZipPath)) {
    throw "Zip not found: $ZipPath"
}
if (-not (Test-Path "$ObsRoot\bin\64bit\obs64.exe")) {
    throw "OBS not found at $ObsRoot"
}

$work = Join-Path $env:TEMP ("irlsafety-install-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
try {
    Expand-Archive -Path $ZipPath -DestinationPath $work -Force
    $pkg = Get-ChildItem $work -Directory | Where-Object { Test-Path (Join-Path $_.FullName 'bin\64bit') } | Select-Object -First 1
    if (-not $pkg) {
        throw "Could not find irlsafety-plus/bin/64bit inside zip"
    }

    $destDll = Join-Path $ObsRoot 'obs-plugins\64bit'
    $destData = Join-Path $ObsRoot 'data\obs-plugins\irlsafety-plus'
    if (-not (Test-Path $destDll)) { New-Item -ItemType Directory -Path $destDll | Out-Null }
    if (-not (Test-Path $destData)) { New-Item -ItemType Directory -Path $destData | Out-Null }

    Copy-Item (Join-Path $pkg.FullName 'bin\64bit\*') $destDll -Force
    Copy-Item (Join-Path $pkg.FullName 'data\*') $destData -Recurse -Force

    Write-Host "Installed IRLSAFETY+ into:"
    Write-Host "  $destDll"
    Write-Host "  $destData"
    Write-Host "Restart OBS, then Filters -> + -> IRLSAFETY+ PII Blur"
}
finally {
    Remove-Item $work -Recurse -Force -ErrorAction SilentlyContinue
}
