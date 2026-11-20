using System;
using System.Collections.Generic;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using TMPro;
using UnityEngine;
using UnityEngine.Networking;
using UnityEngine.UI;

// Play Games PC SDK Managed Wrapper namespaces
using PlayPcSdkManaged.Billing;
using PlayPcSdkManaged.Core;
using PlayPcSdkManaged.Initialization;
using PlayPcSdkManaged.Integrity;
using PlayPcSdkManaged.Unity;

public class GameStoreManager : MonoBehaviour
{
    [Header("Configuration")]
    [Tooltip("Your Play Console application package name (e.g., com.yourcompany.yourgame).")]
    [SerializeField] private string _packageName = "com.yourcompany.yourgame";

    [Tooltip("The numeric Project Number from your Google Cloud Console.")]
    [SerializeField] private long _cloudProjectNumber;

    [Tooltip("In-app product IDs configured in Google Play Console.")]
    [SerializeField] private string[] _productIds = new[]
    {
        "com.yourcompany.product_01",
        "com.yourcompany.product_02",
        "com.yourcompany.product_03",
        "com.yourcompany.product_04"
    };

    [Tooltip("BillingProgram to query when testing alternative billing APIs.")]
    [SerializeField] private BillingProgram _billingProgram = BillingProgram.UserChoice;

    [Tooltip("Backend base URL (api-server) used to verify Play Integrity tokens.")]
    [SerializeField] private string _backendBaseUrl = "https://your-project.web.app";

    [Header("UI References (Existing Scene Bindings Preserved)")]
    [SerializeField] private Button _initializeButton;
    [SerializeField] private Button _queryProductsButton;
    [SerializeField] private Button _isBillingProgramAvailableButton;
    [SerializeField] private Button _createReportingDetailsButton;
    [SerializeField] private Button _purchaseButton;
    [SerializeField] private Button _queryPurchasesButton;
    [SerializeField] private Button _acknowledgePurchaseButton;
    [SerializeField] private Button _consumePurchaseButton;
    [SerializeField] private Button _integrityButton;
    [SerializeField] private Button _requestIntegrityButton;
    [SerializeField] private Button _verifyIntegrityButton;
    [SerializeField] private Button _exitButton;
    [SerializeField] private TextMeshProUGUI _logText;

    // SDK Clients
    private BillingClient _billingClient;
    private IntegrityClient _integrityClient;

    // Cached Billing & Play Integrity state
    private string _offerToken;
    private string _lastPurchaseToken;
    private readonly List<ProductPurchaseDetails> _knownPurchases = new List<ProductPurchaseDetails>();
    private RequestTokenData? _preparedRequestTokenData;
    private string _cachedRequestHash;
    private string _lastIntegrityToken;

    [Serializable]
    private class VerifyIntegrityRequest
    {
        public string packageName;
        public string token;
    }

    [Serializable]
    private class VerifyIntegrityResponse
    {
        public bool result;
        public string hash;
        public string error;
    }

    void Start()
    {
        SetupUnifiedButtonSequence();

        _initializeButton?.onClick.AddListener(InitializeSDK);
        _queryProductsButton?.onClick.AddListener(QueryProducts);
        _isBillingProgramAvailableButton?.onClick.AddListener(CheckBillingProgramAvailable);
        _createReportingDetailsButton?.onClick.AddListener(CreateBillingProgramReportingDetails);
        _purchaseButton?.onClick.AddListener(PurchaseItem);
        _queryPurchasesButton?.onClick.AddListener(QueryPurchases);
        _acknowledgePurchaseButton?.onClick.AddListener(AcknowledgePurchase);
        _consumePurchaseButton?.onClick.AddListener(ConsumePurchase);
        _integrityButton?.onClick.AddListener(PrepareIntegrityToken);
        _requestIntegrityButton?.onClick.AddListener(RequestIntegrityToken);
        _verifyIntegrityButton?.onClick.AddListener(VerifyIntegrityToken);
        _exitButton?.onClick.AddListener(ExitGame);

        SetPostInitButtonsInteractable(false);
        if (_purchaseButton != null) _purchaseButton.interactable = false;
        if (_acknowledgePurchaseButton != null) _acknowledgePurchaseButton.interactable = false;
        if (_consumePurchaseButton != null) _consumePurchaseButton.interactable = false;
        if (_requestIntegrityButton != null) _requestIntegrityButton.interactable = false;
        if (_verifyIntegrityButton != null) _verifyIntegrityButton.interactable = false;

        if (_logText != null)
        {
            _logText.text = "Ready. Click '1. Initialize SDK' to connect to Google Play Games on PC.";
        }
    }

