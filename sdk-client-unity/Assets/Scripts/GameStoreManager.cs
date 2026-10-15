using UnityEngine;
using UnityEngine.UI;
using TMPro;
using System.Collections.Generic;
using System.Text;
using System.Threading.Tasks;
using System;

// namespaces for the wrapper
using PlayPcSdkManaged.Core;
using PlayPcSdkManaged.Initialization;
using PlayPcSdkManaged.Billing;
using PlayPcSdkManaged.Integrity;
using PlayPcSdkManaged.CloudSave;
using PlayPcSdkManaged.Unity;

public class GameStoreManager : MonoBehaviour
{
    [Header("Configuration")]
    [Tooltip("The numeric Project ID from your Google Cloud Console.")]
    [SerializeField] private long _cloudProjectNumber;

    [Header("Cloud Save MVP Configuration")]
    [Tooltip("Target snapshot slot name in Spanner.")]
    [SerializeField] private string _snapshotName = "save_slot_1";
    [Tooltip("Absolute local file path passed to GDS (no symlinks, junctions, or '..' traversal).")]
    [SerializeField] private string _saveFilePath = @"C:\Saves\save_slot_1.dat";
    [Tooltip("Auto-resolution conflict policy evaluated during OpenSnapshot.")]
    [SerializeField] private SnapshotConflictPolicy _conflictPolicy = SnapshotConflictPolicy.LongestPlaytime;
    [Tooltip("Metadata description committed on CommitSnapshot (full metadata replacement).")]
    [SerializeField] private string _saveDescription = "Chapter 4 - The Cave Entrance";
    [SerializeField] private long _playedTimeMillis = 18000000;
    [SerializeField] private long _progressValue = 4;
    
    [Header("UI References")]
    [SerializeField] private Button _initializeButton;
    [SerializeField] private Button _integrityButton;
    [SerializeField] private Button _queryProductsButton;
    [SerializeField] private Button _purchaseButton;
    [SerializeField] private Button _exitButton;
    [SerializeField] private TextMeshProUGUI _logText;

    [Header("Cloud Save UI References (Auto-Created if Unassigned)")]
    [SerializeField] private Button _listSnapshotsButton;
    [SerializeField] private Button _openSnapshotButton;
    [SerializeField] private Button _commitSnapshotButton;
    [SerializeField] private Button _deleteSnapshotButton;

    // SDK Clients
    private BillingClient _billingClient;
    private IntegrityClient _integrityClient;
    private GamesCloudSaveClient _cloudSaveClient;

    // Stored product information
    private string _offerToken;

    // Active Spanner revision_id obtained from OpenSnapshot (required before CommitSnapshot)
    private string _currentRevisionId;

