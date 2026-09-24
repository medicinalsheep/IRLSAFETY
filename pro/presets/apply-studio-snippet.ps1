<#
.SYNOPSIS
  Emit an OBS-compatible settings snippet for the Studio Pro preset.
#>
[CmdletBinding()]
param(
  [string]$PresetPath = "",
  [string]$OutFile = ""
)

$ErrorActionPreference = "Stop"

if (-not $PresetPath) {
  $repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
  $candidate = Join-Path $repo "data\presets\studio-pro.json"
  if (Test-Path -LiteralPath $candidate) {
    $PresetPath = $candidate
  } else {
    $PresetPath = Join-Path $PSScriptRoot "studio.json"
  }
}

if (-not (Test-Path -LiteralPath $PresetPath)) {
  throw "Preset not found: $PresetPath"
}

$preset = Get-Content -LiteralPath $PresetPath -Raw | ConvertFrom-Json
$settings = $preset.obs_filter_settings
$snippet = [ordered]@{}
$settings.PSObject.Properties | ForEach-Object { $snippet[$_.Name] = $_.Value }

$json = ($snippet | ConvertTo-Json -Depth 5)
Write-Output "Studio Pro OBS filter settings snippet"
Write-Output "Source: $PresetPath"
Write-Output "----------------------------------------"
Write-Output $json
Write-Output "----------------------------------------"
Write-Output "Apply manually in OBS filter properties, or paste into a scene collection JSON filter settings object (advanced)."

if ($OutFile) {
  Set-Content -LiteralPath $OutFile -Value $json -Encoding utf8
  Write-Host "Wrote $OutFile" -ForegroundColor Green
}