    /// <summary>
    /// Dynamically clones any unassigned UI buttons from _initializeButton and arranges all
    /// operations in the exact 1..11 sequence matching sdk-client/ without modifying SampleScene.unity.
    /// </summary>
    private void SetupUnifiedButtonSequence()
    {
        if (_initializeButton == null)
        {
            return;
        }

        Transform panel = _initializeButton.transform.parent;
        if (panel != null && panel.GetComponent<LayoutGroup>() == null)
        {
            GridLayoutGroup grid = panel.gameObject.AddComponent<GridLayoutGroup>();
            grid.cellSize = new Vector2(220f, 32f);
            grid.spacing = new Vector2(10f, 8f);
            grid.constraint = GridLayoutGroup.Constraint.FixedColumnCount;
            grid.constraintCount = 3;
            grid.childAlignment = TextAnchor.UpperCenter;
        }

        _initializeButton = EnsureButton(_initializeButton, panel, "InitializeSDKButton", "1. Initialize SDK");
        _queryProductsButton = EnsureButton(_queryProductsButton, panel, "QueryProductsButton", "2. Query Products");
        _isBillingProgramAvailableButton = EnsureButton(
            _isBillingProgramAvailableButton, panel, "IsBillingProgramAvailableButton", "3. IsBillingProgramAvail");
        _createReportingDetailsButton = EnsureButton(
            _createReportingDetailsButton, panel, "CreateReportingDetailsButton", "4. Reporting Details");
        _purchaseButton = EnsureButton(_purchaseButton, panel, "LaunchPurchaseButton", "5. Launch Purchase");
        _queryPurchasesButton = EnsureButton(
            _queryPurchasesButton, panel, "QueryPurchasesButton", "6. Query Purchases");
        _acknowledgePurchaseButton = EnsureButton(
            _acknowledgePurchaseButton, panel, "AcknowledgePurchaseButton", "7. Acknowledge");
        _consumePurchaseButton = EnsureButton(
            _consumePurchaseButton, panel, "ConsumePurchaseButton", "8. Consume");
        _integrityButton = EnsureButton(_integrityButton, panel, "PrepareIntegrityButton", "9. Prepare Integrity");
        _requestIntegrityButton = EnsureButton(
            _requestIntegrityButton, panel, "RequestIntegrityButton", "10. Request Integrity");
        _verifyIntegrityButton = EnsureButton(
            _verifyIntegrityButton, panel, "VerifyIntegrityButton", "11. Verify Integrity");
        _exitButton = EnsureButton(_exitButton, panel, "ExitButton", "Exit Game");

        Button[] orderedButtons =
        {
            _initializeButton,
            _queryProductsButton,
            _isBillingProgramAvailableButton,
            _createReportingDetailsButton,
            _purchaseButton,
            _queryPurchasesButton,
            _acknowledgePurchaseButton,
            _consumePurchaseButton,
            _integrityButton,
            _requestIntegrityButton,
            _verifyIntegrityButton,
            _exitButton
        };

        for (int i = 0; i < orderedButtons.Length; i++)
        {
            if (orderedButtons[i] != null)
            {
                orderedButtons[i].transform.SetSiblingIndex(i);
            }
        }
    }

    private Button EnsureButton(Button existing, Transform parent, string objectName, string label)
    {
        Button btn = existing;
        if (btn == null)
        {
            GameObject clone = Instantiate(_initializeButton.gameObject, parent);
            clone.name = objectName;
            btn = clone.GetComponent<Button>();
            btn.onClick = new Button.ButtonClickedEvent();
        }

        btn.gameObject.SetActive(true);
        TMP_Text tmpLabel = btn.GetComponentInChildren<TMP_Text>(true);
        if (tmpLabel != null)
        {
            tmpLabel.text = label;
        }
        return btn;
    }

    private void SetPostInitButtonsInteractable(bool interactable)
    {
        if (_queryProductsButton != null) _queryProductsButton.interactable = interactable;
        if (_isBillingProgramAvailableButton != null) _isBillingProgramAvailableButton.interactable = interactable;
        if (_createReportingDetailsButton != null) _createReportingDetailsButton.interactable = interactable;
        if (_queryPurchasesButton != null) _queryPurchasesButton.interactable = interactable;
        if (_integrityButton != null) _integrityButton.interactable = interactable;
    }

