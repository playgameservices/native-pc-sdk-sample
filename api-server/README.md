# Backend Verification API Server (`api-server`)

This Firebase Cloud Functions (**Node.js 22**) project provides the reference backend verification server for:
- **[`sdk-client/`](../sdk-client/)** (C++ Play Games PC SDK `26.9.261.0+`)
- **[`sdk-client-unity/`](../sdk-client-unity/)** (Unity C# Play Games PC SDK `26.9.261.0+`)
- **[`api-client/`](../api-client/)** (Legacy OAuth2 + REST Migration Reference)

---

## Authentication & Exposed Endpoints (`/api/*`)

All `/api/*` endpoints require a valid **Firebase Auth ID token** in the `Authorization: Bearer <ID_TOKEN>` header (requests without a valid token are rejected with HTTP `401`) and accept **JSON `POST` bodies only** so purchase, integrity, and OAuth tokens are never placed in URL query strings or access logs. CORS is **disabled by default** (`allowedOrigins = false`) and only enabled for origins explicitly listed in `ALLOWED_ORIGINS`.

| Endpoint | Method | Used By | Purpose |
| :--- | :--- | :--- | :--- |
| `/api/verifyIntegrityToken` | `POST` | `sdk-client`, `sdk-client-unity`, `api-client` | Calls `playintegrity.googleapis.com/v1/{packageName}:decodePcIntegrityToken` to verify `MEETS_PC_INTEGRITY`, `appLicensingVerdict`, and the SHA-256 `requestHash`. |
| `/api/verifyPurchaseToken` | `POST` | Backend verification / `api-client` | Calls `androidpublisher.purchases.products.get` to verify purchase state (`PURCHASED` vs `PENDING`) and order metadata on the server. |
| `/api/acknowledgePurchase` | `POST` | Server-authoritative games / `api-client` | Calls `androidpublisher.purchases.products.acknowledge`. |
| `/api/consumePurchase` | `POST` | Server-authoritative games / `api-client` | Calls `androidpublisher.purchases.products.consume`. |
| `/api/getPurchases` | `POST` | `api-client` *(Legacy Migration Reference)* | Lists user purchases via JSON body `userAccessToken` *(requires EAP-enrolled Cloud project on `androidpublisher.googleapis.com`)*. |
| `/api/getSkuDetails` | `POST` | `api-client` *(Legacy Migration Reference)* | Queries SKU metadata & web checkout link via JSON body `userAccessToken` *(requires EAP-enrolled Cloud project on `androidpublisher.googleapis.com`)*. |

---

## Setup & Deployment

1. **Configure Google Cloud, Firebase Auth & Play Console**:
   - Create or select a Firebase / Google Cloud project in the [Firebase Console](https://console.firebase.google.com/) and enable **Firebase Authentication** (so clients can obtain Firebase Auth ID tokens for the `Authorization: Bearer` header).
   - Enable the **Google Play Android Developer API** (`androidpublisher.googleapis.com`) and **Google Play Integrity API** (`playintegrity.googleapis.com`) on the Google Cloud project that owns your runtime Service Account.
   - Invite your Service Account (`...@...gserviceaccount.com`) in **Google Play Console → Users and permissions** with **View app information**, **View financial data**, and **Manage orders and subscriptions** permissions for your game package.
   - **Important (`<CloudProjectNumber>` Alignment)**: Ensure `<CloudProjectNumber>` in `sdk-client` / `sdk-client-unity` is set to the numeric Google Cloud project number of `SA_EMAIL` so `decodePcIntegrityToken` can decrypt integrity tokens minted by the client.

2. **Update `.firebaserc` and `firebase.json`** with your Firebase project ID and Hosting site name.
   - Note: `firebase.json` pins `"runtime": "nodejs22"` (Cloud Functions decommissioned `nodejs20` on 2026-10-30) and uses cross-platform `npm --prefix functions` `predeploy` hooks compatible with Windows `cmd.exe`, macOS, and Linux.

3. **Configure Runtime Environment Variables** *(never commit service account keys to git)*:
   Create a gitignored **`api-server/functions/.env`** file (or use Cloud Secret Manager via `firebase functions:secrets:set`):
   ```ini
   SA_EMAIL=<SERVICE_ACCOUNT_EMAIL>
   SA_KEY=<SERVICE_ACCOUNT_PRIVATE_KEY>
   PLAY_PACKAGE_NAME=com.example.yourgame
   ```
   - `SA_EMAIL`: Google Cloud Service Account email granted Play Console permissions.
   - `SA_KEY`: Service Account private key PEM (with `\n` newlines).
   - `PLAY_PACKAGE_NAME`: Server-enforced single package name (requests targeting any other package name are rejected with HTTP `403`).
   - `ALLOWED_ORIGINS` *(optional)*: Comma-separated list of allowed CORS browser origins (disabled by default when unset).

4. **Install, Lint, Test, Build, and Deploy**:
   ```bash
   cd api-server/functions
   npm install
   npm run lint
   npm test
   npm run build
   cd ..
   firebase deploy --only functions,hosting --force
   ```
