<#
.SYNOPSIS
  Soft gate: exit 0 if a Pro license is valid, 1 otherwise.

.DESCRIPTION
  Checks %ProgramData%\IRLSAFETY+\license.key then optional APPDATA plugin path.
  Full hard-lock inside the .dll is Phase 2 (see pro/README.md).
#>
[CmdletBinding()]
param(
  [string]$KeyFile = "",
  [string]$SecretFile = ""
)

$ErrorActionPreference = "Stop"

$candidates = @()
if ($KeyFile) { $candidates += $KeyFile }
$candidates += (Join-Path $env:ProgramData "IRLSAFETY+\license.key")
$candidates += (Join-Path $env:APPDATA "obs-studio\plugin_config\irlsafety-plus\license.key")

$found = $null
foreach ($c in $candidates) {
  if ($c -and (Test-Path -LiteralPath $c)) { $found = $c; break }
}

$verify = Join-Path $PSScriptRoot "..\pro\license\verify-license.ps1"
if (-not (Test-Path -LiteralPath $verify)) {
  Write-Host "verify-license.ps1 missing" -ForegroundColor Red
  exit 1
}

if (-not $found) {
  Write-Host "No license.key found under ProgramData or plugin_config." -ForegroundColor Red
  Write-Host "See pro/license/where-license-goes.txt"
  exit 1
}

$argsList = @{ KeyFile = $found; Quiet = $true }
if ($SecretFile) { $argsList.SecretFile = $SecretFile }
& $verify @argsList
exit $LASTEXITCODE