    private void UpdatePurchaseActionButtons()
    {
        bool hasToken = !string.IsNullOrEmpty(_lastPurchaseToken);
        if (_acknowledgePurchaseButton != null) _acknowledgePurchaseButton.interactable = hasToken;
        if (_consumePurchaseButton != null) _consumePurchaseButton.interactable = hasToken;
    }

    // 1. Initialize Billing SDK
    private async void InitializeSDK()
    {
        Log("Initializing Play Games PC SDK...");
        if (_initializeButton != null) _initializeButton.interactable = false;

        var initializationHandler = PlayPcSdkFactory.InitializationHandler;
        var initParams = new GooglePlayInitializeParams
        {
            PackageName = string.IsNullOrWhiteSpace(_packageName) || _packageName == "com.yourcompany.yourgame"
                ? null
                : _packageName
        };

        var result = await GooglePlayInitialization.InitializeAsync(initializationHandler, initParams);
        if (result.IsOk)
        {
            _billingClient?.Dispose();
            _integrityClient?.Dispose();

            var clientParams = new BillingClientParams { EnablePendingPurchases = true };
            _billingClient = PlayPcSdkFactory.CreateBillingClient(clientParams);
            _integrityClient = PlayPcSdkFactory.CreateIntegrityClient();

            Log($"<color=green>SDK initialized! Environment={result.Value.Environment}, FormFactor={result.Value.FormFactor}</color>");
            SetPostInitButtonsInteractable(true);
        }
        else
        {
            Log($"<color=red>Initialization failed! Code={result.Code}, Message={result.ErrorMessage}</color>");

            switch (result.Code)
            {
                case InitializationError.ActionRequiredShutdownClientProcess:
                    Log("<color=yellow>ActionRequiredShutdownClientProcess: Google Play Games is relaunching the game. Quitting current process...</color>");
                    Application.Quit();
                    return;
                case InitializationError.SdkRuntimeUpdateRequired:
                    Log("<color=yellow>Please update Google Play Games on PC and the SDK runtime.</color>");
                    break;
                case InitializationError.SdkRuntimeUnavailable:
                    Log("<color=yellow>Google Play Games SDK runtime service is unavailable.</color>");
                    break;
                case InitializationError.SdkRuntimeNotInstalled:
                    Log("<color=yellow>Google Play Games on PC is not installed on this machine.</color>");
                    break;
            }

            if (_initializeButton != null) _initializeButton.interactable = true;
        }
    }

    // 2. Query Product Details
    private async void QueryProducts()
    {
        if (_billingClient == null)
        {
            Log("<color=red>Initialize the SDK first.</color>");
            return;
        }

        Log("Querying product details...");
        _queryProductsButton.interactable = false;

        var productIds = new List<ProductId>();
        if (_productIds != null)
        {
            foreach (string id in _productIds)
            {
                if (!string.IsNullOrWhiteSpace(id))
                {
                    productIds.Add(new ProductId { Id = id.Trim(), ProductType = ProductType.InApp });
                }
            }
        }

        var queryParams = new QueryProductDetailsParams { ProductIds = productIds };
        var result = await _billingClient.QueryProductDetailsAsync(queryParams);

        if (result.IsOk)
        {
            var detailsList = result.Value.ProductDetailsList;
            if (detailsList != null && detailsList.Count > 0)
            {
                _offerToken = null;
                foreach (var product in detailsList)
                {
                    if (product.ProductOffers != null && product.ProductOffers.Count > 0)
                    {
                        var offer = product.ProductOffers[0];
                        if (string.IsNullOrEmpty(_offerToken))
                        {
                            _offerToken = offer.OfferToken;
                        }
                        string discountInfo = offer.FullPriceMicros > 0
                            ? $" (FullPriceMicros: {offer.FullPriceMicros})"
                            : string.Empty;
                        Log($"<color=green>Product Loaded: {product.Id} - {product.Title} ({offer.FormattedPrice}{discountInfo})</color>");
                    }
                    else
                    {
                        Log($"<color=yellow>Product '{product.Id}' ({product.Title}) has no active offers.</color>");
                    }
                }

                if (_purchaseButton != null)
                {
                    _purchaseButton.interactable = !string.IsNullOrEmpty(_offerToken);
                }
            }
            else
            {
                Log("<color=yellow>QueryProductDetails succeeded, but 0 products matched the configured Product IDs.</color>");
            }
        }
        else
        {
            Log($"<color=red>QueryProductDetails failed! Code={result.Code}, Message={result.ErrorMessage}</color>");
        }

        _queryProductsButton.interactable = true;
    }

