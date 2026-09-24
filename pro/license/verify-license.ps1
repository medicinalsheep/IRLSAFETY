<#
.SYNOPSIS
  Verify an IRLSAFETY+ Pro license key string or file.

.DESCRIPTION
  Checks HMAC-SHA256 and optional expiry. Exit 0 if valid, 1 otherwise.
#>
[CmdletBinding()]
param(
  [string]$Key = "",
  [string]$KeyFile = "",
  [string]$SecretFile = "",
  [switch]$Quiet
)

$ErrorActionPreference = "Stop"

function Get-Base64UrlDecode([string]$Text) {
  $s = $Text.Replace("-", "+").Replace("_", "/")
  switch ($s.Length % 4) {
    2 { $s += "==" }
    3 { $s += "=" }
  }
  return [Convert]::FromBase64String($s)
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
    throw "No secret. Set IRLSAFETY_PRO_LICENSE_SECRET or create $path."
  }
  return (Get-Content -LiteralPath $path -Raw).Trim()
}

function Write-Result([bool]$Ok, [string]$Message) {
  if (-not $Quiet) {
    if ($Ok) { Write-Host $Message -ForegroundColor Green }
    else { Write-Host $Message -ForegroundColor Red }
  }
  if ($Ok) { exit 0 } else { exit 1 }
}

if (-not $Key -and -not $KeyFile) {
  $default = Join-Path $env:ProgramData "IRLSAFETY+\license.key"
  if (Test-Path -LiteralPath $default) { $KeyFile = $default }
}

if ($KeyFile) {
  if (-not (Test-Path -LiteralPath $KeyFile)) {
    Write-Result $false "Key file not found: $KeyFile"
  }
  $Key = (Get-Content -LiteralPath $KeyFile -Raw).Trim()
}

if (-not $Key) {
  Write-Result $false "No key provided. Pass -Key, -KeyFile, or install to %ProgramData%\IRLSAFETY+\license.key"
}

$parts = $Key.Split(".")
if ($parts.Count -ne 2) {
  Write-Result $false "Key must be payload.signature (base64url)."
}

try {
  $payloadBytes = Get-Base64UrlDecode $parts[0]
  $sigBytes = Get-Base64UrlDecode $parts[1]
} catch {
  Write-Result $false "Key is not valid base64url."
}

$payload = [Text.Encoding]::UTF8.GetString($payloadBytes)
$fields = $payload.Split("|")
if ($fields.Count -ne 4 -or $fields[0] -ne "IRLSAFE-PRO") {
  Write-Result $false "Payload shape invalid (want IRLSAFE-PRO|seat|expiry|edition)."
}

$secret = Get-LicenseSecret $SecretFile
$hmac = [System.Security.Cryptography.HMACSHA256]::new([Text.Encoding]::UTF8.GetBytes($secret))
try {
  $expect = $hmac.ComputeHash($payloadBytes)
} finally {
  $hmac.Dispose()
}

if ($expect.Length -ne $sigBytes.Length) {
  Write-Result $false "Signature length mismatch."
}
$diff = 0
for ($i = 0; $i -lt $expect.Length; $i++) { $diff = $diff -bor ($expect[$i] -bxor $sigBytes[$i]) }
if ($diff -ne 0) {
  Write-Result $false "Signature invalid (wrong secret or tampered key)."
}

$expiry = 0L
if (-not [long]::TryParse($fields[2], [ref]$expiry)) {
  Write-Result $false "Expiry field is not an integer."
}
if ($expiry -gt 0) {
  $now = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
  if ($now -gt $expiry) {
    Write-Result $false "License expired (expiry unix $expiry)."
  }
}

if (-not $Quiet) {
  Write-Host "VALID" -ForegroundColor Green
  Write-Host "  seat:    $($fields[1])"
  Write-Host "  edition: $($fields[3])"
  Write-Host "  expiry:  $(if ($expiry -eq 0) { 'none' } else { $expiry })"
}
exit 0
