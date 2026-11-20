# Changelist (`CHANGELIST.md`)

All notable requirement, architectural, and implementation changes to `native-pc-sdk-sample` are tracked in reverse-chronological order in this file.

---

## [001] - 2026-11-17: Native PC SDK Sample Security Remediation, Cleanup & SDK `26.9.261.0+` Modernization

### Revision 10 (2026-11-19 — Bundled Open-Standard Developer Setup Agent Skill)
- **Developer Onboarding Agent Skill (`.agents/skills/setup-native-pc-sdk-sample/`)**:
  - Added `SKILL.md` following the `agentskills.io` open standard (auto-discovered by Gemini CLI, Google Antigravity, Claude Code, Codex, Cursor, and GitHub Copilot) to guide developers through setting up, configuring, building, deploying, and troubleshooting `sdk-client`, `sdk-client-unity`, `api-client`, and `api-server` with zero secret commits.
  - Added zero-dependency Node.js helper scripts `scripts/preflight.mjs` (detects toolchains, Play PC SDK, `vcpkg_installed`, gitignored overrides, and Windows 64-bit `EarlyAccessPartnerGuid` registry state without echoing secrets) and `scripts/configure_local.mjs` (converts external Service Account JSON and Google Cloud Console `{"installed": {...}}` or flat Desktop OAuth JSON into gitignored BOM-free UTF-8 `.local` and `functions/.env` files after verifying `git check-ignore`).
  - Added `references/troubleshooting.md` and updated root `README.md` and `docs/DESIGN.md` with quick-start and architecture details.

### Revision 9 (2026-11-19 — Pre-Public Release Sanitization, Asset Cleanup & Placeholder Standardization)
- **Public Documentation & Asset Sanitization (`docs/`)**:
  - Removed unreferenced legacy screenshots (`docs/screenshots/api-client/02.png`–`17.png` and `docs/screenshots/sdk-client/02.png`–`08.png`) that contained captured test tokens and outdated UI captures, retaining only the clean README-referenced captures (`00.png` and `01.png` per client).
  - Replaced non-public IPC implementation details and shorthand in `docs/DESIGN.md` with public-safe developer guidance.
- **Placeholder Standardization (`sdk-client/`, `sdk-client-unity/`, `api-client/`, `api-server/`)**:
  - Standardized all remaining tracked sample defaults (`manifest.xml`, `GameAppGenericClasses.hpp`, `GameStoreManager.cs`, `GameApis.json`, `AppSettingsFiles.cpp`, `firebase.json`, `.firebaserc`, and `config.ts`) on generic placeholders (`com.example.yourgame`, `your-iap-product-id`, `https://your-project.web.app`, `your-firebase-project-id`, `your-rtdn-pubsub-topic`).
  - Removed tracked local Unity editor layout state (`sdk-client-unity/UserSettings/`), added `/UserSettings` to `sdk-client-unity/.gitignore`, and removed stale internal TODO comments in `billing.ts`, `JNIHelper.cpp`, and `Shader.cpp`.

### Revision 8 (2026-11-19 — End-to-End Empirical Verification & Comprehensive Documentation Update)
- **Documentation & Architecture Alignment (`README.md`, `sdk-client/README.md`, `sdk-client-unity/README.md`, `api-client/README.md`, `api-server/README.md`, `docs/DESIGN.md`)**:
  - Updated `docs/DESIGN.md` to reflect the empirically verified end-to-end architecture across all four sub-projects, including the Node.js 22 (`nodejs22`) Cloud Functions backend layout, lazy Service Account JWT initialization (`getPlayClients()`), `std::shared_ptr` RAII client lifetimes, MSBuild `Debug`/`Release` `vcpkg` linkage rules, and gitignored `.local` / `functions/.env` override pipelines (`CopyLocalManifestOverride` & `CopyLocalConfigOverrides`).
  - Updated root and sub-project `README.md` guides with verified operational requirements: comma-separated multi-SKU `<SKU>` dropdown configuration, 64-bit Windows Registry `HKLM\Software\Google\Play Games Services\EarlyAccessPartnerGuid` (`/reg:64`) local Developer Mode verification, `<CloudProjectNumber>` alignment between client Play Integrity requests and `api-server` Service Account decoding, and Early Access Partner (`androidpublisher.googleapis.com`) + non-developer license-tester requirements on legacy `api-client` endpoints.

### Revision 7 (2026-11-19 — Upgrade `api-server` Cloud Functions Runtime from Decommissioned `nodejs20` to `nodejs22`)
- **`api-server/`**: Updated `engines.node` from `"20"` to `"22"` in `api-server/functions/package.json`, pinned `"runtime": "nodejs22"` in `api-server/firebase.json`, synchronized `api-server/functions/package-lock.json` with `package.json` for Cloud Build `npm ci`, and removed the duplicate `mainHandler` alias in `api-server/functions/src/index.ts` so only `main` and `rtdnListener` are exported.

