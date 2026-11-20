---
name: setup-native-pc-sdk-sample
description: >-
  Guides developers end-to-end through setting up, configuring, building,
  deploying, and running the Google Play Games on PC Native SDK samples
  (sdk-client C++, sdk-client-unity C#, api-client legacy REST reference, and
  api-server Firebase Cloud Functions) using gitignored .local and .env files
  without committing credentials. Use when a developer clones this repository
  and asks to set up, configure, build, deploy, run, or troubleshoot any sample
  app or backend in playgameservices/native-pc-sdk-sample. Don't use for
  general Android mobile-only Play Billing integration unrelated to this
  repository.
license: Apache-2.0
compatibility: >-
  Windows 10/11 (x64) for C++/Unity client builds and Google Play Games on PC
  runtime testing; Windows, macOS, or Linux with Node.js 22+ for api-server
  deployment. Requires Git, Node.js 22+, Firebase CLI, and (for C++) Visual
  Studio 2022+ with vcpkg and the Google Play Games on PC SDK (26.9.261.0+).
metadata:
  author: Google Play Games DevRel
  version: "1.0.0"
---

# Setup Google Play Games on PC Native SDK Samples

This repository ships **generic placeholders only** (no OAuth client secrets,
no Service Account keys, no Firebase project IDs, and no Play Console package
names). Use this workflow to take a developer from a fresh clone to a running
client and deployed verification backend safely and deterministically.

## Non-Negotiable Safety Guardrails

1. **Never commit credentials or modify tracked template files**:
   - Never write real package names, SKUs, OAuth secrets, or Service Account
     keys into tracked `manifest.xml`, `AppConfig.json`, `config.ts`,
     `firebase.json`, or `.firebaserc`.
   - Always store master JSON keys **outside the git repository** (for example
     `../secrets/`) and write only gitignored override files
     (`manifest.local.xml`, `AppConfig.local.json`, and
     `api-server/functions/.env`).
2. **Never echo secret material**:
   - Never print `private_key`, `SA_KEY`, `client_secret`, `EarlyAccessPartnerGuid`,
     or raw OAuth/Play Integrity tokens to terminal output or chat logs.
3. **Confirm before cloud or system mutations**:
   - Ask for confirmation before running `gcloud` provisioning, `firebase deploy`,
     or Windows Registry updates (`reg add ... /reg:64`), and pause whenever an
     interactive browser sign-in or Play Console UI step is required.

---

## Step 0 — Run Automated Preflight

Run the bundled zero-dependency preflight script from the repository root first
to inspect installed tools, SDK files, Windows Registry Developer Mode state,
and existing gitignored overrides without guessing:

```bash
node .agents/skills/setup-native-pc-sdk-sample/scripts/preflight.mjs
```

Pass `--json` if you need machine-readable output. Report only what is missing
or misaligned before asking the developer for inputs.

---

## Step 1 — Confirm Target Track & Collect Minimal Inputs

Ask the developer which track they want to run, and **only ask for inputs
required by that track**:

| Track | Projects Configured | Required Developer Inputs |
| :--- | :--- | :--- |
| **1. C++ Native PC SDK *(Recommended)*** | `sdk-client` + `api-server` | `PackageName`, one or more active Play Console `SKU`s (comma-separated, e.g. `coins_100,starter_pack`), Firebase `projectId`, numeric Google Cloud `CloudProjectNumber`, and path to an external Service Account JSON key (or offer to create one via `gcloud` in Step 2). |
| **2. Unity Native PC SDK** | `sdk-client-unity` + `api-server` | Same inputs as Track 1 (plus Unity 6000 installed and the Play PC SDK `.unitypackage`). |
| **3. Legacy REST Reference *(Migration Only)*** | `api-client` + `api-server` | All Track 1 inputs **plus** a Desktop OAuth 2.0 Client JSON downloaded from Google Cloud Console (and a non-developer license-tester Google Account). |

Do **not** ask Track 1 or Track 2 developers for an OAuth 2.0 Desktop client —
only legacy `api-client` uses Desktop OAuth 2.0.

---

## Step 2 — Provision Cloud APIs, Service Account & Play Console Links

If the developer already has a Service Account JSON key linked to Play Console
and Firebase Hosting/Functions ready, skip directly to **Step 3**.

### 2A. Optional `gcloud` Automation (Confirm First)

If `gcloud` is installed and authenticated (`gcloud auth list`), offer to run:

```bash
gcloud services enable androidpublisher.googleapis.com playintegrity.googleapis.com \
  --project "<GCP_PROJECT_ID>"

gcloud iam service-accounts create play-pc-sample-backend \
  --display-name "Play Games PC Sample Backend" \
  --project "<GCP_PROJECT_ID>"

gcloud iam service-accounts keys create "<EXTERNAL_SECRETS_DIR>/play-sa-key.json" \
  --iam-account "play-pc-sample-backend@<GCP_PROJECT_ID>.iam.gserviceaccount.com" \
  --project "<GCP_PROJECT_ID>"

# Look up the numeric Cloud Project Number needed for Play Integrity:
gcloud projects describe "<GCP_PROJECT_ID>" --format="value(projectNumber)"
```

