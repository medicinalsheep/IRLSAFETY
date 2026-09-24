<#
.SYNOPSIS
  Compare a model update manifest to the locally installed model version.

.DESCRIPTION
  Reads a manifest JSON (file path or http/https URL). Compares "version" to
  an installed version file if present. Does not download binaries unless
  -Download is passed together with -Confirm; downloads always verify sha256.
#>
[CmdletBinding()]
param(
  [Parameter(Mandatory = $true)]
  [string]$Manifest,
  [string]$InstalledVersionFile = "",
  [string]$DownloadDir = "",
  [switch]$Download,
  [switch]$Confirm
)

$ErrorActionPreference = "Stop"

function Get-ManifestObject([string]$Source) {
  if ($Source -match "^https?://") {
    Write-Host "Fetching manifest: $Source"
    $resp = Invoke-WebRequest -Uri $Source -UseBasicParsing
    return ($resp.Content | ConvertFrom-Json)
  }
  if (-not (Test-Path -LiteralPath $Source)) {
    throw "Manifest file not found: $Source"
  }
  return (Get-Content -LiteralPath $Source -Raw | ConvertFrom-Json)
}

function Get-FileSha256([string]$Path) {
  return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
if (-not $InstalledVersionFile) {
  $candidates = @(
    (Join-Path $env:ProgramData "IRLSAFETY+\models\installed-version.txt"),
    (Join-Path $repo "data\models\installed-version.txt")
  )
  foreach ($c in $candidates) {
    if (Test-Path -LiteralPath $c) { $InstalledVersionFile = $c; break }
  }
}

$man = Get-ManifestObject $Manifest
$remoteVer = [string]$man.version
$channel = [string]$man.channel
$remoteSha = ([string]$man.sha256).ToLowerInvariant()
$remoteUrl = [string]$man.url

$localVer = ""
if ($InstalledVersionFile -and (Test-Path -LiteralPath $InstalledVersionFile)) {
  $localVer = (Get-Content -LiteralPath $InstalledVersionFile -Raw).Trim()
}

Write-Host "Channel:          $(if ($channel) { $channel } else { '(none)' })"
Write-Host "Manifest version: $remoteVer"
Write-Host "Installed:        $(if ($localVer) { $localVer } else { '(no installed-version.txt found)' })"
Write-Host "Released:         $($man.released)"
if ($man.notes) { Write-Host "Notes:            $($man.notes)" }

$updateAvailable = $false
if (-not $localVer) {
  Write-Host "Status:           unknown local version (treat as update available if you trust the manifest)" -ForegroundColor Yellow
  $updateAvailable = $true
} elseif ($localVer -ne $remoteVer) {
  Write-Host "Status:           UPDATE AVAILABLE" -ForegroundColor Yellow
  $updateAvailable = $true
} else {
  Write-Host "Status:           up to date" -ForegroundColor Green
}

if ($Download) {
  if (-not $Confirm) {
    throw "Refusing download without -Confirm. Example: -Download -Confirm"
  }
  if (-not $remoteUrl -or $remoteUrl -match "example.com") {
    throw "Manifest url is missing or still the example placeholder."
  }
  if (-not $remoteSha -or $remoteSha -match "REPLACE_") {
    throw "Manifest sha256 is missing or still the example placeholder."
  }
  if (-not $DownloadDir) {
    $DownloadDir = Join-Path $env:TEMP "IRLSAFETY-model-update"
  }
  New-Item -ItemType Directory -Force -Path $DownloadDir | Out-Null
  $dest = Join-Path $DownloadDir ("irlsafety-detect-" + ($remoteVer -replace "[^\w\.-]", "_") + ".onnx")
  Write-Host "Downloading to $dest ..."
  Invoke-WebRequest -Uri $remoteUrl -OutFile $dest -UseBasicParsing
  $got = Get-FileSha256 $dest
  if ($got -ne $remoteSha) {
    Remove-Item -LiteralPath $dest -Force -ErrorAction SilentlyContinue
    throw "Checksum mismatch. expected=$remoteSha got=$got (file deleted)"
  }
  Write-Host "Download OK (sha256 matched): $dest" -ForegroundColor Green
  Write-Host "Copy into the plugin models folder and Reload Model in OBS. Update installed-version.txt yourself."
}

if ($updateAvailable) { exit 0 } else { exit 0 }
