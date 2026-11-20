# Google Play Games on PC — Native SDK Samples

Official sample repository demonstrating how to integrate the **[Google Play Games on PC SDK (`26.9.261.0+`)](https://developer.android.com/games/playgames/native-pc/)** in native Windows C++ and Unity C# games, verify tokens on a trusted backend, and migrate from legacy REST APIs.

---

## Repository Structure

| Directory | Stack | Purpose |
| :--- | :--- | :--- |
| **[`sdk-client/`](sdk-client/)** | C++20 / Win32 / OpenGL | **Recommended for C/C++ games.** Demonstrates all core Play Games PC SDK Billing (`Initialize`, `QueryProductDetails`, `IsBillingProgramAvailable`, `CreateBillingProgramReportingDetails`, `LaunchPurchaseFlow`, `QueryPurchases`, `AcknowledgePurchase`, `ConsumePurchase`) and Play Integrity (`Prepare`, `Request`, `Verify`) APIs. |
| **[`sdk-client-unity/`](sdk-client-unity/)** | Unity 6000 / C# (`PlayPcSdkManaged`) | **Recommended for Unity games.** Demonstrates the exact same 11-step Play Billing + Purchase Restoration + Play Integrity workflow using `PlayPcSdkFactory` and async `Result` APIs. |
| **[`api-server/`](api-server/)** | Node.js 22 / TypeScript / Firebase Cloud Functions | **Backend verification reference.** Verifies Play Integrity tokens (`decodePcIntegrityToken`) and Play Billing purchase tokens (`androidpublisher` v3) using a Google Cloud Service Account. |
| **[`api-client/`](api-client/)** | C++/CLI WinForms (OAuth2 + REST) | **Legacy Migration Reference Only.** Preserved solely to show developers migrating from legacy OAuth2 + REST (`Billing APIs`) how each step maps to `sdk-client/` (`Billing SDK`). **Do not use for new integrations.** |
| **[`.agents/skills/setup-native-pc-sdk-sample/`](.agents/skills/setup-native-pc-sdk-sample/SKILL.md)** | Agent Skill (`agentskills.io`) + Node.js | **Automated developer onboarding skill.** Auto-discovered by Gemini CLI, Google Antigravity, Claude Code, Codex, Cursor, and GitHub Copilot to run preflight checks, generate gitignored `.local` / `.env` overrides safely, build C++ solutions, deploy `api-server`, and troubleshoot setup errors. |
| **[`docs/`](docs/)** | Markdown | System architecture ([`docs/DESIGN.md`](docs/DESIGN.md)) and changelist ([`docs/CHANGELIST.md`](docs/CHANGELIST.md)). |

---

## Core Functionalities Demonstrated (`sdk-client` & `sdk-client-unity`)

Both modern SDK samples implement the unified 11-operation sequence targeting **Play Games PC SDK `26.9.261.0+`**:

1. **Initialize Billing SDK** (`GooglePlayInitialize` + `BillingClient` + `IntegrityClient`, handling `kActionRequiredShutdownClientProcess` and `26.9.261.0+` runtime status codes)
2. **Query Product Details** (`QueryProductDetails`, supporting multi-SKU dropdown selection and `full_price_micros` strike-through discount pricing)
3. **IsBillingProgramAvailable** (`IsBillingProgramAvailable` for Alternative Billing Only & User Choice Billing)
4. **CreateBillingProgramReportingDetails** (`CreateBillingProgramReportingDetails` returning `external_transaction_token`)
5. **Launch Purchase Flow** (`LaunchPurchaseFlow` with non-blocking UI and `PURCHASED` vs `PENDING` state handling)
6. **Query Purchases** (`QueryPurchases` for startup/foreground purchase restoration)
7. **Acknowledge Purchase** (`AcknowledgePurchase`)
8. **Consume Purchase** (`ConsumePurchase`)
9. **Prepare Integrity Token** (`PrepareIntegrityToken`)
10. **Request Integrity Token** (`RequestIntegrityToken` with cryptographic SHA-256 `request_hash`)
11. **Verify Integrity Token** (`POST /api/verifyIntegrityToken` via `api-server`)

---

## Local Configuration Overrides (Zero Secret Commits)

All projects support **gitignored `.local` / `.env` configuration overrides** so you can build and test against your real Play Console package, SKUs, OAuth credentials, and Service Account keys without modifying tracked template files:

- **`sdk-client/`**: Create `sdk-client/GameApplication/manifest.local.xml` (copied automatically to `$(OutDir)manifest.xml` on build).
- **`api-client/`**: Create `api-client/GameApplication/Resource Files/manifest.local.xml` and `AppConfig.local.json` (copied automatically to `$(OutDir)manifest.xml` and `$(OutDir)AppConfig.json` on build).
- **`api-server/`**: Create `api-server/functions/.env` with `SA_EMAIL`, `SA_KEY`, and `PLAY_PACKAGE_NAME` (or use Cloud Secret Manager).

---

## Quick Start

### Option A: Automated Setup with an AI Coding Agent *(Recommended)*

Because this repository ships generic placeholders rather than credentials, it bundles an open-standard **Agent Skill** at [`.agents/skills/setup-native-pc-sdk-sample/SKILL.md`](.agents/skills/setup-native-pc-sdk-sample/SKILL.md) (compatible with Gemini CLI, Google Antigravity, Claude Code, Codex, Cursor, and GitHub Copilot).

Open this repository in your AI coding agent and ask:

> *"Help me set up and run `sdk-client` and `api-server` with my Google Play Console app and Firebase project."*

Your agent will run [`scripts/preflight.mjs`](.agents/skills/setup-native-pc-sdk-sample/scripts/preflight.mjs) to check your toolchains and 64-bit Windows Registry state, walk you through GCP / Play Console wiring, generate gitignored `.local` and `functions/.env` files via [`scripts/configure_local.mjs`](.agents/skills/setup-native-pc-sdk-sample/scripts/configure_local.mjs) without touching tracked templates, and verify the build and backend deployment.

### Option B: Manual Setup Guides

- **C++ Native PC SDK Sample**: Follow [`sdk-client/README.md`](sdk-client/README.md).
- **Unity PC SDK Sample**: Follow [`sdk-client-unity/README.md`](sdk-client-unity/README.md).
- **Backend Verification Server**: Follow [`api-server/README.md`](api-server/README.md).
- **Migrating from `api-client` to `sdk-client`**: Read [`api-client/README.md`](api-client/README.md) and [`docs/DESIGN.md`](docs/DESIGN.md).
