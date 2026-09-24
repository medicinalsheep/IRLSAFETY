# IRLSAFETY+ Pro (SaaS MVP)

Pro is the paid layer on top of the free **Lite** OBS plugin and Android alpha.

Everything still runs **local-only** on your machine. A Pro license unlocks Studio presets, model update checks, and (later) hard-gated Pro features inside the plugin. Payment and license issuance can live on Stripe or Gumroad; the plugin never needs a cloud account to blur frames.

## Pricing sketch

| Plan | Price | What you get |
|------|-------|--------------|
| **Lite** | Free (MIT) | GitHub Releases builds, plates/signs/mail/IDs, local training |
| **Pro monthly** | **$12 / mo** | Studio preset, license key, model update channel, priority notes |
| **Pro yearly** | **$99 / yr** | Same as monthly, billed once per year |

See [PRICING.md](PRICING.md) and [STRIPE_SETUP.md](STRIPE_SETUP.md).

## Lite vs Pro

| | Lite | Pro |
|---|------|-----|
| Windows OBS plugin | Yes | Yes (same binary today) |
| Detection categories | Yes | Yes + Studio preset defaults |
| Local training | Yes | Yes |
| License key | No | Yes (offline-verifiable) |
| Model update manifest | Manual | `check-model-update.ps1` |
| Hard DLL feature gate | N/A | **Phase 2** (not in this MVP) |

## Local-only promise

- Frames and models stay on your PC (or phone).
- License verify is HMAC offline; no phone-home required to censor.
- Optional update checks only fetch a small JSON manifest you choose (URL or file). Downloads never run silently.

## Try Pro today (no plugin rebuild)

1. Generate a signing secret once (see below).
2. Issue yourself a key with `pro/license/issue-license.ps1`.
3. Install it with `scripts/install-pro-license.ps1`.
4. Apply the Studio preset (see `pro/presets/APPLY_STUDIO.txt`).
5. Open the releases page: [docs/pro/index.html](../docs/pro/index.html).

## Generate a secret once

```powershell
# Option A: env var (recommended for CI / issuance machine)
$env:IRLSAFETY_PRO_LICENSE_SECRET = -join ((1..48) | ForEach-Object { '{0:x2}' -f (Get-Random -Max 256) })

# Option B: local file (gitignored)
New-Item -ItemType Directory -Force -Path pro\license | Out-Null
Set-Content -Path pro\license\.secret -Value $env:IRLSAFETY_PRO_LICENSE_SECRET -NoNewline
```

Never commit `.secret` or real Stripe tokens.

## Issue and verify a key

```powershell
.\pro\license\issue-license.ps1 -Seat "you@example.com" -Edition pro -ExpiryUnix 0
.\pro\license\verify-license.ps1 -KeyFile .\pro\license\dev-license.key
.\scripts\verify-pro-features.ps1
```

Expiry `0` means no expiry (dev / lifetime seat).

## Phase 2 (not in this branch)

- Hard lock Pro-only UI / model channel inside `irlsafety-plus.dll`.
- Stripe Checkout + webhook that calls `issue-license.ps1` and emails the key.
- Signed update CDN for Pro models.

Until Phase 2 ships, Pro is **soft-gated**: scripts and docs work; the open Lite plugin still runs without a key.
