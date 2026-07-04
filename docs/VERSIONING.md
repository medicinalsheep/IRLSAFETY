# IRLSAFETY+ — Versioning & Naming

**Last updated:** 2026-07-03

---

## Product naming (no domain required)

| Name | Use |
|------|-----|
| **IRLSAFETY+** | Product / app display name |
| **IRLSAFETY** | Repo and core library (`libirlsafety`) |
| **irlsafety-plus** | OBS plugin id, Windows DLL, release zip prefix |
| **com.irlsafety.plus** | Android package + macOS bundle id placeholder |

You do **not** need a domain to ship alpha builds. `com.irlsafety.plus` follows reverse-DNS convention and can stay until you register a domain (e.g. `com.yourdomain.irlsafety`).

**Website field today:** GitHub repo URL in `buildspec.json` — sufficient for alpha.

---

## Version policy

**Nothing is 1.0.0 until a deliberate public release** (Windows installer, Play Store, App Store).

| Platform | Current line | Format |
|----------|--------------|--------|
| Windows OBS plugin | **0.9.4** | `buildspec.json` → `IRLSAFETY+-v0.9.4-win64.zip` |
| Android app | **0.9.5-dev** | `versionName` in Gradle; bump only on meaningful milestones |
| iOS (planned) | **0.9.x-dev** | Not started — same pre-1.0 line |

### When to bump

- **Patch** (0.9.4 → 0.9.5): bugfix or small feature on one platform
- **Do not** bump on every internal PR/phase (P10–P15 are dev milestones, not user versions)
- **1.0.0**: first stable, documented release you are willing to support

### Android `versionCode`

Tied loosely to semver: `0.9.5` → `905`. Increment only when `versionName` changes.

---

## Internal dev phases vs user versions

Docs may reference P10–P15 (Android scaffold → tester APK). Those are **engineering milestones**, not marketing versions. User-facing strings should show `0.9.5-dev`, not "P14" or "alpha5".

---

## Release naming examples

```
IRLSAFETY+-v0.9.4-win64.zip          Windows
irlsafety-plus-0.9.5-dev-android.apk  Android sideload
```

iOS (future): `IRLSAFETY+ 0.9.5-dev` TestFlight build number separate from marketing version.

### Android GitHub release

Manual: **Actions → Android APK Release → Run workflow**

Publishes prerelease tag `v0.9.5-dev` with asset `IRLSAFETY+-0.9.5-dev-android.apk`

See `docs/GITHUB.md`