### Revision 6 (2026-11-18 — Local Config Override Support, Multi-SKU Dropdown, Lazy Backend JWT Init & Runtime Fixes)
- **`.gitignore` & Build Output Overrides (`sdk-client/` & `api-client/`)**:
  - Added `**/manifest.local.xml` and `**/AppConfig.local.json` to `.gitignore` and added MSBuild `AfterTargets="CopyFilesToOutputDirectory"` targets (`CopyLocalManifestOverride` / `CopyLocalConfigOverrides`) in both `GameApplication.vcxproj` files so gitignored `.local` config files automatically populate `$(OutDir)manifest.xml` (required by `GooglePlayGamesServices.exe` caller-identity verification) and `$(OutDir)AppConfig.json` without dirtying tracked placeholder files.
- **`sdk-client/`**:
  - Updated `ResolveManifestPath()` and `ReadDataFromManifestFile()` (`GameAppGenericClasses.hpp`) to check `manifest.local.xml` candidates first and split comma-separated `<SKU>` lists into `manifest.App.skuList`.
  - Updated `OnSelectedQueryProductDetails()` (`GameAppBillingSDKDialog.hpp`) to populate all configured SKUs in `manifest.App.skuList` into the SKU selector combo box.
- **`api-client/`**:
  - Added `RegenerateOAuthSecurityParams()`, `.local` file candidate lookup, and `<BackendBaseUrl>` URL override support to `AppSettingsFiles` (`AppSettingsFiles.h` / `AppSettingsFiles.cpp`), and updated `AccessToken()` (`GameHelper.cpp`) to mint fresh `state` + PKCE `code_verifier`/`code_challenge` values on every login attempt.
  - Updated `GetPurchasesDetails()`, `GetValidatePurchase()`, `GetAcknowledgePurchase()`, `PostConsumePurchase()`, and `PostVerifyIntegrityToken()` (`GameHelper.cpp`) to sync `productId` from fetched purchases, honor manual parameter-table `token`/`productId` edits, avoid prefilling `VerifyIntegrityToken` with the OAuth2 access token, guard query-string stripping, and surface HTTP error bodies in `txtResult`.
- **`api-server/`**:
  - Refactored `playApiClient.ts` to lazily initialize `google.auth.JWT` and `google.androidpublisher` via `getPlayClients()` so `process.env.SA_EMAIL`, `process.env.SA_KEY`, and `process.env.PLAY_PACKAGE_NAME` from `functions/.env` or Cloud Functions runtime secrets are resolved cleanly at request time, replaced internal test identifiers in `config.ts` / `index.ts` with generic placeholders, and replaced Unix-only `"$RESOURCE_DIR"` in `firebase.json` `predeploy` hooks with cross-platform `npm --prefix functions` so `firebase deploy` works on Windows `cmd.exe` as well as Linux/macOS.

