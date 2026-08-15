# GitHub — Releases & CI (simplified)

**Last updated:** 2026-08-14

---

## Download Android APK

1. Open **Actions** → **Android APK Release** → **Run workflow** → Run  
2. Wait ~10–15 minutes for green checkmark  
3. Open **Releases**: https://github.com/medicinalsheep/IRLSAFETY/releases  
4. Open **IRLSAFETY+ 0.9.6-dev (Android)** prerelease (or latest Android prerelease)  
5. Download **`IRLSAFETY+-0.9.6-dev-android.apk`** under Assets  

If Releases is still empty, download the same file from the workflow run’s **Artifacts** section.

Install: **android/TESTER.md**

---

## Download Windows OBS plugin

Latest Windows package is a separate release (e.g. **v0.9.5**).  
Built locally:

```bat
scripts\build-windows.bat
scripts\package-windows.bat
scripts\zip-release.ps1
```

---

## CI workflows (what runs automatically)

| Workflow | Auto-run? | Purpose |
|----------|-----------|---------|
| **Android APK Release** | **No** — manual only | Build + publish Android APK |
| Push (OBS) | No — manual only | OBS plugin (needs secrets) |
| Pull Request | Disabled | Avoids unused PR CI noise |
| Dispatch | No — manual only | OBS build |

**Nothing runs on ordinary pushes to `main` anymore.**

---

## Archive / remove old releases

GitHub has **no “archive release” button**. To clean up the Releases page you either **delete** old entries or hide failed draft releases.

### Keep (recommended)

| Release | Why |
|---------|-----|
| **v0.9.6-dev** | Current Android APK |
| **v0.9.5** | Current Windows OBS zip |
| **v0.7.1** | Optional archive baseline |

### Option A — GitHub website (no tools)

1. Open https://github.com/medicinalsheep/IRLSAFETY/releases  
2. For each **old** release you don’t need: Delete (and tag if it was a failed CI tag)  
3. Leave current Windows + Android tags published  

### Option B — Script (fast, keeps allowlist)

```powershell
gh auth login
.\scripts\cleanup-github-releases.ps1 -WhatIf
.\scripts\cleanup-github-releases.ps1
```

Custom keep list:

```powershell
.\scripts\cleanup-github-releases.ps1 -KeepTags v0.9.5,v0.9.6-dev,v0.7.1
```

---

## Publish a new Android build

1. Bump `versionName` in `android/app/build.gradle.kts` only when needed (stay 0.9.x)  
2. Commit to `main`  
3. Actions → **Android APK Release** → **Run workflow**  
4. Release tag updates to `v` + `versionName` (e.g. `v0.9.6-dev`)
