# Compatibility wrapper around build-props.ps1 (kept for existing local kits).
param([string]$PluginRoot = "")
$ErrorActionPreference = "Stop"
if (-not $PluginRoot) { $PluginRoot = Split-Path $PSScriptRoot -Parent }
& (Join-Path $PSScriptRoot "build-props.ps1") -PluginRoot $PluginRoot