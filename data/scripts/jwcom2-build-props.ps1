# Kit-root wrapper — copy to playground as jwcom2-build-props.ps1
param([string]$PluginRoot = "")
$ErrorActionPreference = "Stop"
if (-not $PluginRoot) { $PluginRoot = Split-Path $PSScriptRoot -Parent }
& (Join-Path $PSScriptRoot "build-props.ps1") -PluginRoot $PluginRoot