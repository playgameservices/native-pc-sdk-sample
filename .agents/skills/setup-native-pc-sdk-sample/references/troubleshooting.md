# Troubleshooting Guide — Play Games on PC Native SDK Samples

Symptom-to-fix reference verified against real Windows 10/11 builds, Visual
Studio / `vcpkg`, Firebase Cloud Functions (Node.js 22), and the Google Play
Games on PC runtime.

---

## 1. `sdk-client` Runtime & Build Errors

| Symptom | Root Cause | Verified Fix |
| :--- | :--- | :--- |
| `Initialize Billing SDK` fails with `code 1: Stream removed` (`Unable to determine caller identity due to invalid or missing package name`) | The Google Play Games on PC runtime inspects a file literally named `manifest.xml` next to `GameApplication.exe` (`sdk-client\x64\Debug\manifest.xml`), which still contained placeholder `your-package-id` or was missing. | Create `sdk-client\GameApplication\manifest.local.xml` via `configure_local.mjs` and rebuild `sdk-client\Teapot.sln`. MSBuild's `CopyLocalManifestOverride` target copies it to `$(OutDir)manifest.xml`. |
| Unsigned local Visual Studio build rejected by the Play Games runtime even though `EarlyAccessPartnerGuid` is set in Windows Registry | On 64-bit Windows, the key was written to 32-bit `HKLM\SOFTWARE\WOW6432Node\Google\Play Games Services`, while 64-bit `GooglePlayGamesServices.exe` reads the **64-bit** registry view. | Add the key with `/reg:64`: `reg add "HKLM\Software\Google\Play Games Services" /v EarlyAccessPartnerGuid /t REG_SZ /d "<GUID>" /reg:64 /f`, then restart the `GooglePlayGamesServices-*` Windows service. |
| `error C1083: Cannot open include file: 'GL/glew.h'` or `'play/pc/sdk/...'` | Missing `vcpkg_installed` dependencies or missing Play Games PC SDK archive (`sdk-client/play_pc_sdk/`). | Run `vcpkg install --triplet=x64-windows` inside `sdk-client/` and extract the Play Games PC SDK (`26.9.261.0+`) into `sdk-client/play_pc_sdk/` (`includes/` + `imports/x64/`). |
| `LINK : fatal error LNK1104: cannot open file 'freeglutd.lib'` | `<GL/freeglut_std.h>` auto-links `freeglutd.lib` in `Debug` builds; `vcpkg_installed/x64-windows/debug/lib` must be populated. | Ensure `vcpkg install --triplet=x64-windows` completed (which builds both `debug` and `release` libs) and build with the latest `.vcxproj` files. |
| `Launch Purchase Flow` test order disappears after ~3 minutes | Developer Mode test purchases automatically refund after 3 minutes if left unconsumed/unacknowledged. | Run `Query Purchases` → `Acknowledge Purchase` / `Consume Purchase` promptly after completing the test checkout overlay. |
| `Verify Integrity Token` returns `"result": false` or `400 INVALID_ARGUMENT` / `403 PERMISSION_DENIED` | `<CloudProjectNumber>` in `manifest.local.xml` does not match the Google Cloud project that owns `SA_EMAIL` in `api-server`, or Play Integrity API is not enabled/linked in Play Console. | Confirm `<CloudProjectNumber>` equals `gcloud projects describe <project> --format="value(projectNumber)"` for the Service Account's project, enable `playintegrity.googleapis.com`, and link that project under Play Console → **App integrity**. |

---

## 2. `api-server` Build & Firebase Deployment Errors

| Symptom | Root Cause | Verified Fix |
| :--- | :--- | :--- |
| `firebase deploy` fails on Windows with `ENOENT: ...\$RESOURCE_DIR\package.json` | Older `firebase.json` `predeploy` hooks used Unix-only `"$RESOURCE_DIR"` syntax instead of cross-platform `npm --prefix functions`. | Pull the latest `api-server/firebase.json` (`npm --prefix functions run lint` / `run build`). |
| `firebase deploy` rejects `nodejs20` runtime | Cloud Functions decommissioned the `nodejs20` runtime. | Use Node.js 22+ locally and confirm `api-server/firebase.json` sets `"runtime": "nodejs22"` and `api-server/functions/package.json` sets `"engines": { "node": "22" }`. |
| Cloud Build `npm ci` fails during `firebase deploy` | `package-lock.json` is out of sync with `package.json`, or contains internal corporate npm registry `resolved` URLs unreachable from Cloud Build. | Regenerate `package-lock.json` cleanly via `npm install` against the public npm registry (or strip private proxy `resolved` URLs while keeping `version` and `integrity`). |
| `verifyPurchaseToken` or `verifyIntegrityToken` returns `Unauthorized packageName` | `PLAY_PACKAGE_NAME` in `api-server/functions/.env` restricts requests to one package to prevent cross-package abuse. | Confirm `PLAY_PACKAGE_NAME` in `api-server/functions/.env` exactly matches `<PackageName>` in your client `manifest.local.xml`. |
| `androidpublisher` returns `403 The current user has insufficient permissions` | Service Account has not been granted app permissions in Google Play Console (note: Play Console permission changes can also take up to 24 hours, or trigger immediately after creating/editing a product). | Invite `SA_EMAIL` under Play Console → **Users and permissions** with **View app information**, **View financial data**, and **Manage orders and subscriptions** for the target app. |

---

## 3. Legacy `api-client` OAuth2 & REST Errors

| Symptom | Root Cause | Verified Fix |
| :--- | :--- | :--- |
| `Google Login` browser sign-in returns `redirect_uri_mismatch` | `AppConfig.local.json` `redirect_uri` is not set to `http://127.0.0.1:8080/`, or the OAuth client type is not **Desktop app**. | Create a **Desktop app** OAuth 2.0 client in Google Cloud Console and run `configure_local.mjs --oauth-json ...` (which normalizes `redirect_uri` to `http://127.0.0.1:8080/`). |
| `Get SKUs` returns HTTP `403` after `Google Login` succeeds | Signed into Google OAuth with a **Play Console developer/admin account** instead of a regular license-tester account. | Sign out or use an incognito profile with a **non-developer license-tester Gmail account** added to both Play Console License Testing and the OAuth Consent Screen Test Users list. |
| `Get SKUs` or `Get Purchases` returns `404 Method not found.` | The legacy `/skus` and `/purchases/products` REST endpoints require Early Access Partner visibility on `androidpublisher.googleapis.com` for the calling Service Account's Cloud project. | Use modern `sdk-client` or `sdk-client-unity` (recommended — requires no legacy REST endpoints or Desktop OAuth), or use a Google Cloud project explicitly enrolled for legacy REST visibility. |
| `Verify Integrity Token` in `api-client` says token is empty | `api-client` cannot mint Play Integrity tokens directly; it verifies a token obtained from the Play Games PC SDK. | Run `Prepare Integrity Token` → `Request Integrity Token` in `sdk-client`, copy the generated token, paste it into the `token` row of `api-client`'s `Parameter` table, and click **Execute**. |
