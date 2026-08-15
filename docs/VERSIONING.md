# IRLSAFETY+ — Versioning & Naming

**Last updated:** 2026-08-14

---

## Product naming (no domain required)

| Name | Use |
|------|-----|
| **IRLSAFETY+** | Product / app display name |
| **IRLSAFETY** | Repo and core library (`libirlsafety`) |
| **irlsafety-plus** | OBS plugin id, Windows DLL, release zip prefix |
| **com.irlsafety.plus** | Android package + macOS/iOS bundle id placeholder |

You do **not** need a domain to ship alpha builds. `com.irlsafety.plus` follows reverse-DNS convention and can stay until you register a domain (e.g. `com.yourdomain.irlsafety`).

**Website field today:** GitHub repo URL in `buildspec.json` — sufficient for alpha.

---

## Version policy

**Nothing is 1.0.0 until a deliberate public release** (Windows installer, Play Store, App Store).

| Platform | Current line | Format |
|----------|--------------|--------|
| Windows OBS plugin | **0.9.5** | `buildspec.json` → `IRLSAFETY+-v0.9.5-win64.zip` |
| Android app | **0.9.6-dev** | `versionName` in Gradle; promote to `0.9.6` after A53 checklist |
| macOS OBS plugin | — | Planned with **0.9.6** alignment |
| iOS app | **0.9.6-dev** scaffold | I1 CMake/Swift shell; see `docs/DESIGN-ios-v1.md` |

### Next aligned release: **0.9.6**

Bump Windows + Android together when:

1. A53 alpha checklist in `docs/ANDROID_ALPHA_REVIEW.md` is signed off  
2. `irlsafety_v08` ONNX (or current best model) is bundled on both platforms  
3. Optional: macOS OBS smoke + iOS I1 already landable in parallel  

### When to bump

- **Patch** (0.9.4 → 0.9.5): bug fix or small feature on one platform
- **Do not** bump on every internal PR/phase (P10–P16 are dev milestones, not user versions)
- **1.0.0**: first stable, documented release you are willing to support

### Android `versionCode`

Tied loosely to semver: `0.9.6` → `906`. Increment only when `versionName` changes.

---

## Internal dev phases vs user versions

Docs may reference P10–P16 (Android scaffold → tester APK) and iOS I1–I7. Those are **engineering milestones**, not marketing versions. User-facing strings should show `0.9.6-dev`, not "P16" or "I1".

---

## Release naming examples

```
IRLSAFETY+-v0.9.5-win64.zip           Windows
IRLSAFETY+-0.9.6-dev-android.apk      Android sideload
```

iOS (future): `IRLSAFETY+ 0.9.6-dev` TestFlight build number separate from marketing version.

### Package scripts

| Script | Role |
|--------|------|
| `scripts/package-windows.bat` | Build portable Windows zip folder (preferred) |
| `scripts/package-v0.1.bat` | Compatibility wrapper → package-windows.bat |
| `scripts/package-android-apk.bat` | Local Android APK |
| `scripts/zip-release.ps1` | Zip the Windows package folder |

### Android GitHub release

Manual: **Actions → Android APK Release → Run workflow**

Publishes prerelease tag (e.g. `v0.9.6-dev`) with asset `IRLSAFETY+-0.9.6-dev-android.apk`

See `docs/GITHUB.md`
