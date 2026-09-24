# IRLSAFETY+ pricing

Simple tiers. Adjust numbers before going live; this is the MVP sketch.

## Lite (free)

- Price: **$0**
- Source: GitHub Releases (MIT)
- Includes: Windows OBS plugin, Android alpha when published, local detection, local training hub
- Support: community / issues

## Pro monthly

- Price: **$12 / month**
- Includes: everything in Lite, plus
  - Offline-verifiable Pro license key
  - Studio preset (plates, mail labels, IDs, street signs on; Screen Text off)
  - Model update manifest checks (`pro/scripts/check-model-update.ps1`)
  - Installer helpers for `license.key`
- Seat: one machine / one seat email unless you issue more keys

## Pro yearly

- Price: **$99 / year** (about two months free vs monthly)
- Same features as Pro monthly
- License expiry set to purchase date + 365 days (or `0` for lifetime if you choose)

## What is not charged

- Frame processing (always local)
- Training on your own captures
- Reading these docs

## Payment rails (later)

Stripe Checkout or Gumroad. After payment, issue a key and email it. Checklist: [STRIPE_SETUP.md](STRIPE_SETUP.md).
