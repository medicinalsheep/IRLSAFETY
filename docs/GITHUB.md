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

## Clean up old failed runs

Failed runs stay in the Actions history but **won’t repeat** if workflows are manual-only.  
You can filter Actions by workflow name **Android APK Release** to see only mobile builds.

---

## Publish a new Android build

1. Bump `versionName` in `android/app/build.gradle.kts` only when needed (stay 0.9.x)  
2. Commit to `main`  
3. Actions → **Android APK Release** → **Run workflow**  
4. Release tag updates: `v0.9.5-dev` (same tag, new APK file)