> **Important (Play Integrity Project Alignment)**: The numeric
> `<CloudProjectNumber>` configured in the client (`manifest.local.xml` or
> Unity Inspector) **must** match the Google Cloud project that owns the
> Service Account (`SA_EMAIL`) deployed in `api-server`. Otherwise
> `decodePcIntegrityToken` fails because Play encrypts integrity tokens for the
> requested project number.

### 2B. Required Play Console UI Steps (Cannot Be Done via CLI)

Guide the developer to complete these UI steps in **Google Play Console**
(`https://play.google.com/console`):

1. **Grant Service Account API Access**: Go to **Users and permissions** →
   **Invite new users** → enter the Service Account email → under **App
   permissions**, select the test game and grant **View app information**,
   **View financial data**, and **Manage orders and subscriptions**.
2. **Link Play Integrity**: Open the game → **Test and release** (or **Release**)
   → **App integrity** → **Play Integrity API** → link the same Google Cloud
   project (`<GCP_PROJECT_ID>`).
3. **Create Test Products & License Testers**:
   - Under **Monetize with Play → Products → In-app products** (or One-time
     products), confirm at least one active product ID (for example a
     consumable multi-quantity SKU and a non-consumable single-quantity SKU).
   - Under **Settings → License testing**, add a test Gmail account (for legacy
     `api-client`, use a tester account that is **not** a Play Console
     developer/admin account).
4. **Legacy `api-client` Only (Desktop OAuth 2.0 Client)**:
   - In Google Cloud Console → **APIs & Services → Credentials** → **Create
     Credentials → OAuth client ID → Desktop app**, download the JSON to
     `<EXTERNAL_SECRETS_DIR>/oauth-desktop-client.json`.
   - Under **OAuth consent screen → Test users**, add the license-tester Gmail
     account.

---

## Step 3 — Generate Gitignored `.local` & `functions/.env` Files

Never hand-edit tracked template files or echo secrets into shell commands. Run
the bundled deterministic configurator (supports both Google Cloud Console's
raw `{"installed": {...}}` OAuth JSON download and flat `AppConfig.json`):

```bash
node .agents/skills/setup-native-pc-sdk-sample/scripts/configure_local.mjs \
  --target all \
  --package-name "<com.example.yourgame>" \
  --skus "<sku_consumable,sku_nonconsumable>" \
  --cloud-project-number "<NUMERIC_PROJECT_NUMBER>" \
  --backend-url "https://<FIREBASE_PROJECT_ID>.web.app" \
  --sa-json "<EXTERNAL_SECRETS_DIR>/play-sa-key.json" \
  --oauth-json "<EXTERNAL_SECRETS_DIR>/oauth-desktop-client.json"
```

- Omit `--oauth-json` and pass `--target sdk-client,api-server` (or `api-server`)
  when skipping legacy `api-client`.
- Pass `--rtdn-topic "<topic-id>"` if using a custom Cloud Pub/Sub topic for
  `rtdnListener`.
- Pass `--dry-run` first to preview exact target paths without writing files.

The script writes BOM-free UTF-8 files exclusively to gitignored locations
(`api-server/functions/.env`, `sdk-client/GameApplication/manifest.local.xml`,
and `api-client/GameApplication/{,Resource Files/}{manifest.local.xml,AppConfig.local.json}`)
and verifies every written path with `git check-ignore` so `git status` remains
clean.

---

## Step 4 — Build, Deploy & Verify Gates

### 4A. Deploy & Verify `api-server` (Node.js 22 Firebase Functions)

1. Install dependencies, lint, and compile TypeScript:
   ```bash
   cd api-server/functions
   npm install
   npm run lint
   npm run build
   ```
   *(Node.js 22 is required because Cloud Functions decommissioned `nodejs20`.)*
2. Authenticate Firebase CLI (if `firebase projects:list` reports no active
   account, ask the developer to complete `firebase login` in their browser).
3. After developer confirmation, deploy from `api-server/`:
   ```bash
   cd ../
   firebase deploy --only functions,hosting --project "<FIREBASE_PROJECT_ID>"
   ```
4. **Empirical Smoke-Test Gate**:
   ```bash
   curl -s -X POST "https://<FIREBASE_PROJECT_ID>.web.app/api/verifyIntegrityToken" \
     -H "Content-Type: application/json" \
     -d "{\"packageName\":\"<com.example.yourgame>\",\"token\":\"invalid_probe_token\"}"
   ```
   - **Pass**: Returns structured JSON `{"result":false,"error":...}` (proving
     Hosting rewrite → Cloud Function → `functions/.env` → Play Integrity API
     worked).
   - **Fail**: HTTP `404`/`500` or HTML error page → inspect
     `firebase functions:log --project "<FIREBASE_PROJECT_ID>"`.

