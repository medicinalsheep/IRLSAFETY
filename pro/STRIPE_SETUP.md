# Stripe / Gumroad setup checklist (no secrets in git)

Wire payment later. This MVP only needs license scripts and docs.

## Before you take money

1. Create a Stripe account (or Gumroad) under your business entity.
2. Create two products:
   - `IRLSAFETY+ Pro Monthly` at $12 / month (subscription)
   - `IRLSAFETY+ Pro Yearly` at $99 / year (subscription or one-time)
3. Enable Customer Portal for cancel / update card.
4. Pick a success URL (thank-you page) and a cancel URL.
5. Decide seat identity: customer email is fine for v1.

## License issuance flow

1. Customer pays.
2. Webhook (Stripe `checkout.session.completed` or Gumroad ping) runs on a private machine or server you control.
3. That job calls:

```powershell
$env:IRLSAFETY_PRO_LICENSE_SECRET = "<from secret store, not git>"
.\pro\license\issue-license.ps1 -Seat $CustomerEmail -Edition pro -ExpiryUnix $Expiry -OutFile "issued\$CustomerEmail.key"
```

4. Email the key file contents (or a download link from storage you own).
5. Customer runs `scripts\install-pro-license.ps1` and pastes / drops `license.key`.

## Secret handling

- Store `IRLSAFETY_PRO_LICENSE_SECRET` in Windows Credential Manager, 1Password, or Stripe-adjacent secrets. **Never** commit it.
- Rotate by issuing new keys with a new secret; old keys stop verifying (plan a grace period if you rotate).
- Keep a separate **dev** secret in `pro/license/.secret` (gitignored) for local tests only.

## Gumroad alternative

- Create a product with license key delivery enabled, **or**
- On sale ping, run the same `issue-license.ps1` and attach the output.

## Do not put in this repo

- Stripe secret keys (`sk_live_…`)
- Webhook signing secrets
- Customer emails or paid key dumps
- `.env` files with live tokens

## Smoke test before launch

1. Issue a key with the production secret on a locked-down PC.
2. `verify-license.ps1` and `scripts\verify-pro-features.ps1` exit 0.
3. Install to `%ProgramData%\IRLSAFETY+\license.key`.
4. Confirm thank-you page links to [docs/pro/index.html](../docs/pro/index.html) "I have a key" steps.
