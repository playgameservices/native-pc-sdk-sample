# **Guide: Creating a Sample Game with the Play PC SDK Unity Package v1.0** 

**Author**: [Milan Pandey](mailto:mipandey@google.com) [Hakim Hauston](mailto:hakimh@google.com)  
**Last updated**: Nov 19, 2025

## **Introduction**

This document walks through the process of integrating the refactored Play PC SDK C\# Wrapper into a new Unity project.

This new version of the wrapper features a decoupled, modern architecture that separates the core SDK logic from the Unity engine. The primary way to interact with the SDK is through the PlayPcSdkFactory, which provides safe, pre-configured, and easy-to-use clients for all SDK features.**Explicit Initialization:** You now manually initialize the SDK by passing a handler from the factory: GooglePlayInitialization.InitializeAsync(PlayPcSdkFactory.InitializationHandler). This gives you full control over the startup sequence.

* **Factory-Based Clients:** Instead of new BillingClient(), you now use the factory to get a pre-configured instance: PlayPcSdkFactory.CreateBillingClient(). This is safer and hides implementation details.  
* **Result Pattern (No Exceptions):** Asynchronous methods no longer throw SdkException for API errors. Instead, they return a Result object. You must now check result.IsOk to handle success and failure cases, which mirrors the C++ SDK experience.

**Unity Package download URL:** [PlayPcSdkManaged\_25.11.345.0.unitypackage](https://drive.google.com/file/d/1a5JfdniJosRMYnJvGbwihrP739_eXl4B/view?usp=sharing) 

### **Key Usage Changes from Prior Versions:**

* 

## **Step 1: Create a New Unity Project**

1. Open **Unity Hub**.  
2. Click **New project**.  
3. Select the **3D Core** template.  
4. Name the project (e.g., PlaySdkSample).  
5. Click **Create project**.

## **Step 2: Import the SDK Package**

1. Locate the PlayPcSdk.unitypackage file.  
2. Drag and drop this file directly into the **Assets** area of the Unity Editor's Project window.  
3. The "Import Unity Package" window will appear. Leave everything checked and click **Import**.  
4. The SDK is now installed. You will find it correctly structured in the Assets/Plugins/PlayPcSdkManaged folder.

## **Step 3: Create the Game Scene and UI**

We'll build a simple UI to interact with the SDK.

1. **Create the Canvas**: In the Hierarchy window, right-click and go to **UI \> Canvas**.  
2. **Add UI Elements**: Inside the Canvas, create the following:  
   * **Text (TextMeshPro)** for a title ("Game Store").  
   * **Text (TextMeshPro)** for logging output. Anchor it to fill the bottom half of the screen.  
   * Five **Button (TextMeshPro)** elements. Name and label them:  
     * InitializeButton \-\> "Connect to Store"  
     * IntegrityButton \-\> "Check Integrity"  
     * QueryProductsButton \-\> "Load Products"  
     * PurchaseButton \-\> "Buy Item"  
     * ExitButton \-\> "Exit Game"

## **Step 4: Write the Game Logic Script**

1. In the Assets folder, create a new folder named Scripts.  
2. Inside Scripts, create a new C\# script and name it GameStoreManager.cs.  
3. **Replace the entire content** of this new file with the code from **Appendix A** [below](#bookmark=kix.1v4gsdhxznzh).

## **Step 5: Connect UI and Run**

1. In the Hierarchy, create an empty GameObject and name it GameStoreManager.  
2. Drag the GameStoreManager.cs script onto this new object.  
3. Select the GameStoreManager object. In the Inspector, you will see empty slots for the UI elements and a configuration field.  
4. **Important**: In the **Cloud Project Number** field, enter the numeric ID from your Google Cloud Console project.  
5. Drag each UI element from the Hierarchy into its corresponding slot in the Inspector.  
6. Press **Play** to run the scene and test the buttons.

## **Step 6: Build Settings**

Before building, you must configure the project to use the **IL2CPP** scripting backend.

1. Go to **File \> Build Settings**.  
2. Click on **Player Settings...**.  
3. In the Player settings window, find the **Configuration** section.  
4. Set **Scripting Backend** to **IL2CPP**.  
5. Set **Api Compatibility Level** to **.NET Framework** (or **.NET Standard 2.1** in newer Unity versions).  
6. Close the settings and return to the Build Settings window to build your game.

## **Step 7: Place the Manifest File and Run**

1. Inside the Build directory you just created, place the manifest.xml file alongside the game's .exe file. ([A sample is provided in Appendix B](#bookmark=kix.1v4gsdhxznzh)).  
2. Launch the game executable.

## **Appendix A: GameStoreManager.cs** 

```c#
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
            _billingClient = PlayPcSdkFactory.CreateBillingClient();
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
            new ProductId { Id = "tic_tac_toe_product_1", ProductType = ProductType.InApp }
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

```

## 

## 

## **Appendix B: manifest.xml**

```xml
<?xml version="1.0" encoding="utf-8"?>
<manifest version="1">
    <application>
        <!-- Replace with your game's package name -->
        <package_name>com.yourcompany.yourawesomegame</package_name>
        <play_games_services>
            <!-- Replace with your Project ID from the Google Play Console -->
            <project_id>123456789101</project_id>
        </play_games_services>
        <developer_mode>true</developer_mode>
    </application>
</manifest>

```

