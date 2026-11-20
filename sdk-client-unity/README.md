# Play Games PC SDK Unity Sample (`sdk-client-unity`)

This Unity 6000 sample demonstrates how to integrate the **Google Play Games on PC Unity SDK (`26.9.261.0+`)** via `PlayPcSdkManaged` and `PlayPcSdkFactory`, matching the exact 11-step operation sequence of [`sdk-client/`](../sdk-client/):

1. **Initialize SDK** (`GooglePlayInitialization.InitializeAsync` + `PlayPcSdkFactory.CreateBillingClient` + `PlayPcSdkFactory.CreateIntegrityClient`)
2. **Query Products** (`BillingClient.QueryProductDetailsAsync`, including `FullPriceMicros` discount metadata)
3. **IsBillingProgramAvailable** (`BillingClient.IsBillingProgramAvailableAsync`)
4. **CreateBillingProgramReportingDetails** (`BillingClient.CreateBillingProgramReportingDetailsAsync` $\rightarrow$ `ReportingDetails` external transaction token)
5. **Launch Purchase Flow** (`BillingClient.LaunchPurchaseFlowAsync`, distinguishing `PurchaseState.Purchased` vs `PurchaseState.Pending`)
6. **Query Purchases** (`BillingClient.QueryPurchasesAsync` for startup/resume purchase restoration)
7. **Acknowledge Purchase** (`BillingClient.AcknowledgePurchaseAsync`)
8. **Consume Purchase** (`BillingClient.ConsumePurchaseAsync`)
9. **Prepare Integrity Token** (`IntegrityClient.PrepareIntegrityTokenAsync`)
10. **Request Integrity Token** (`IntegrityClient.RequestIntegrityTokenAsync` with cryptographic SHA-256 `RequestHash`)
11. **Verify Integrity Token** (`POST /api/verifyIntegrityToken` via [`api-server`](../api-server))

> **Scene Compatibility Note**: [`Assets/Scenes/SampleScene.unity`](Assets/Scenes/SampleScene.unity) is preserved untouched. At runtime, [`GameStoreManager.cs`](Assets/Scripts/GameStoreManager.cs) automatically clones any unassigned UI buttons from `_initializeButton` and arranges all operations in a clean 3-column grid in the exact 1..11 order above.

---

## Step 1: Import the Play Games PC Unity SDK Package (`26.9.261.0+`)

1. Download the latest Unity `.tgz` package from [Integrate Google Play Games PC SDK with Unity](https://developer.android.com/games/playgames/native-pc/unity/unity_start).
2. Open this project (`sdk-client-unity/`) in **Unity Hub / Unity Editor**.
3. Open **Window > Package Manager**, click **+ > Add package from tarball...**, and select `play_pc_sdk_unity_x.x.x.tgz`.

## Step 2: Configure `GameStoreManager` in `SampleScene`

1. Open `Assets/Scenes/SampleScene.unity` and select the `GameStoreManager` GameObject.
2. In the Inspector under **Configuration**, set:
   - **Package Name**: Your Play Console package name (e.g., `com.example.yourgame`).
   - **Cloud Project Number**: Your numeric Google Cloud project number *(must match the Google Cloud project of the Service Account configured in [`api-server`](../api-server) so `decodePcIntegrityToken` can decrypt the token)*.
   - **Product Ids**: Your in-app product IDs configured in Google Play Console.
   - **Backend Base Url**: Your deployed [`api-server`](../api-server) base URL.

## Step 3: Build Settings (`IL2CPP`)

1. Open **File > Build Settings > Player Settings...**.
2. Under **Configuration**, set **Scripting Backend** to **IL2CPP** and target **Windows (`x86_64`)**.
3. Build the Windows `.exe`.

## Step 4: Place `manifest.xml` Next to Your Built `.exe`

Place a `manifest.xml` file in the same directory as your built `.exe` (tag names are case-sensitive PascalCase):

```xml
<?xml version="1.0" encoding="utf-8"?>
<Manifest version="1">
  <Application>
    <PackageName>com.example.yourgame</PackageName>
  </Application>
</Manifest>
```

> **Local Developer Mode Note**: When launching an unsigned local build outside the Google Play Games client using `EarlyAccessPartnerGuid`, verify the key is set in the **64-bit** Windows Registry view (`HKLM\Software\Google\Play Games Services\EarlyAccessPartnerGuid`, `/reg:64`) rather than `WOW6432Node`.
