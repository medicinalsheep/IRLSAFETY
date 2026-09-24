<#
.SYNOPSIS
  Issue an offline-verifiable IRLSAFETY+ Pro license key.

.DESCRIPTION
  Payload format: IRLSAFE-PRO|<seat>|<expiry-unix-or-0>|<edition>
  Signature: HMAC-SHA256 over the UTF-8 payload, secret from env
  IRLSAFETY_PRO_LICENSE_SECRET or pro/license/.secret

  Human-pasteable key: base64url(payload) + "." + base64url(hmac)

.PARAMETER Seat
  Email or seat id (no pipe characters).

.PARAMETER Edition
  Edition tag (default: pro).

.PARAMETER ExpiryUnix
  Unix seconds expiry, or 0 for no expiry.

.PARAMETER OutFile
  Optional path to write the key string (one line).

.PARAMETER SecretFile
  Optional path to secret file (default: pro/license/.secret next to this script).
#>
[CmdletBinding()]
param(
  [Parameter(Mandatory = $true)]
  [string]$Seat,
  [string]$Edition = "pro",
  [long]$ExpiryUnix = 0,
  [string]$OutFile = "",
  [string]$SecretFile = ""
)

$ErrorActionPreference = "Stop"

function Get-Base64Url([byte[]]$Bytes) {
  $b64 = [Convert]::ToBase64String($Bytes)
  return ($b64.TrimEnd("=") -replace "\+", "-" -replace "/", "_")
}

function Get-LicenseSecret([string]$PathHint) {
  if ($env:IRLSAFETY_PRO_LICENSE_SECRET -and $env:IRLSAFETY_PRO_LICENSE_SECRET.Trim().Length -gt 0) {
    return $env:IRLSAFETY_PRO_LICENSE_SECRET.Trim()
  }
  $path = $PathHint
  if (-not $path) {
    $path = Join-Path $PSScriptRoot ".secret"
  }
  if (-not (Test-Path -LiteralPath $path)) {
    throw "No secret. Set IRLSAFETY_PRO_LICENSE_SECRET or create $path (see .secret.example)."
  }
  return (Get-Content -LiteralPath $path -Raw).Trim()
}

if ($Seat -match "\|") { throw "Seat must not contain '|'." }
if ($Edition -match "\|") { throw "Edition must not contain '|'." }
if ($ExpiryUnix -lt 0) { throw "ExpiryUnix must be >= 0." }

$secret = Get-LicenseSecret $SecretFile
$payload = "IRLSAFE-PRO|$Seat|$ExpiryUnix|$Edition"
$payloadBytes = [Text.Encoding]::UTF8.GetBytes($payload)
$hmac = [System.Security.Cryptography.HMACSHA256]::new([Text.Encoding]::UTF8.GetBytes($secret))
try {
  $sig = $hmac.ComputeHash($payloadBytes)
} finally {
  $hmac.Dispose()
}

$key = (Get-Base64Url $payloadBytes) + "." + (Get-Base64Url $sig)

Write-Output $key

if ($OutFile) {
  $dir = Split-Path -Parent $OutFile
  if ($dir) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
  Set-Content -LiteralPath $OutFile -Value $key -NoNewline -Encoding ascii
  Write-Host "Wrote $OutFile" -ForegroundColor Green
}
