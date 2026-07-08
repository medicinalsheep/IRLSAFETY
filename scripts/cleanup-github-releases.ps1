# Remove old GitHub Releases (keeps a short allowlist).
# Requires: gh CLI — https://cli.github.com/ then: gh auth login
#
# Usage:
#   .\scripts\cleanup-github-releases.ps1
#   .\scripts\cleanup-github-releases.ps1 -KeepTags v0.9.5,v0.9.6-dev -WhatIf

param(
    [string[]]$KeepTags = @('v0.9.6-dev', 'v0.9.5', 'v0.9.4', '0.9.4', 'v0.9.5-dev', 'v0.7.1'),
    [switch]$WhatIf
)

$ErrorActionPreference = 'Stop'
$Repo = 'medicinalsheep/IRLSAFETY'

if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
    Write-Error @"
GitHub CLI (gh) is not installed.
Install: https://cli.github.com/
Then run: gh auth login
Or delete releases manually — see docs/GITHUB.md
"@
}

gh auth status 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Error 'Run: gh auth login'
}

Write-Host "Fetching releases from $Repo ..."
$releases = gh api "repos/$Repo/releases" --paginate | ConvertFrom-Json
if (-not $releases) {
    Write-Host 'No releases found.'
    exit 0
}

foreach ($release in $releases) {
    $tag = $release.tag_name
    $id = $release.id
    $name = $release.name

    if ($KeepTags -contains $tag) {
        Write-Host "[KEEP] $tag - $name"
        continue
    }

    if ($WhatIf) {
        Write-Host "[WOULD DELETE] $tag - $name (id $id)"
        continue
    }

    Write-Host "[DELETE] $tag - $name"
    gh api -X DELETE "repos/$Repo/releases/$id"

    # Release delete does not remove the git tag.
    try {
        gh api -X DELETE "repos/$Repo/git/refs/tags/$tag" 2>$null
        Write-Host "         removed tag $tag"
    } catch {
        Write-Host "         tag $tag not removed (may not exist)"
    }
}

Write-Host ''
Write-Host 'Done. Refresh: https://github.com/medicinalsheep/IRLSAFETY/releases'