    void Start()
    {
        EnsureCloudSaveButtonsExist();

        _initializeButton.onClick.AddListener(InitializeSDK);
        _integrityButton.onClick.AddListener(CheckIntegrity);
        _queryProductsButton.onClick.AddListener(QueryProducts);
        _purchaseButton.onClick.AddListener(PurchaseItem);
        _exitButton.onClick.AddListener(ExitGame);

        if (_listSnapshotsButton != null) _listSnapshotsButton.onClick.AddListener(ListSnapshots);
        if (_openSnapshotButton != null) _openSnapshotButton.onClick.AddListener(OpenSnapshot);
        if (_commitSnapshotButton != null) _commitSnapshotButton.onClick.AddListener(CommitSnapshot);
        if (_deleteSnapshotButton != null) _deleteSnapshotButton.onClick.AddListener(DeleteSnapshot);

        // Disable buttons until the SDK is initialized
        _integrityButton.interactable = false;
        _queryProductsButton.interactable = false;
        _purchaseButton.interactable = false;
        SetCloudSaveButtonsInteractable(false, false);

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
            _cloudSaveClient = PlayPcSdkFactory.CreateCloudSaveClient();

            // Enable the next steps
            _integrityButton.interactable = true;
            _queryProductsButton.interactable = true;
            SetCloudSaveButtonsInteractable(true, !string.IsNullOrEmpty(_currentRevisionId));
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

    // =========================================================================
    // Cloud Save MVP Operations (GamesCloudSaveClient)
    // =========================================================================

    private async void ListSnapshots()
    {
        Log("Cloud Save: Listing player snapshots...");
        if (_listSnapshotsButton != null) _listSnapshotsButton.interactable = false;

        var result = await _cloudSaveClient.ListSnapshotsAsync(new ListSnapshotsParams());

        if (result.IsOk)
        {
            var snapshots = result.Value.Metadata;
            Log($"<color=green>ListSnapshots succeeded! Found {snapshots.Count} active snapshot(s):</color>");
            foreach (var meta in snapshots)
            {
                Log($"  Slot '{meta.SnapshotName}': desc=\"{meta.Description}\", played={meta.PlayedTimeMillis}ms, progress={meta.ProgressValue}, device=\"{meta.DeviceName}\"");
            }
        }
        else
        {
            Log($"<color=red>ListSnapshots failed! Error: {result.Code}, Message: {result.ErrorMessage}</color>");
        }

        if (_listSnapshotsButton != null) _listSnapshotsButton.interactable = true;
    }

    private async void OpenSnapshot()
    {
        Log($"Cloud Save: Opening snapshot '{_snapshotName}' (policy={_conflictPolicy}, file='{_saveFilePath}')...");
        if (_openSnapshotButton != null) _openSnapshotButton.interactable = false;

        var openParams = new OpenSnapshotParams
        {
            SnapshotName = _snapshotName,
            CreateIfNotFound = true,
            ConflictPolicy = _conflictPolicy,
            FilePath = _saveFilePath
        };

        var result = await _cloudSaveClient.OpenSnapshotAsync(openParams);

        if (result.IsOk)
        {
            _currentRevisionId = result.Value.RevisionId;
            var meta = result.Value.Metadata;
            Log($"<color=green>OpenSnapshot succeeded! revision_id=\"{_currentRevisionId}\"</color>");
            Log($"  Streamed to disk: \"{result.Value.FilePath}\" | desc=\"{meta.Description}\", played={meta.PlayedTimeMillis}ms, progress={meta.ProgressValue}");

            if (_commitSnapshotButton != null) _commitSnapshotButton.interactable = true;
        }
        else
        {
            Log($"<color=red>OpenSnapshot failed! Error: {result.Code}, Message: {result.ErrorMessage}</color>");
        }

        if (_openSnapshotButton != null) _openSnapshotButton.interactable = true;
    }

    private async void CommitSnapshot()
    {
        if (string.IsNullOrEmpty(_currentRevisionId))
        {
            Log("<color=yellow>Please call OpenSnapshot first to obtain a valid Spanner revision_id!</color>");
            return;
        }

        Log($"Cloud Save: Committing snapshot '{_snapshotName}' against base revision_id='{_currentRevisionId}'...");
        if (_commitSnapshotButton != null) _commitSnapshotButton.interactable = false;

        // Write updated local save state to disk before committing (under 3 MB MVP limit)
        string sampleJsonPayload = $"{{\"snapshot\":\"{_snapshotName}\",\"description\":\"{_saveDescription}\",\"played_time_millis\":{_playedTimeMillis},\"progress_value\":{_progressValue}}}";
        _cloudSaveClient.WriteLocalSaveFile(_saveFilePath, Encoding.UTF8.GetBytes(sampleJsonPayload));

        // Note: PC CommitSnapshot uses "Commit update" (full replacement) behavior for metadata
        var commitParams = new CommitSnapshotParams
        {
            SnapshotName = _snapshotName,
            RevisionId = _currentRevisionId,
            FilePath = _saveFilePath,
            MetadataChange = new SnapshotMetadataChange
            {
                Description = _saveDescription,
                PlayedTimeMillis = _playedTimeMillis,
                ProgressValue = _progressValue
            }
        };

        var result = await _cloudSaveClient.CommitSnapshotAsync(commitParams);

        if (result.IsOk)
        {
            // Retain the newly committed revision_id for subsequent saves in the same session
            _currentRevisionId = result.Value.RevisionId;
            var meta = result.Value.Metadata;
            Log($"<color=green>CommitSnapshot succeeded! New revision_id=\"{_currentRevisionId}\"</color>");
            Log($"  Confirmed Metadata: desc=\"{meta.Description}\", played={meta.PlayedTimeMillis}ms, progress={meta.ProgressValue}");
        }
        else
        {
            Log($"<color=red>CommitSnapshot failed! Error: {result.Code}, Message: {result.ErrorMessage}</color>");
        }

        if (_commitSnapshotButton != null) _commitSnapshotButton.interactable = !string.IsNullOrEmpty(_currentRevisionId);
    }

    private async void DeleteSnapshot()
    {
        Log($"Cloud Save: Deleting snapshot '{_snapshotName}'...");
        if (_deleteSnapshotButton != null) _deleteSnapshotButton.interactable = false;

        var deleteParams = new DeleteSnapshotParams { SnapshotName = _snapshotName };
        var result = await _cloudSaveClient.DeleteSnapshotAsync(deleteParams);

        if (result.IsOk)
        {
            _currentRevisionId = null;
            if (_commitSnapshotButton != null) _commitSnapshotButton.interactable = false;
            Log($"<color=green>DeleteSnapshot succeeded! Status: {result.Value.Status}</color>");
        }
        else
        {
            Log($"<color=red>DeleteSnapshot failed! Error: {result.Code}, Message: {result.ErrorMessage}</color>");
        }

        if (_deleteSnapshotButton != null) _deleteSnapshotButton.interactable = true;
    }

    private void SetCloudSaveButtonsInteractable(bool sdkReady, bool hasOpenRevision)
    {
        if (_listSnapshotsButton != null) _listSnapshotsButton.interactable = sdkReady;
        if (_openSnapshotButton != null) _openSnapshotButton.interactable = sdkReady;
        if (_commitSnapshotButton != null) _commitSnapshotButton.interactable = sdkReady && hasOpenRevision;
        if (_deleteSnapshotButton != null) _deleteSnapshotButton.interactable = sdkReady;
    }

    private void EnsureCloudSaveButtonsExist()
    {
        if (_initializeButton == null || _initializeButton.transform.parent == null)
        {
            return;
        }

        Transform panel = _initializeButton.transform.parent;
        if (_listSnapshotsButton == null)
            _listSnapshotsButton = CreateButtonClone(panel, "ListSnapshotsButton", "List Snapshots", new Vector2(0f, -45f));
        if (_openSnapshotButton == null)
            _openSnapshotButton = CreateButtonClone(panel, "OpenSnapshotButton", "Open Snapshot", new Vector2(180f, -45f));
        if (_commitSnapshotButton == null)
            _commitSnapshotButton = CreateButtonClone(panel, "CommitSnapshotButton", "Commit Snapshot", new Vector2(360f, -45f));
        if (_deleteSnapshotButton == null)
            _deleteSnapshotButton = CreateButtonClone(panel, "DeleteSnapshotButton", "Delete Snapshot", new Vector2(540f, -45f));
    }

    private Button CreateButtonClone(Transform parent, string objectName, string labelText, Vector2 anchoredOffset)
    {
        GameObject clone = Instantiate(_initializeButton.gameObject, parent);
        clone.name = objectName;
        RectTransform rt = clone.GetComponent<RectTransform>();
        if (rt != null)
        {
            rt.anchorMin = new Vector2(0f, 1f);
            rt.anchorMax = new Vector2(0f, 1f);
            rt.pivot = new Vector2(0f, 0f);
            rt.anchoredPosition = anchoredOffset;
        }

        TextMeshProUGUI tmp = clone.GetComponentInChildren<TextMeshProUGUI>();
        if (tmp != null)
        {
            tmp.text = labelText;
        }

        Button btn = clone.GetComponent<Button>();
        if (btn != null)
        {
            btn.onClick.RemoveAllListeners();
        }
        return btn;
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
        _cloudSaveClient?.Dispose();
    }
}
