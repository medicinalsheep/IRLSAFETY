<#
.SYNOPSIS
  Install an IRLSAFETY+ Pro license.key into ProgramData.
#>
[CmdletBinding()]
param(
  [Parameter(Mandatory = $true)]
  [string]$KeyFile,
  [string]$DestDir = ""
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $KeyFile)) {
  throw "Key file not found: $KeyFile"
}

$key = (Get-Content -LiteralPath $KeyFile -Raw).Trim()
if (-not $key -or ($key.Split(".").Count -ne 2)) {
  throw "Key file does not look like payload.signature"
}

if (-not $DestDir) {
  $DestDir = Join-Path $env:ProgramData "IRLSAFETY+"
}
New-Item -ItemType Directory -Force -Path $DestDir | Out-Null
$dest = Join-Path $DestDir "license.key"
Set-Content -LiteralPath $dest -Value $key -NoNewline -Encoding ascii
Write-Host "Installed license to $dest" -ForegroundColor Green

$verify = Join-Path $PSScriptRoot "..\pro\license\verify-license.ps1"
if (Test-Path -LiteralPath $verify) {
  & $verify -KeyFile $dest
  if ($LASTEXITCODE -ne 0) {
    Write-Warning "Installed, but verify-license.ps1 failed. Check your secret matches the issuer secret."
    exit $LASTEXITCODE
  }
} else {
  Write-Host "verify-license.ps1 not found beside repo; skip crypto check."
}