### Revision 5 (2026-11-18 — Debug `vcpkg` `LibraryPath`, Linker & Runtime DLL Copy Fixes)
- **`api-client/`**: Added `..\vcpkg_installed\<arch>-windows\debug\lib` and `debug\bin` to `Debug` `LibraryPath`, switched `Debug` `AdditionalDependencies` to `freeglutd.lib;glew32d.lib;OpenGL32.lib` (resolving `LNK1104: cannot open file 'freeglutd.lib'` triggered by `<GL/freeglut_std.h>`'s `#pragma comment`), added conditional `Debug`/`Release` DLL `<CopyFileToFolders>` rules for `freeglutd.dll` and `glew32d.dll`, and enabled MSBuild `vcpkg` manifest properties (`$(SolutionDir)vcpkg_installed`).
- **`sdk-client/`**: Enabled MSBuild `vcpkg` manifest properties across `GameEngine.vcxproj` and `GameApplication.vcxproj`, added a `ResolveVcpkgDebugLibs` `BeforeTargets="Link"` target to automatically pick `restclient-cppd.lib` vs `restclient-cpp.lib` and `tinyxml2d.lib` vs `tinyxml2.lib` in `Debug`, and scoped runtime DLL `<CopyFileToFolders>` rules strictly by `Debug` vs `Release` (including `tinyxml2*.dll` and `restclient-cpp*.dll`).

### Revision 4 (2026-11-18 — `api-client` UTF-16LE Resource Script Fix)
- Removed stale `IDR_JSON3 RCDATA "Resource Files\\userToken.json"` entry from UTF-16LE `api-client/GameApplication/GameApplication.rc` (along with `#define IDR_JSON3` in `resource.h` and unused `hServiceJson` lookup in `GameHelper.cpp`), resolving `error RC2135: file not found: Resource Files\userToken.json`.

### Revision 3 (2026-11-18 — `api-client` MSVC Build Fix & Zero-Warning Cleanup)
- **`api-client/` & `sdk-client/` Build & Warning Fixes**:
  - Added missing `void PostVerifyIntegrityToken();` declaration to `public ref class GameApplication` in `api-client/GameApplication/GameApplication.h`, resolving `C3861`/`C2039` and cascading member-scope errors in `GameHelper.cpp`.
  - Resolved `C4715` (`not all control paths return a value`) in `HelperClasses.cpp` (`GetLocalIPAddress`, `ParseQueryParameters`) and `AppSettingsFiles.cpp` (`GetLocalIPAddress`, `ReadFile`).
  - Resolved `C4677` C++/CLI assembly-visibility warnings by marking `GameApis`, `Config`, `Response`, `SharedData` (`HelperClasses.h`), `AppSettingsFiles` (`AppSettingsFiles.h`), and `HttpListenerThread` (`DevServer.h`) as `public ref class`.
  - Replaced unconditional early `return;` in `TeapotRenderer::Unload()` (`TeapotRenderer.cpp` / `TeapotRenderer.h` across both `api-client` and `sdk-client`) with a `wglGetCurrentContext() == nullptr` guard and zero-initialized OpenGL buffer/program fields, resolving `C4702` unreachable-code warnings.

### Revision 2 (2026-11-17 — Full Implementation Completed Across All 4 Projects)
- **`sdk-client/` (C++20 Win32 Play Games PC SDK `26.9.261.0+`)**:
  - Removed dead legacy OAuth2/REST files (`GameAppBaseWin.hpp`, `AppConfig.json`, `LegacyGameApis.json`, `userToken.json`, `index.htm`) and dead Android NDK / Gradle files (`android_native_app_glue.*`, `src/`, `build.gradle`, `gradlew*`, `proguard-rules.pro`, `#ifdef __ANDROID__` in `GameApplication.cpp`).
  - Stripped unused `boost-asio`, `boost-beast`, `boost-json`, and `google-cloud-cpp` from `vcpkg.json`; standardized all `GameApplication.vcxproj` and `GameEngine.vcxproj` configurations (`Debug`/`Release`, `Win32`/`x64`) on `..\play_pc_sdk\includes` and `..\play_pc_sdk\imports\<arch>` (`bcrypt.lib` added).
  - Fixed all 7 core Billing SDK methods + added `Query Purchases` + hardened Play Integrity (`Prepare`, `Request`, `Verify`) in `GameAppBillingSDKDialog.hpp`:
    1. Fixed `Initialize Billing SDK` error handling (`switch (result.code())` instead of `bool initReturn = result.ok()`), added `GooglePlayInitializeParams{.package_name}`, handled all 5 `InitializationError` codes (including mandatory clean exit on `kActionRequiredShutdownClientProcess` and new `kSdkRuntimeUnavailable` / `kSdkRuntimeNotInstalled`), and logged `environment` & `form_factor`.
    2. Fixed `Query Product Details` SKU logging and added `ProductOffer::full_price_micros` display.
    3. Removed invalid `3 - kBillingProgramExternalOffers` from `IsBillingProgramAvailable` and `CreateBillingProgramReportingDetails`, and surfaced `external_transaction_token` in `CreateBillingProgramReportingDetails`.
    4. Fixed `Launch Purchase Flow` out-of-bounds `product_offers[0]` crash, distinguished `PURCHASED` vs `PENDING` states, and ran continuations on a non-blocking worker thread posting `WM_APP_SDK_RESULT` back to the Win32 UI thread.
    5. Added `Query Purchases` (`BillingClient::QueryPurchases`) feeding restored purchase tokens directly into `Acknowledge Purchase` and `Consume Purchase`, and fixed copy-pasted acknowledgement log messages.
    6. Replaced predictable `std::time(nullptr)` Play Integrity `request_hash` with Windows CNG (`BCryptHash` + `BCryptGenRandom`) SHA-256 hashing and replaced all raw `new`/`delete` pointers with RAII (`std::shared_ptr`, `std::optional`, stack dialog allocation) and null-safe `tinyxml2` XML parsing.
- **`sdk-client-unity/` (Unity 6000 C# `PlayPcSdkManaged`)**:
  - Updated `Assets/Scripts/GameStoreManager.cs` (leaving `Assets/Scenes/SampleScene.unity` 100% untouched) to implement all 11 operations in the exact `sdk-client` sequence (`InitializeSDK`, `QueryProducts`, `CheckBillingProgramAvailable`, `CreateBillingProgramReportingDetails`, `PurchaseItem`, `QueryPurchases`, `AcknowledgePurchase`, `ConsumePurchase`, `PrepareIntegrityToken`, `RequestIntegrityToken` with `SHA256` + CSPRNG nonce, and `VerifyIntegrityToken` via `UnityWebRequest`), dynamically cloning and laying out unassigned UI buttons in `Start()`.
  - Updated `sdk-client-unity/README.md` (including fixing PascalCase `<Manifest version="1"><Application><PackageName>` XML syntax).
- **`api-client/` (Legacy REST Migration Reference)**:
  - Retained `api-client/` with `"Billing APIs"` as the dedicated migration reference, stripped dead Android NDK / Gradle files, removed `cmd.exe /C wmic` shell execution and the `free(hold)` stack-pointer corruption bug in `CGLControl.cpp`, enforced RFC 7636 `S256` PKCE + strict `state` CSRF validation on `127.0.0.1` loopback, removed open-proxy endpoints in `DevServer.cpp`, fixed `PostVerifyIntegrityToken()` JSON body serialization, and added a side-by-side migration table in `api-client/README.md`.
- **`api-server/` (Firebase Cloud Functions Backend)**:
  - Removed unused ChromeOS TWA/PWA frontend (`src/`, `images/`, `snowpack*`, `skusValue.ts`, `usersdb.ts`, `tokensdb.ts`) and vulnerable `axios` `0.21.4` dependency.
  - Replaced raw URL template interpolation (`purchases.ts` / `skuDetailsApi.ts`) with `functions/src/playApiClient.ts` using parameterized `googleapis` `androidpublisher_v3` methods, `URL` + `encodeURIComponent()` + `URLSearchParams`, and strict regex validation while preserving the existing token-based request contract.

### Revision 1 (2026-11-16 — Pre-Implementation Requirement & Design Specification)
- **Requirements & Architecture Documentation (`docs/`)**:
  - Defined all **P0** security fixes, crash fixes, and Play PC SDK `26.9.261.0+` functional updates alongside **P1/P2** modernization and de-bloating tasks across all four sub-projects (`sdk-client`, `sdk-client-unity`, `api-client`, `api-server`).
  - Authored `docs/DESIGN.md` as the living overall system design document covering the end-to-end IPC + backend verification architecture, the 1:1 migration mapping table from `api-client` (Legacy OAuth2 + REST) to `sdk-client` (C++) and `sdk-client-unity` (Unity C#), and component-level security invariants.
- **Scoped Decisions Confirmed**:
  1. **Retain `api-client/` as Migration Reference**: Keep `api-client/` in the repository so developers migrating from REST to Native PC SDK can see the before/after comparison, while fixing its security vulnerabilities and stripping dead Android NDK code.
  2. **Full 4-Project Coverage**: Cover `sdk-client`, `sdk-client-unity`, `api-client`, and `api-server`.
  3. **Core Billing + Purchase Restoration (`QueryPurchases`) + Play Integrity Parity**:
     - Preserve and fix all 7 core Billing SDK functionalities (`Initialize Billing SDK`, `QueryProductDetails`, `IsBillingProgramAvailable`, `CreateBillingProgramReportingDetails`, `LaunchPurchaseFlow`, `AcknowledgePurchase`, `ConsumePurchase`).
     - Keep and harden the existing Play Integrity operations (`PrepareIntegrityToken`, `RequestIntegrityToken`, `VerifyIntegrityToken`) with cryptographic SHA-256 `request_hash`.
     - Expose `BillingClient::QueryPurchases` in `sdk-client` and `QueryPurchasesAsync()` in `sdk-client-unity` to match `api-client`'s existing `Get Purchases` operation and satisfy the Play Games PC Launch Checklist.
  4. **5 Implementation Invariants Locked Down**:
     - **UI Separation**: Keep only `"Billing SDK"` in `sdk-client/` (removing dead `GameAppBaseWin.hpp` / `LegacyGameApis.json` / `AppConfig.json`) and keep only `"Billing APIs"` in `api-client/`.
     - **Unified Operation Sequence**: Add `Query Purchases` and align the operation order across `sdk-client` and `sdk-client-unity`.
     - **Unity Scene Preservation**: Leave `.unity` scene files in `sdk-client-unity/` untouched; dynamically instantiate/clone UI buttons at runtime in `GameStoreManager.cs` when serialized button fields are unassigned, verified against the official `PlayPcSdkManaged` C# wrapper signatures.
     - **Backend Auth Arrangement**: Preserve the existing token-based request arrangement between sample clients and `api-server` (Service Account JWT on the server; `userAccessToken`, `purchaseToken`, or `integrity_token` passed by clients without Firebase Auth), while remediating URL path/query injection (`SSRF`) via `encodeURIComponent()` and parameterized `googleapis` methods.
     - **Version-Independent SDK Path**: Standardize `GameApplication.vcxproj` across all configurations (`Debug`/`Release`, `Win32`/`x64`) on `..\play_pc_sdk`.