    // 3. IsBillingProgramAvailable
    private async void CheckBillingProgramAvailable()
    {
        if (_billingClient == null)
        {
            Log("<color=red>Initialize the SDK first.</color>");
            return;
        }

        Log($"Checking IsBillingProgramAvailable({_billingProgram})...");
        _isBillingProgramAvailableButton.interactable = false;

        var checkParams = new IsBillingProgramAvailableParams { BillingProgram = _billingProgram };
        var result = await _billingClient.IsBillingProgramAvailableAsync(checkParams);

        if (result.IsOk)
        {
            Log($"<color=green>IsBillingProgramAvailable({_billingProgram}): Available={result.Value.IsBillingProgramAvailable}</color>");
        }
        else
        {
            Log($"<color=yellow>IsBillingProgramAvailable({_billingProgram}) not available: Code={result.Code}, Message={result.ErrorMessage}</color>");
        }

        _isBillingProgramAvailableButton.interactable = true;
    }

    // 4. CreateBillingProgramReportingDetails
    private async void CreateBillingProgramReportingDetails()
    {
        if (_billingClient == null)
        {
            Log("<color=red>Initialize the SDK first.</color>");
            return;
        }

        Log($"Calling CreateBillingProgramReportingDetails({_billingProgram})...");
        _createReportingDetailsButton.interactable = false;

        var reportParams = new CreateBillingProgramReportingDetailsParams { BillingProgram = _billingProgram };
        var result = await _billingClient.CreateBillingProgramReportingDetailsAsync(reportParams);

        if (result.IsOk)
        {
            Log($"<color=green>CreateBillingProgramReportingDetails succeeded! ExternalTransactionToken={result.Value.ReportingDetails}</color>");
        }
        else
        {
            Log($"<color=red>CreateBillingProgramReportingDetails failed! Code={result.Code}, Message={result.ErrorMessage}</color>");
        }

        _createReportingDetailsButton.interactable = true;
    }

    // 5. Launch Purchase Flow
    private async void PurchaseItem()
    {
        if (_billingClient == null || string.IsNullOrEmpty(_offerToken))
        {
            Log("<color=yellow>Please run Query Products first to obtain an OfferToken.</color>");
            return;
        }

        Log("Launching purchase flow...");
        _purchaseButton.interactable = false;

        var purchaseParams = new LaunchPurchaseFlowParams
        {
            OfferToken = _offerToken,
            Quantity = 1
        };

        var result = await _billingClient.LaunchPurchaseFlowAsync(purchaseParams);
        if (result.IsOk)
        {
            ProductPurchaseDetails details = result.Value.ProductPurchaseDetails;
            if (details.PurchaseState == PurchaseState.Purchased)
            {
                _lastPurchaseToken = details.PurchaseToken;
                Log($"<color=green>Purchase completed! OrderId={details.OrderId}, ProductId={details.ProductId}, Qty={details.Quantity}, Ack={details.IsAcknowledged}</color>");
                UpdatePurchaseActionButtons();
            }
            else if (details.PurchaseState == PurchaseState.Pending)
            {
                Log($"<color=yellow>Purchase is PENDING external completion (ProductId={details.ProductId}). Do not grant entitlement until Purchased.</color>");
            }
            else
            {
                Log($"<color=yellow>Purchase returned state {details.PurchaseState}.</color>");
            }
        }
        else if (result.Code == BillingError.UserCanceled)
        {
            Log("<color=yellow>Purchase canceled by user.</color>");
        }
        else
        {
            Log($"<color=red>LaunchPurchaseFlow failed! Code={result.Code}, Message={result.ErrorMessage}</color>");
        }

        _purchaseButton.interactable = true;
    }