### 4B. Build & Verify `sdk-client` (C++20 Win32 — Recommended)

1. **Install the Google Play Games on PC SDK (`26.9.261.0+`)**:
   - If `preflight.mjs` reported `sdk-client/play_pc_sdk` missing, guide the
     developer to download the Native PC SDK zip from
     `https://developer.android.com/games/playgames/native-pc/setup` and extract
     it to `sdk-client/play_pc_sdk/` so `sdk-client/play_pc_sdk/includes/` and
     `sdk-client/play_pc_sdk/imports/x64/` exist.
2. **Install C++ dependencies (`vcpkg` manifest mode)**:
   - If `sdk-client/vcpkg_installed` is missing, run from `sdk-client/`:
     ```powershell
     vcpkg install --triplet=x64-windows
     ```
3. **Build (`Debug | x64`)**:
   ```powershell
   & "<PATH_TO_MSBUILD.EXE>" sdk-client\Teapot.sln /t:Build /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
   ```
   - MSBuild's `CopyLocalManifestOverride` target automatically copies
     `sdk-client\GameApplication\manifest.local.xml` →
     `sdk-client\x64\Debug\manifest.xml` (the runtime verifies caller identity
     using a file literally named `manifest.xml` next to `GameApplication.exe`).
   - Verify `sdk-client\x64\Debug\manifest.xml` contains `<PackageName><com.example.yourgame></PackageName>`.
4. **Local Developer Mode 64-Bit Registry Check (Windows)**:
   - If `preflight.mjs` reported `EarlyAccessPartnerGuid` present only in 32-bit
     `WOW6432Node` and missing in the **64-bit** registry view, 64-bit
     `GooglePlayGamesServices.exe` will reject unsigned local IDE builds. After
     confirming with the developer (requires elevated PowerShell), copy the key
     to `/reg:64` and restart the `GooglePlayGamesServices-*` Windows service:
     ```powershell
     reg add "HKLM\Software\Google\Play Games Services" /v EarlyAccessPartnerGuid /t REG_SZ /d "<EAP_GUID>" /reg:64 /f
     Get-Service GooglePlayGamesServices* | Restart-Service -Force
     ```
5. **Run & Verify All 11 Operations**:
   - Launch `sdk-client\x64\Debug\GameApplication.exe`, open **Billing SDK**,
     and verify: `Initialize Billing SDK` (`kOk`) → `Query Product Details` →
     `IsBillingProgramAvailable` → `CreateBillingProgramReportingDetails` →
     `Launch Purchase Flow` → `Query Purchases` → `Acknowledge Purchase` →
     `Consume Purchase` → `Prepare Integrity Token` → `Request Integrity Token`
     → `Verify Integrity Token` (`"result": true`).

### 4C. Setup & Verify `sdk-client-unity` (Unity 6000 C#)

1. Open `sdk-client-unity/` in **Unity 6000** (`6000.0+`).
2. Import the `GooglePlayPcSdk` `.unitypackage` from the downloaded Play Games
   PC SDK archive (`Assets → Import Package → Custom Package...`).
3. Open `Assets/Scenes/SampleScene.unity`, select the GameObject holding
   `GameStoreManager`, and set in the Inspector:
   - `_productIds`: your active Play Console SKU(s)
   - `_cloudProjectNumber`: your numeric Google Cloud Project Number
   - `_backendBaseUrl`: `https://<FIREBASE_PROJECT_ID>.web.app`
4. Build a Windows x64 standalone `.exe` and place `manifest.xml` (matching
   `<Manifest version="1"><Application><PackageName>...</PackageName></Application></Manifest>`)
   next to the built `.exe`. Verify all 11 operations in the same sequence as
   `sdk-client`.

### 4D. Build & Verify `api-client` (Legacy OAuth2 + REST Reference Only)

1. If `api-client/vcpkg_installed` is missing, run `vcpkg install --triplet=x64-windows` inside `api-client/`.
2. Build `api-client\Teapot-Sdk.sln` (`Debug | x64`):
   ```powershell
   & "<PATH_TO_MSBUILD.EXE>" api-client\Teapot-Sdk.sln /t:Build /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
   ```
3. Verify `api-client\x64\Debug\manifest.xml` and `api-client\x64\Debug\AppConfig.json`
   reflect the `.local` overrides (without printing `client_secret`).
4. Launch `api-client\x64\Debug\GameApplication.exe`, open **Billing APIs**, and
   test `0. Google Login` using a **non-developer license-tester Google Account**
   → `1. Get SKUs` → `2. Get Purchases` → `3. Validate Purchase Token` →
   `4. Acknowledge Purchase` → `5. Consume Purchase` → `6. Verify Integrity Token`
   (pasting a fresh Play Integrity token obtained from `sdk-client`).

---

## Troubleshooting

When any build, deployment, registry, OAuth, Play Billing, or Play Integrity
step fails, read [references/troubleshooting.md](references/troubleshooting.md)
for exact error-to-fix mappings before improvising workarounds.
