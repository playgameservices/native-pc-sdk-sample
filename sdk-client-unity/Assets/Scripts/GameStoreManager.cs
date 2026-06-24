using UnityEngine;
using UnityEngine.UI;
using TMPro;
using System.Collections.Generic;
using System.Threading.Tasks;
using System;

// namespaces for the wrapper
using PlayPcSdkManaged.Core;
using PlayPcSdkManaged.Initialization;
using PlayPcSdkManaged.Billing;
using PlayPcSdkManaged.Integrity;
using PlayPcSdkManaged.Unity;

public class GameStoreManager : MonoBehaviour
{
    [Header("Configuration")]
    [Tooltip("The numeric Project ID from your Google Cloud Console.")]
    [SerializeField] private long _cloudProjectNumber;
    
    [Header("UI References")]
    [SerializeField] private Button _initializeButton;
    [SerializeField] private Button _integrityButton;
    [SerializeField] private Button _queryProductsButton;
    [SerializeField] private Button _purchaseButton;
    [SerializeField] private Button _exitButton;
    [SerializeField] private TextMeshProUGUI _logText;

    // SDK Clients
    private BillingClient _billingClient;
    private IntegrityClient _integrityClient;

    // Stored product information
    private string _offerToken;

    void Start()
    {
        _initializeButton.onClick.AddListener(InitializeSDK);
        _integrityButton.onClick.AddListener(CheckIntegrity);
        _queryProductsButton.onClick.AddListener(QueryProducts);
        _purchaseButton.onClick.AddListener(PurchaseItem);
        _exitButton.onClick.AddListener(ExitGame);

        // Disable buttons until the SDK is initialized
        _integrityButton.interactable = false;
        _queryProductsButton.interactable = false;
        _purchaseButton.interactable = false;

        _logText.text = "Logs will appear here.";
    }

    private async void InitializeSDK()
    {
        Log("Connecting to the store...");
        _initializeButton.interactable = false;

        // The factory provides the necessary Unity-specific handler for initialization.
        var initializationHandler = PlayPcSdkFactory.InitializationHandler;
        var result = await GooglePlayInitialization.InitializeAsync(initializationHandler);

        if (result.IsOk)
        {
            Log("<color=green>Successfully connected to the store!</color>");

            // Use the factory to get Unity-compatible instances of the clients
            var clientParams = new BillingClientParams { EnablePendingPurchases = true };
            _billingClient = PlayPcSdkFactory.CreateBillingClient(clientParams);
            _integrityClient = PlayPcSdkFactory.CreateIntegrityClient();

            // Enable the next steps
            _integrityButton.interactable = true;
            _queryProductsButton.interactable = true;
        }
        else
        {
            Log($"<color=red>Connection failed! Code: {result.Code}, Message: {result.ErrorMessage}</color>");

            // Handle specific, actionable errors
            if (result.Code == InitializationError.ActionRequiredShutdownClientProcess)
            {
                Log("This game must be launched through the Google Play Games client. Please exit and relaunch.");
            }
            
            _initializeButton.interactable = true;
        }
    }

    private async void CheckIntegrity()
    {
        Log("Checking application integrity...");
        _integrityButton.interactable = false;

        var prepareParams = new PrepareIntegrityTokenParams { CloudProjectNumber = _cloudProjectNumber };
        var prepareResult = await _integrityClient.PrepareIntegrityTokenAsync(prepareParams);

        if (!prepareResult.IsOk)
        {
            Log($"<color=red>Could not prepare integrity token. Error: {prepareResult.Code}, Message: {prepareResult.ErrorMessage}</color>");
            _integrityButton.interactable = true;
            return;
        }

        Log("Successfully prepared token. Now requesting integrity verdict...");
        
        var requestParams = new RequestIntegrityTokenParams
        {
            RequestTokenData = prepareResult.Value.RequestTokenData,
            RequestHash = "example-request-hash" // In a real game, this should be a hash of the request data
        };
        var requestResult = await _integrityClient.RequestIntegrityTokenAsync(requestParams);

        if (requestResult.IsOk)
        {
            Log($"<color=green>Integrity check successful! Token length: {requestResult.Value.TokenBytes.Length}</color>");
        }
        else
        {
            Log($"<color=red>Integrity check failed. Error: {requestResult.Code}, Message: {requestResult.ErrorMessage}</color>");
        }

        _integrityButton.interactable = true;
    }

    private async void QueryProducts()
    {
        Log("Loading available products...");
        _queryProductsButton.interactable = false;

        var productIds = new List<ProductId>
        {
            // IMPORTANT: Replace this with a real product ID from your Google Play Console
            new ProductId { Id = "com.yourcompany.product_01", ProductType = ProductType.InApp },
            new ProductId { Id = "com.yourcompany.product_02", ProductType = ProductType.InApp },
            new ProductId { Id = "com.yourcompany.product_03", ProductType = ProductType.InApp },
            new ProductId { Id = "com.yourcompany.product_04", ProductType = ProductType.InApp }
        };
        var queryParams = new QueryProductDetailsParams { ProductIds = productIds };

        var result = await _billingClient.QueryProductDetailsAsync(queryParams);

        if (result.IsOk)
        {
            if (result.Value.ProductDetailsList != null && result.Value.ProductDetailsList.Count > 0)
            {
                var product = result.Value.ProductDetailsList[0];
                if (product.ProductOffers != null && product.ProductOffers.Count > 0)
                {
                    _offerToken = product.ProductOffers[0].OfferToken;
                    Log($"<color=green>Product Loaded: {product.Title} ({product.ProductOffers[0].FormattedPrice})</color>");
                    _purchaseButton.interactable = true;
                }
                else
                {
                    Log($"<color=yellow>Product '{product.Title}' found, but it has no offers.</color>");
                }
            }
            else
            {
                Log("<color=yellow>Store connection successful, but no products were found with that ID.</color>");
                _queryProductsButton.interactable = true;
            }
        }
        else
        {
            Log($"<color=red>Could not load products. Error: {result.Code}, Message: {result.ErrorMessage}</color>");
            _queryProductsButton.interactable = true;
        }
    }

    private async void PurchaseItem()
    {
        if (string.IsNullOrEmpty(_offerToken))
        {
            Log("<color=yellow>Please load products first!</color>");
            return;
        }

        Log("Attempting to purchase item...");
        _purchaseButton.interactable = false;

        var purchaseParams = new LaunchPurchaseFlowParams { OfferToken = _offerToken, Quantity = 1 };
        
        var result = await _billingClient.LaunchPurchaseFlowAsync(purchaseParams);
        
        if (result.IsOk)
        {
            Log($"<color=green>Purchase successful! Order ID: {result.Value.ProductPurchaseDetails.OrderId}</color>");
        }
        else
        {
            if (result.Code == BillingError.UserCanceled)
            {
                Log("<color=yellow>Purchase canceled by user.</color>");
            }
            else
            {
                Log($"<color=red>Purchase failed. Error: {result.Code}, Message: {result.ErrorMessage}</color>");
            }
        }
        
        _purchaseButton.interactable = true;
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
        // Dispose of the clients to release native resources
        _billingClient?.Dispose();
        _integrityClient?.Dispose();
    }
}
