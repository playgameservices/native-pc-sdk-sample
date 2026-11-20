# Backend Verification API Server (`api-server`)

This Firebase Cloud Functions (**Node.js 22**) project provides the reference backend verification server for:
- **[`sdk-client/`](../sdk-client/)** (C++ Play Games PC SDK `26.9.261.0+`)
- **[`sdk-client-unity/`](../sdk-client-unity/)** (Unity C# Play Games PC SDK `26.9.261.0+`)
- **[`api-client/`](../api-client/)** (Legacy OAuth2 + REST Migration Reference)

---

## Exposed Endpoints (`/api/*`)

| Endpoint | Method | Used By | Purpose |
| :--- | :--- | :--- | :--- |
| `/api/verifyIntegrityToken` | `POST` | `sdk-client`, `sdk-client-unity`, `api-client` | Calls `playintegrity.googleapis.com/v1/{packageName}:decodePcIntegrityToken` to verify `MEETS_PC_INTEGRITY`, `appLicensingVerdict`, and the SHA-256 `requestHash`. |
| `/api/verifyPurchaseToken` | `GET` / `POST` | Backend verification / `api-client` | Calls `androidpublisher.purchases.products.get` to verify purchase state (`PURCHASED` vs `PENDING`) and order metadata on the server. |
| `/api/acknowledgePurchase` | `POST` | Server-authoritative games / `api-client` | Calls `androidpublisher.purchases.products.acknowledge`. |
| `/api/consumePurchase` | `POST` | Server-authoritative games / `api-client` | Calls `androidpublisher.purchases.products.consume`. |
| `/api/getPurchases` | `GET` | `api-client` *(Legacy Migration Reference)* | Lists user purchases via `userAccessToken` *(requires EAP-enrolled Cloud project on `androidpublisher.googleapis.com`)*. |
| `/api/getSkuDetails` | `GET` | `api-client` *(Legacy Migration Reference)* | Queries SKU metadata & web checkout link via `userAccessToken` *(requires EAP-enrolled Cloud project on `androidpublisher.googleapis.com`)*. |

---

## Setup & Deployment

1. **Configure Google Cloud & Play Console**:
   - Create or select a Firebase / Google Cloud project in the [Firebase Console](https://console.firebase.google.com/).
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
   - `PLAY_PACKAGE_NAME`: Enforces single-package isolation so callers cannot query arbitrary package names with your Service Account.
   - `ALLOWED_ORIGINS` *(optional)*: Comma-separated list of allowed CORS origins.

4. **Install, Build, and Deploy**:
   ```bash
   cd api-server/functions
   npm install
   npm run lint
   npm run build
   cd ..
   firebase deploy --only functions,hosting --force
   ```