    // 6. Query Purchases (Purchase Restoration)
    private async void QueryPurchases()
    {
        if (_billingClient == null)
        {
            Log("<color=red>Initialize the SDK first.</color>");
            return;
        }

        Log("Querying owned/restored purchases...");
        _queryPurchasesButton.interactable = false;

        var result = await _billingClient.QueryPurchasesAsync();
        if (result.IsOk)
        {
            _knownPurchases.Clear();
            var list = result.Value.ProductPurchaseDetails;
            int count = list != null ? list.Count : 0;
            Log($"<color=green>QueryPurchases succeeded: {count} purchase(s) found.</color>");

            if (list != null)
            {
                foreach (var purchase in list)
                {
                    _knownPurchases.Add(purchase);
                    Log($"  - ProductId={purchase.ProductId}, State={purchase.PurchaseState}, Ack={purchase.IsAcknowledged}, OrderId={purchase.OrderId}");
                    if (purchase.PurchaseState == PurchaseState.Purchased && !string.IsNullOrEmpty(purchase.PurchaseToken))
                    {
                        _lastPurchaseToken = purchase.PurchaseToken;
                    }
                }
            }
            UpdatePurchaseActionButtons();
        }
        else
        {
            Log($"<color=red>QueryPurchases failed! Code={result.Code}, Message={result.ErrorMessage}</color>");
        }

        _queryPurchasesButton.interactable = true;
    }

    // 7. Acknowledge Purchase
    private async void AcknowledgePurchase()
    {
        if (_billingClient == null || string.IsNullOrEmpty(_lastPurchaseToken))
        {
            Log("<color=yellow>No active PurchaseToken available. Complete a purchase or run Query Purchases first.</color>");
            return;
        }

        Log("Acknowledging purchase...");
        _acknowledgePurchaseButton.interactable = false;

        var ackParams = new AcknowledgePurchaseParams { PurchaseToken = _lastPurchaseToken };
        var result = await _billingClient.AcknowledgePurchaseAsync(ackParams);

        if (result.IsOk)
        {
            Log("<color=green>AcknowledgePurchase succeeded!</color>");
        }
        else
        {
            Log($"<color=red>AcknowledgePurchase failed! Code={result.Code}, Message={result.ErrorMessage}</color>");
        }

        UpdatePurchaseActionButtons();
    }

    // 8. Consume Purchase
    private async void ConsumePurchase()
    {
        if (_billingClient == null || string.IsNullOrEmpty(_lastPurchaseToken))
        {
            Log("<color=yellow>No active PurchaseToken available. Complete a purchase or run Query Purchases first.</color>");
            return;
        }

        Log("Consuming purchase...");
        _consumePurchaseButton.interactable = false;

        string tokenToConsume = _lastPurchaseToken;
        var consumeParams = new ConsumePurchaseParams { PurchaseToken = tokenToConsume };
        var result = await _billingClient.ConsumePurchaseAsync(consumeParams);

        if (result.IsOk)
        {
            Log("<color=green>ConsumePurchase succeeded! Item can now be purchased again.</color>");
            _lastPurchaseToken = null;
        }
        else
        {
            Log($"<color=red>ConsumePurchase failed! Code={result.Code}, Message={result.ErrorMessage}</color>");
        }

        UpdatePurchaseActionButtons();
    }

    // 9. Prepare Integrity Token
    private async void PrepareIntegrityToken()
    {
        if (_integrityClient == null)
        {
            Log("<color=red>Initialize the SDK first.</color>");
            return;
        }
        if (_cloudProjectNumber <= 0)
        {
            Log("<color=yellow>Set a valid Cloud Project Number on GameStoreManager in the Inspector first.</color>");
            return;
        }

        Log("Preparing Play Integrity token...");
        _integrityButton.interactable = false;

        var prepareParams = new PrepareIntegrityTokenParams { CloudProjectNumber = _cloudProjectNumber };
        var prepareResult = await _integrityClient.PrepareIntegrityTokenAsync(prepareParams);

        if (prepareResult.IsOk)
        {
            _preparedRequestTokenData = prepareResult.Value.RequestTokenData;
            Log("<color=green>PrepareIntegrityToken succeeded! Ready to request integrity token.</color>");
            if (_requestIntegrityButton != null) _requestIntegrityButton.interactable = true;
        }
        else
        {
            Log($"<color=red>PrepareIntegrityToken failed! Code={prepareResult.Code}, Message={prepareResult.ErrorMessage}</color>");
        }

        _integrityButton.interactable = true;
    }

