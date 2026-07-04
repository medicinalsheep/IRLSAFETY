# GitHub — Releases & CI (simplified)

**Last updated:** 2026-07-03

---

## Download Android APK

1. Open **Actions** → **Android APK Release** → **Run workflow** → Run  
2. Wait ~10–15 minutes for green checkmark  
3. Open **Releases**: https://github.com/medicinalsheep/IRLSAFETY/releases  
4. Open **IRLSAFETY+ 0.9.5-dev (Android)** prerelease  
5. Download **`IRLSAFETY+-0.9.5-dev-android.apk`** under Assets  

If Releases is still empty, download the same file from the workflow run’s **Artifacts** section.

Install: **android/TESTER.md**

---

## Download Windows OBS plugin

Latest Windows package is a separate release (e.g. **v0.9.4**).  
Built locally: `scripts\build-windows.bat` + release zip.

---

## CI workflows (what runs automatically)

| Workflow | Auto-run? | Purpose |
|----------|-----------|---------|
| **Android APK Release** | **No** — manual only | Build + publish Android APK |
| Push (OBS) | No — manual only | OBS plugin (needs secrets) |
| Pull Request | Disabled | Was causing failed emails |
| Dispatch | No — manual only | OBS build |

**Nothing runs on ordinary pushes to `main` anymore.**

---

## Archive / remove old releases

GitHub has **no “archive release” button**. To clean up the Releases page you either **delete** old entries or hide failed draft releases.

### Keep (recommended)

| Release | Why |
|---------|-----|
| **v0.9.5-dev** | Current Android APK (when published) |
| **v0.9.4** (or `0.9.4`) | Current Windows OBS zip (when published) |
| **v0.7.1** | Latest Windows release on GitHub today |

### Option A — GitHub website (no tools)

1. Open https://github.com/medicinalsheep/IRLSAFETY/releases  
2. For each **old** release (failed `android-v*` attempts, empty drafts, very old versions you don’t need):  
   - Click the release → **Delete** (trash icon or Edit → Delete this release)  
   - When asked, also delete the **tag** if it was a failed Android CI tag  
3. Leave **v0.9.4** (Windows) and **v0.9.5-dev** (Android) published  

### Option B — Script (fast, keeps allowlist)

```powershell
# One-time: install gh from https://cli.github.com/ then:
gh auth login

# Preview what would be removed:
.\scripts\cleanup-github-releases.ps1 -WhatIf

# Delete everything except v0.9.4 and v0.9.5-dev:
.\scripts\cleanup-github-releases.ps1
```

Custom keep list:

```powershell
.\scripts\cleanup-github-releases.ps1 -KeepTags v0.9.4,v0.9.5-dev
```

### Actions history (failed CI runs)

Failed **workflow runs** are separate from Releases. To reduce noise:

1. **Actions** tab → filter by workflow **Android APK Release**  
2. Old red runs can be left as history; they no longer auto-trigger  

There is no bulk-delete for Actions runs without GitHub support / API.

**Do not** use **Settings → Archive repository** — that archives the entire repo, not individual releases.

---

## Publish a new Android build

1. Bump `versionName` in `android/app/build.gradle.kts` only when needed (stay 0.9.x)  
2. Commit to `main`  
3. Actions → **Android APK Release** → **Run workflow**  
4. Release tag updates: `v0.9.5-dev` (same tag, new APK file)