    // 10. Request Integrity Token (with cryptographic SHA-256 RequestHash)
    private async void RequestIntegrityToken()
    {
        if (_integrityClient == null || !_preparedRequestTokenData.HasValue)
        {
            Log("<color=yellow>Run Prepare Integrity first.</color>");
            return;
        }

        Log("Requesting Play Integrity token with SHA-256 requestHash...");
        _requestIntegrityButton.interactable = false;

        _cachedRequestHash = ComputeIntegrityRequestHash(_packageName, "unity_verify_integrity");
        var requestParams = new RequestIntegrityTokenParams
        {
            RequestTokenData = _preparedRequestTokenData.Value,
            RequestHash = _cachedRequestHash
        };

        var requestResult = await _integrityClient.RequestIntegrityTokenAsync(requestParams);
        if (requestResult.IsOk)
        {
            _lastIntegrityToken = requestResult.Value.TokenBytes;
            Log($"<color=green>RequestIntegrityToken succeeded! Token length={_lastIntegrityToken.Length}, SHA-256 hash={_cachedRequestHash}</color>");
            if (_verifyIntegrityButton != null) _verifyIntegrityButton.interactable = true;
        }
        else
        {
            Log($"<color=red>RequestIntegrityToken failed! Code={requestResult.Code}, Message={requestResult.ErrorMessage}</color>");
        }

        _requestIntegrityButton.interactable = true;
    }

    // 11. Verify Integrity Token (via api-server)
    private async void VerifyIntegrityToken()
    {
        if (string.IsNullOrEmpty(_lastIntegrityToken))
        {
            Log("<color=yellow>Run Request Integrity first.</color>");
            return;
        }

        string baseUrl = (_backendBaseUrl ?? string.Empty).TrimEnd('/');
        if (string.IsNullOrEmpty(baseUrl))
        {
            Log("<color=yellow>Configure Backend Base Url on GameStoreManager first.</color>");
            return;
        }

        string verifyUrl = baseUrl + "/api/verifyIntegrityToken";
        Log($"Verifying integrity token via {verifyUrl}...");
        _verifyIntegrityButton.interactable = false;

        string jsonBody = JsonUtility.ToJson(new VerifyIntegrityRequest
        {
            packageName = _packageName,
            token = _lastIntegrityToken
        });

        using (var request = new UnityWebRequest(verifyUrl, UnityWebRequest.kHttpVerbPOST))
        {
            byte[] bodyRaw = Encoding.UTF8.GetBytes(jsonBody);
            request.uploadHandler = new UploadHandlerRaw(bodyRaw);
            request.downloadHandler = new DownloadHandlerBuffer();
            request.SetRequestHeader("Content-Type", "application/json");

            var op = request.SendWebRequest();
            while (!op.isDone)
            {
                await Task.Yield();
            }

            if (request.result != UnityWebRequest.Result.Success)
            {
                Log($"<color=red>Backend HTTP error ({request.responseCode}): {request.error}</color>");
            }
            else
            {
                VerifyIntegrityResponse parsed = JsonUtility.FromJson<VerifyIntegrityResponse>(request.downloadHandler.text);
                if (parsed != null && parsed.result)
                {
                    if (parsed.hash == _cachedRequestHash)
                    {
                        Log("<color=green>VerifyIntegrityToken succeeded! MEETS_PC_INTEGRITY & SHA-256 requestHash verified.</color>");
                    }
                    else
                    {
                        Log($"<color=red>VerifyIntegrityToken hash mismatch! Expected={_cachedRequestHash}, Received={parsed.hash}</color>");
                    }
                }
                else
                {
                    string err = parsed != null ? parsed.error : request.downloadHandler.text;
                    Log($"<color=red>VerifyIntegrityToken rejected: {err}</color>");
                }
            }
        }

        _verifyIntegrityButton.interactable = true;
    }

    private static string ComputeIntegrityRequestHash(string packageName, string action)
    {
        byte[] nonceBytes = new byte[16];
        using (var rng = RandomNumberGenerator.Create())
        {
            rng.GetBytes(nonceBytes);
        }

        var nonceHex = new StringBuilder(32);
        foreach (byte b in nonceBytes)
        {
            nonceHex.Append(b.ToString("x2"));
        }

        string canonicalPayload = $"{packageName}|{action}|{DateTimeOffset.UtcNow.ToUnixTimeSeconds()}|{nonceHex}";
        using (var sha256 = SHA256.Create())
        {
            byte[] digest = sha256.ComputeHash(Encoding.UTF8.GetBytes(canonicalPayload));
            var hex = new StringBuilder(64);
            foreach (byte b in digest)
            {
                hex.Append(b.ToString("x2"));
            }
            return hex.ToString();
        }
    }

    private void ExitGame()
    {
        Log("Exiting application...");
        Application.Quit();
    }

    private void Log(string message)
    {
        if (_logText != null) _logText.text += "\n> " + message;
        Debug.Log(message);
    }

    void OnDestroy()
    {
        _billingClient?.Dispose();
        _integrityClient?.Dispose();
    }
}
