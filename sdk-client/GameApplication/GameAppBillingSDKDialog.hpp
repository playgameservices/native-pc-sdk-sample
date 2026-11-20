#ifndef GAME_APP_BILLING_SDK_DIALOG_HPP_
#define GAME_APP_BILLING_SDK_DIALOG_HPP_

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <format>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <billing/client.h>
#include <billing/enums.h>
#include <billing/models.h>
#include <initialization/enums.h>
#include <initialization/initialization.h>
#include <initialization/models.h>
#include <integrity/client.h>
#include <integrity/enums.h>
#include <integrity/models.h>
#include <restclient-cpp/connection.h>
#include <restclient-cpp/restclient.h>

#include "GameAppGenericClasses.hpp"
#include "resource.h"

namespace GameApp {

using ::google::play::billing::AcknowledgePurchaseParams;
using ::google::play::billing::AcknowledgePurchaseResult;
using ::google::play::billing::AcknowledgePurchaseResultValue;
using ::google::play::billing::BillingClient;
using ::google::play::billing::BillingClientParams;
using ::google::play::billing::BillingError;
using ::google::play::billing::BillingProgram;
using ::google::play::billing::ConsumePurchaseParams;
using ::google::play::billing::ConsumePurchaseResult;
using ::google::play::billing::ConsumePurchaseResultValue;
using ::google::play::billing::CreateBillingProgramReportingDetailsParams;
using ::google::play::billing::CreateBillingProgramReportingDetailsResult;
using ::google::play::billing::IsBillingProgramAvailableParams;
using ::google::play::billing::IsBillingProgramAvailableResult;
using ::google::play::billing::LaunchPurchaseFlowParams;
using ::google::play::billing::LaunchPurchaseFlowResult;
using ::google::play::billing::LaunchPurchaseFlowResultValue;
using ::google::play::billing::ProductId;
using ::google::play::billing::ProductOffer;
using ::google::play::billing::ProductPurchaseDetails;
using ::google::play::billing::ProductType;
using ::google::play::billing::PurchaseState;
using ::google::play::billing::QueryProductDetailsParams;
using ::google::play::billing::QueryProductDetailsResult;
using ::google::play::billing::QueryProductDetailsResultValue;
using ::google::play::billing::QueryPurchasesResult;
using ::google::play::initialization::Environment;
using ::google::play::initialization::FormFactor;
using ::google::play::initialization::GooglePlayInitialize;
using ::google::play::initialization::GooglePlayInitializeParams;
using ::google::play::initialization::InitializationError;
using ::google::play::initialization::InitializeResult;
using ::google::play::integrity::IntegrityClient;
using ::google::play::integrity::PrepareIntegrityTokenParams;
using ::google::play::integrity::PrepareIntegrityTokenResult;
using ::google::play::integrity::PrepareIntegrityTokenResultValue;
using ::google::play::integrity::RequestIntegrityTokenParams;
using ::google::play::integrity::RequestIntegrityTokenResult;
using ::google::play::integrity::RequestIntegrityTokenResultValue;

// Unified operation sequence shared with sdk-client-unity:
// 1..8: Core Billing SDK + Purchase Restoration
// 9..11: Play Integrity API
static const std::wstring kListBoxItems[NUM_LIST_ITEMS] = {
    L"Initialize Billing SDK",
    L"Query Product Details",
    L"IsBillingProgramAvailable",
    L"CreateBillingProgramReportingDetails",
    L"Launch Purchase Flow",
    L"Query Purchases",
    L"Acknowledge Purchase",
    L"Consume Purchase",
    L"Prepare Integrity Token",
    L"Request Integrity Token",
    L"Verify Integrity Token",
};

class GameAppBillingSdkDialog {
 public:
  explicit GameAppBillingSdkDialog(HINSTANCE hInstance)
      : m_hWnd(nullptr),
        m_hInstance(hInstance),
        mUiState(std::make_shared<AsyncUiState>()) {
    appEvents[BillingOps::kSdkInit] = {
        [this]() { OnSelectedInitializeSdk(); },
        [this]() { OnClickInitializeSdk(); }};
    appEvents[BillingOps::kQueryProductDetails] = {
        [this]() { OnSelectedQueryProductDetails(); },
        [this]() { OnClickQueryProductDetails(); }};
    appEvents[BillingOps::kIsBillingProgramAvailable] = {
        [this]() { OnSelectedIsBillingProgramAvailable(); },
        [this]() { OnClickIsBillingProgramAvailable(); }};
    appEvents[BillingOps::kCreateBillingProgramReportingDetails] = {
        [this]() { OnSelectedCreateBillingProgramReportingDetails(); },
        [this]() { OnClickCreateBillingProgramReportingDetails(); }};
    appEvents[BillingOps::kLaunchPurchaseFlow] = {
        [this]() { OnSelectedLaunchPurchaseFlow(); },
        [this]() { OnClickLaunchPurchaseFlow(); }};
    appEvents[BillingOps::kQueryPurchases] = {
        [this]() { OnSelectedQueryPurchases(); },
        [this]() { OnClickQueryPurchases(); }};
    appEvents[BillingOps::kAcknowledgePurchase] = {
        [this]() { OnSelectedAcknowledgePurchase(); },
        [this]() { OnClickAcknowledgePurchase(); }};
    appEvents[BillingOps::kConsumePurchase] = {
        [this]() { OnSelectedConsumePurchase(); },
        [this]() { OnClickConsumePurchase(); }};
    appEvents[BillingOps::kPrepareIntegrityToken] = {
        [this]() { OnSelectedPrepareIntegrityToken(); },
        [this]() { OnClickPrepareIntegrityToken(); }};
    appEvents[BillingOps::kRequestIntegrityToken] = {
        [this]() { OnSelectedRequestIntegrityToken(); },
        [this]() { OnClickRequestIntegrityToken(); }};
    appEvents[BillingOps::kVerifyIntegrityToken] = {
        [this]() { OnSelectedVerifyIntegrityToken(); },
        [this]() { OnClickVerifyIntegrityToken(); }};
  }

  ~GameAppBillingSdkDialog() { InvalidateUiState(); }

  GameAppBillingSdkDialog(const GameAppBillingSdkDialog&) = delete;
  GameAppBillingSdkDialog& operator=(const GameAppBillingSdkDialog&) = delete;

  void ShowDialog() {
    DialogBoxParam(m_hInstance, MAKEINTRESOURCE(IDD_SDK), nullptr, DialogProc,
                   reinterpret_cast<LPARAM>(this));
  }

 private:
  struct AsyncUiState {
    std::mutex mtx;
    HWND hWnd = nullptr;
    bool dialogAlive = false;
    std::function<void()> pendingCallback;
  };

  struct EventPair {
    std::function<void()> onSelect;
    std::function<void()> onClick;
  };

  enum BillingOps {
    kSdkInit = 0,
    kQueryProductDetails,
    kIsBillingProgramAvailable,
    kCreateBillingProgramReportingDetails,
    kLaunchPurchaseFlow,
    kQueryPurchases,
    kAcknowledgePurchase,
    kConsumePurchase,
    kPrepareIntegrityToken,
    kRequestIntegrityToken,
    kVerifyIntegrityToken,
    Count
  };
  static_assert(BillingOps::Count == NUM_LIST_ITEMS,
                "BillingOps::Count must match NUM_LIST_ITEMS");

  HWND m_hWnd = nullptr;
  HINSTANCE m_hInstance = nullptr;
  int m_nSelectedOperation = -1;
  bool m_bOperationInFlight = false;

  HWND hComboBox1 = nullptr;
  HWND hComboBox2 = nullptr;
  HWND hComboBox3 = nullptr;
  HWND hParamLabel1 = nullptr;
  HWND hParamLabel2 = nullptr;
  HWND hParamLabel3 = nullptr;
  HWND hParamStatus = nullptr;
  HWND hResult = nullptr;
  HWND hRunButton = nullptr;

  std::shared_ptr<AsyncUiState> mUiState;

  // SDK clients managed via shared_ptr so background worker threads keep the
  // native handle alive even if the user closes the modal dialog mid-operation.
  std::shared_ptr<BillingClient> mBillingClient;
  std::shared_ptr<IntegrityClient> mIntegrityClient;

  // Cached Billing state
  QueryProductDetailsResultValue mQueryProdResValue;
  std::vector<std::string> mAvailableOfferTokens;
  std::vector<ProductPurchaseDetails> mKnownPurchases;
  std::string mLastExternalTransactionToken;

  // Cached Play Integrity state
  std::optional<PrepareIntegrityTokenResultValue> mPrepareIntegrityTokenResValue;
  std::optional<RequestIntegrityTokenResultValue> mRequestIntegrityTokenResValue;
  std::string mCachedRequestHash;
  bool mSdkInitialized = false;

  Manifest manifest;
  EventPair appEvents[BillingOps::Count];

  const std::map<BillingError, std::string> sdkErrorMsg = {
      {BillingError::kOk, "The operation completed successfully."},
      {BillingError::kUserCanceled, "Transaction was canceled by the user."},
      {BillingError::kServiceUnavailable,
       "The service is currently unavailable."},
      {BillingError::kBillingUnavailable,
       "A user billing error occurred during processing."},
      {BillingError::kItemUnavailable,
       "The requested product is not available for purchase."},
      {BillingError::kDeveloperError,
       "Error resulting from incorrect usage of the API."},
      {BillingError::kError, "Fatal error during the API action."},
      {BillingError::kItemAlreadyOwned,
       "The purchase failed because the item is already owned."},
      {BillingError::kItemNotOwned,
       "Requested action failed because the item is not owned by the user."},
      {BillingError::kNetworkError,
       "A network error occurred during the operation."},
      {BillingError::kServiceDisconnected,
       "The app is not connected to the Google Play Billing Library."},
  };

  void InvalidateUiState() {
    if (!mUiState) return;
    std::lock_guard<std::mutex> lock(mUiState->mtx);
    mUiState->dialogAlive = false;
    mUiState->hWnd = nullptr;
    mUiState->pendingCallback = nullptr;
  }

  // Ensures an async SDK continuation settles its std::promise at most once,
  // preventing std::future_error if a callback were ever invoked more than once.
  template <typename T>
  static void SetPromiseOnce(
      const std::shared_ptr<std::promise<T>>& promise,
      const std::shared_ptr<std::atomic_bool>& settled,
      T&& value) {
    bool expected = false;
    if (settled && settled->compare_exchange_strong(expected, true)) {
      promise->set_value(std::move(value));
    }
  }

  std::string GetBillingErrorDescription(BillingError err) const {
    auto it = sdkErrorMsg.find(err);
    if (it != sdkErrorMsg.end()) {
      return it->second;
    }
    return "Unrecognized billing error code.";
  }

  static std::string EnvironmentToString(Environment env) {
    switch (env) {
      case Environment::kWindows:
        return "Windows";
      case Environment::kAndroid:
        return "Android";
      case Environment::kUnknown:
      default:
        return "Unknown";
    }
  }

  static std::string FormFactorToString(FormFactor ff) {
    switch (ff) {
      case FormFactor::kDesktop:
        return "Desktop";
      case FormFactor::kMobile:
        return "Mobile";
      case FormFactor::kUnknown:
      default:
        return "Unknown";
    }
  }

  static std::string PurchaseStateToString(PurchaseState state) {
    switch (state) {
      case PurchaseState::kPurchaseStatePurchased:
        return "PURCHASED (1)";
      case PurchaseState::kPurchaseStatePending:
        return "PENDING (2)";
      case PurchaseState::kPurchaseStateUnspecified:
      default:
        return "UNSPECIFIED (0)";
    }
  }

  static std::string BillingProgramToString(BillingProgram prog) {
    switch (prog) {
      case BillingProgram::kBillingProgramAlternativeBillingOnly:
        return "1 - kBillingProgramAlternativeBillingOnly";
      case BillingProgram::kBillingProgramUserChoice:
        return "2 - kBillingProgramUserChoice";
      default:
        return "Unknown BillingProgram";
    }
  }

  void UpsertKnownPurchase(const ProductPurchaseDetails& detail) {
    if (detail.purchase_token.empty()) return;
    for (auto& existing : mKnownPurchases) {
      if (existing.purchase_token == detail.purchase_token) {
        existing = detail;
        return;
      }
    }
    mKnownPurchases.push_back(detail);
  }

  void RemoveKnownPurchase(const std::string& token) {
    mKnownPurchases.erase(
        std::remove_if(mKnownPurchases.begin(), mKnownPurchases.end(),
                       [&token](const ProductPurchaseDetails& p) {
                         return p.purchase_token == token;
                       }),
        mKnownPurchases.end());
  }

  void SetResultText(const std::string& utf8Message) {
    if (hResult != nullptr) {
      SetWindowTextW(hResult, stringToWstring(utf8Message).c_str());
    }
  }

  // Dispatches blocking SDK / HTTP work onto a background thread and posts the
  // UI completion callback back onto the Win32 dialog message loop so the
  // window never freezes (e.g. during the 5-minute LaunchPurchaseFlow window).
  void RunAsyncOperation(std::function<std::function<void()>()> backgroundWork) {
    if (m_bOperationInFlight) {
      SetResultText("An operation is already in progress. Please wait...");
      return;
    }
    m_bOperationInFlight = true;
    if (hRunButton != nullptr) {
      EnableWindow(hRunButton, FALSE);
    }

    auto uiState = mUiState;
    std::thread([this, uiState, work = std::move(backgroundWork)]() {
      std::function<void()> uiCompletion;
      try {
        uiCompletion = work();
      } catch (const std::exception& ex) {
        std::string errMsg =
            std::string("Unhandled exception in async worker: ") + ex.what();
        GameLogging::Error(errMsg);
        uiCompletion = [this, errMsg]() { SetResultText(errMsg); };
      }

      std::lock_guard<std::mutex> lock(uiState->mtx);
      if (uiState->dialogAlive && uiState->hWnd != nullptr) {
        uiState->pendingCallback = std::move(uiCompletion);
        PostMessageW(uiState->hWnd, WM_APP_SDK_RESULT, 0, 0);
      }
    }).detach();
  }

  void OnAsyncResultMessage() {
    std::function<void()> callback;
    {
      std::lock_guard<std::mutex> lock(mUiState->mtx);
      callback = std::move(mUiState->pendingCallback);
      mUiState->pendingCallback = nullptr;
    }
    m_bOperationInFlight = false;
    if (hRunButton != nullptr) {
      EnableWindow(hRunButton, TRUE);
    }
    if (callback) {
      callback();
    }
  }

  static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam) {
    GameAppBillingSdkDialog* pThis = reinterpret_cast<GameAppBillingSdkDialog*>(
        GetWindowLongPtr(hwndDlg, GWLP_USERDATA));
    switch (uMsg) {
      case WM_INITDIALOG: {
        pThis = reinterpret_cast<GameAppBillingSdkDialog*>(lParam);
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA,
                         reinterpret_cast<LONG_PTR>(pThis));
        if (pThis != nullptr) {
          HWND hListBox = GetDlgItem(hwndDlg, IDC_ListOperation);
          HFONT hFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
          SendMessageW(hListBox, WM_SETFONT, reinterpret_cast<WPARAM>(hFont),
                       TRUE);
          for (int i = 0; i < NUM_LIST_ITEMS; ++i) {
            SendMessageW(hListBox, LB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(kListBoxItems[i].c_str()));
          }
          pThis->OnInitDialog(hwndDlg);
        }
        return TRUE;
      }

      case WM_APP_SDK_RESULT: {
        if (pThis != nullptr) {
          pThis->OnAsyncResultMessage();
        }
        return TRUE;
      }

      case WM_COMMAND: {
        WORD notificationCode = HIWORD(wParam);
        if (pThis != nullptr) {
          if (LOWORD(wParam) == IDC_Run) {
            pThis->OnRunButtonClicked();
            return TRUE;
          } else if (LOWORD(wParam) == IDC_ListOperation &&
                     notificationCode == LBN_SELCHANGE) {
            pThis->OnListItemSelected(hwndDlg);
            return TRUE;
          } else if (LOWORD(wParam) == IDCANCEL || LOWORD(wParam) == 2) {
            pThis->InvalidateUiState();
            EndDialog(hwndDlg, 0);
            return TRUE;
          }
        }
        break;
      }

      case WM_CLOSE: {
        if (pThis != nullptr) {
          pThis->InvalidateUiState();
        }
        EndDialog(hwndDlg, 0);
        return TRUE;
      }

      case WM_DESTROY: {
        if (pThis != nullptr) {
          pThis->InvalidateUiState();
        }
        return TRUE;
      }

      default:
        break;
    }
    return FALSE;
  }

  void HideLabelsComboBox(int numPar) {
    ShowWindow(hParamLabel1, SW_HIDE);
    ShowWindow(hParamLabel2, SW_HIDE);
    ShowWindow(hParamLabel3, SW_HIDE);
    ShowWindow(hComboBox1, SW_HIDE);
    ShowWindow(hComboBox2, SW_HIDE);
    ShowWindow(hComboBox3, SW_HIDE);
    ShowWindow(hParamStatus, SW_HIDE);

    SendMessageW(hComboBox1, CB_RESETCONTENT, 0, 0);
    SendMessageW(hComboBox2, CB_RESETCONTENT, 0, 0);
    SendMessageW(hComboBox3, CB_RESETCONTENT, 0, 0);

    if (numPar <= 0) {
      ShowWindow(hParamStatus, SW_SHOW);
      return;
    }
    if (numPar >= 1) {
      ShowWindow(hParamLabel1, SW_SHOW);
      ShowWindow(hComboBox1, SW_SHOW);
    }
    if (numPar >= 2) {
      ShowWindow(hParamLabel2, SW_SHOW);
      ShowWindow(hComboBox2, SW_SHOW);
    }
    if (numPar >= 3) {
      ShowWindow(hParamLabel3, SW_SHOW);
      ShowWindow(hComboBox3, SW_SHOW);
    }
  }

  void OnInitDialog(HWND hwndDlg) {
    m_hWnd = hwndDlg;
    {
      std::lock_guard<std::mutex> lock(mUiState->mtx);
      mUiState->hWnd = hwndDlg;
      mUiState->dialogAlive = true;
    }

    ParseXmlFile("manifest.xml", manifest);
    hComboBox1 = GetDlgItem(hwndDlg, IDC_Param1);
    hComboBox2 = GetDlgItem(hwndDlg, IDC_Param2);
    hComboBox3 = GetDlgItem(hwndDlg, IDC_Param3);
    hParamLabel1 = GetDlgItem(hwndDlg, IDC_Parameter1);
    hParamLabel2 = GetDlgItem(hwndDlg, IDC_Parameter2);
    hParamLabel3 = GetDlgItem(hwndDlg, IDC_Parameter3);
    hParamStatus = GetDlgItem(hwndDlg, IDC_ParamStatus);
    hResult = GetDlgItem(hwndDlg, IDC_Result);
    hRunButton = GetDlgItem(hwndDlg, IDC_Run);

    HFONT hFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    SendMessageW(hComboBox1, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    SendMessageW(hComboBox2, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    SendMessageW(hComboBox3, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    HideLabelsComboBox(0);
  }

  void OnListItemSelected(HWND hwndDlg) {
    if (m_bOperationInFlight) {
      return;
    }
    HWND hListBox = GetDlgItem(hwndDlg, IDC_ListOperation);
    SetResultText("");
    m_nSelectedOperation =
        static_cast<int>(SendMessageW(hListBox, LB_GETCURSEL, 0, 0));
    if (m_nSelectedOperation != LB_ERR &&
        m_nSelectedOperation >= 0 &&
        m_nSelectedOperation < BillingOps::Count) {
      wchar_t text[256] = {0};
      SendMessageW(hListBox, LB_GETTEXT,
                   static_cast<WPARAM>(m_nSelectedOperation),
                   reinterpret_cast<LPARAM>(text));
      SetWindowTextW(GetDlgItem(hwndDlg, IDC_Function), text);
      appEvents[m_nSelectedOperation].onSelect();
    }
  }

  void GetComboBoxText(HWND combo, int& index, std::string& ret) {
    index = static_cast<int>(SendMessageW(combo, CB_GETCURSEL, 0, 0));
    ret.clear();
    if (index != CB_ERR) {
      LRESULT len = SendMessageW(combo, CB_GETLBTEXTLEN, index, 0);
      if (len == CB_ERR || len < 0) return;
      std::wstring wstr(static_cast<size_t>(len) + 1, L'\0');
      SendMessageW(combo, CB_GETLBTEXT, index,
                   reinterpret_cast<LPARAM>(wstr.data()));
      wstr.resize(static_cast<size_t>(len));
      ret = wstringTostring(wstr);
    }
  }

  void OnRunButtonClicked() {
    try {
      if (m_nSelectedOperation < 0 ||
          m_nSelectedOperation >= BillingOps::Count) {
        std::string mess = "Warning: Select an operation first.";
        GameLogging::Warning(mess);
        SetResultText(mess);
        return;
      }
      appEvents[m_nSelectedOperation].onClick();
    } catch (const std::exception& e) {
      std::string errMsg =
          "Exception in " +
          wstringTostring(kListBoxItems[m_nSelectedOperation]) + ": " +
          e.what();
      GameLogging::Error(errMsg);
      SetResultText(errMsg);
    }
  }

  // ---------------------------------------------------------------------------
  // Selection Handlers (Populate UI Parameters)
  // ---------------------------------------------------------------------------

  void OnSelectedInitializeSdk() {
    GameLogging::Info("Selected Initialize Billing SDK");
    HideLabelsComboBox(1);
    SetWindowTextW(hParamLabel1, L"package_name:");
    std::string pkg = manifest.App.PackageName.empty()
                          ? "(from manifest.xml)"
                          : manifest.App.PackageName;
    SendMessageW(hComboBox1, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(stringToWstring(pkg).c_str()));
    SendMessageW(hComboBox1, CB_SETCURSEL, 0, 0);
  }

  void OnSelectedQueryProductDetails() {
    if (!mBillingClient) {
      SetResultText("Error: Initialize the Billing SDK first.");
      HideLabelsComboBox(0);
      return;
    }
    GameLogging::Info("Selected Query Product Details");
    HideLabelsComboBox(2);

    SetWindowTextW(hParamLabel1, L"Product ID:");
    if (!manifest.App.skuList.empty()) {
      for (const auto& skuItem : manifest.App.skuList) {
        std::wstring proId = stringToWstring(skuItem);
        SendMessageW(hComboBox1, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(proId.c_str()));
      }
    } else {
      std::wstring proId = stringToWstring(manifest.App.sku);
      SendMessageW(hComboBox1, CB_ADDSTRING, 0,
                   reinterpret_cast<LPARAM>(proId.c_str()));
    }
    SendMessageW(hComboBox1, CB_SETCURSEL, 0, 0);

    SetWindowTextW(hParamLabel2, L"Product Type:");
    SendMessageW(hComboBox2, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(L"1 - kTypeInApp"));
    SendMessageW(hComboBox2, CB_SETCURSEL, 0, 0);
  }

  void PopulateBillingProgramCombo() {
    HideLabelsComboBox(1);
    SetWindowTextW(hParamLabel1, L"billing_program:");
    SendMessageW(
        hComboBox1, CB_ADDSTRING, 0,
        reinterpret_cast<LPARAM>(L"1 - kBillingProgramAlternativeBillingOnly"));
    SendMessageW(hComboBox1, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(L"2 - kBillingProgramUserChoice"));
    SendMessageW(hComboBox1, CB_SETCURSEL, 0, 0);
  }

  void OnSelectedIsBillingProgramAvailable() {
    if (!mBillingClient) {
      SetResultText("Error: Initialize the Billing SDK first.");
      HideLabelsComboBox(0);
      return;
    }
    GameLogging::Info("Selected IsBillingProgramAvailable");
    PopulateBillingProgramCombo();
  }

  void OnSelectedCreateBillingProgramReportingDetails() {
    if (!mBillingClient) {
      SetResultText("Error: Initialize the Billing SDK first.");
      HideLabelsComboBox(0);
      return;
    }
    GameLogging::Info("Selected CreateBillingProgramReportingDetails");
    PopulateBillingProgramCombo();
  }

  void OnSelectedLaunchPurchaseFlow() {
    if (!mBillingClient) {
      SetResultText("Error: Initialize the Billing SDK first.");
      HideLabelsComboBox(0);
      return;
    }
    mAvailableOfferTokens.clear();
    for (const auto& prod : mQueryProdResValue.product_details) {
      for (const auto& offer : prod.product_offers) {
        if (!offer.offer_token.empty()) {
          mAvailableOfferTokens.push_back(offer.offer_token);
        }
      }
    }
    if (mAvailableOfferTokens.empty()) {
      SetResultText(
          "Error: No product offers available. Run 'Query Product Details' "
          "first and verify the SKU has an active offer.");
      HideLabelsComboBox(0);
      return;
    }

    GameLogging::Info("Selected Launch Purchase Flow");
    HideLabelsComboBox(1);
    SetWindowTextW(hParamLabel1, L"offer_token:");
    for (const auto& token : mAvailableOfferTokens) {
      std::wstring wToken = stringToWstring(token);
      SendMessageW(hComboBox1, CB_ADDSTRING, 0,
                   reinterpret_cast<LPARAM>(wToken.c_str()));
    }
    SendMessageW(hComboBox1, CB_SETCURSEL, 0, 0);
  }

  void OnSelectedQueryPurchases() {
    if (!mBillingClient) {
      SetResultText("Error: Initialize the Billing SDK first.");
      HideLabelsComboBox(0);
      return;
    }
    GameLogging::Info("Selected Query Purchases");
    HideLabelsComboBox(0);
  }

  void PopulatePurchaseTokensCombo(bool unacknowledgedOnly) {
    HideLabelsComboBox(1);
    SetWindowTextW(hParamLabel1, L"purchase_token:");
    int count = 0;
    for (const auto& purchase : mKnownPurchases) {
      if (purchase.purchase_token.empty()) continue;
      if (purchase.purchase_state != PurchaseState::kPurchaseStatePurchased) {
        continue;
      }
      if (unacknowledgedOnly && purchase.is_acknowledged) {
        continue;
      }
      std::wstring wToken = stringToWstring(purchase.purchase_token);
      SendMessageW(hComboBox1, CB_ADDSTRING, 0,
                   reinterpret_cast<LPARAM>(wToken.c_str()));
      ++count;
    }
    if (count > 0) {
      SendMessageW(hComboBox1, CB_SETCURSEL, 0, 0);
    } else {
      SetResultText(
          "No eligible PURCHASED tokens cached yet.\r\nRun 'Launch Purchase "
          "Flow' or 'Query Purchases' first.");
    }
  }

  void OnSelectedAcknowledgePurchase() {
    if (!mBillingClient) {
      SetResultText("Error: Initialize the Billing SDK first.");
      HideLabelsComboBox(0);
      return;
    }
    GameLogging::Info("Selected Acknowledge Purchase");
    PopulatePurchaseTokensCombo(/*unacknowledgedOnly=*/true);
  }

  void OnSelectedConsumePurchase() {
    if (!mBillingClient) {
      SetResultText("Error: Initialize the Billing SDK first.");
      HideLabelsComboBox(0);
      return;
    }
    GameLogging::Info("Selected Consume Purchase");
    PopulatePurchaseTokensCombo(/*unacknowledgedOnly=*/false);
  }

  void OnSelectedPrepareIntegrityToken() {
    GameLogging::Info("Selected Prepare Integrity Token");
    HideLabelsComboBox(1);
    SetWindowTextW(hParamLabel1, L"project_number:");
    std::wstring projStr =
        std::to_wstring(manifest.App.cloudProjectNumber);
    SendMessageW(hComboBox1, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(projStr.c_str()));
    SendMessageW(hComboBox1, CB_SETCURSEL, 0, 0);
  }

  void OnSelectedRequestIntegrityToken() {
    GameLogging::Info("Selected Request Integrity Token");
    HideLabelsComboBox(0);
    if (!mPrepareIntegrityTokenResValue.has_value()) {
      SetResultText(
          "Note: Run 'Prepare Integrity Token' before requesting a token.");
    }
  }

  void OnSelectedVerifyIntegrityToken() {
    GameLogging::Info("Selected Verify Integrity Token");
    HideLabelsComboBox(0);
    if (!mRequestIntegrityTokenResValue.has_value()) {
      SetResultText(
          "Note: Run 'Request Integrity Token' before verifying with backend.");
    }
  }

  // ---------------------------------------------------------------------------
  // Execution Handlers (Play Games PC SDK 26.9.261.0+)
  // ---------------------------------------------------------------------------

  // 1. Initialize Billing SDK
  void OnClickInitializeSdk() {
    SetResultText("Initializing Play Games PC SDK...");
    std::string pkgName = manifest.App.PackageName;

    RunAsyncOperation([this, pkgName]() -> std::function<void()> {
      auto promise = std::make_shared<std::promise<InitializeResult>>();
      auto settled = std::make_shared<std::atomic_bool>(false);
      GooglePlayInitializeParams initParams;
      if (!pkgName.empty() && pkgName != "your-package-id") {
        initParams.package_name = pkgName;
      }

      GooglePlayInitialize(
          [promise, settled](InitializeResult result) {
            SetPromiseOnce(promise, settled, std::move(result));
          },
          initParams);

      InitializeResult initResult = promise->get_future().get();

      return [this, result = std::move(initResult)]() {
        if (!result.ok()) {
          std::string errorDetail;
          bool mustExitProcess = false;

          switch (result.code()) {
            case InitializationError::kActionRequiredShutdownClientProcess:
              errorDetail =
                  "kActionRequiredShutdownClientProcess (2): Game must be "
                  "launched via Google Play Games client. Exiting process now "
                  "so Google Play Games can relaunch safely.";
              mustExitProcess = true;
              break;
            case InitializationError::kSdkRuntimeUpdateRequired:
              errorDetail =
                  "kSdkRuntimeUpdateRequired (3): Please update Google Play "
                  "Games on PC and the SDK runtime.";
              break;
            case InitializationError::kSdkRuntimeUnavailable:
              errorDetail =
                  "kSdkRuntimeUnavailable (4): Google Play Games SDK runtime "
                  "service is unavailable or not running.";
              break;
            case InitializationError::kSdkRuntimeNotInstalled:
              errorDetail =
                  "kSdkRuntimeNotInstalled (5): Google Play Games on PC is not "
                  "installed on this machine.";
              break;
            case InitializationError::kError:
            default:
              errorDetail = std::format(
                  "Initialization failed with code {}: {}",
                  static_cast<int>(result.code()), result.error_message());
              break;
          }

          GameLogging::Error(errorDetail);
          SetResultText(errorDetail);
          MessageBoxW(m_hWnd, stringToWstring(errorDetail).c_str(),
                      L"SDK Initialization Error", MB_OK | MB_ICONERROR);

          if (mustExitProcess) {
            InvalidateUiState();
            EndDialog(m_hWnd, 1);
            PostQuitMessage(1);
          }
          return;
        }

        mSdkInitialized = true;
        BillingClientParams clientParams;
        clientParams.enable_pending_purchases = true;
        mBillingClient = std::make_shared<BillingClient>(clientParams);

        if (!mIntegrityClient) {
          mIntegrityClient = std::make_shared<IntegrityClient>();
        }

        const auto& val = result.value();
        std::ostringstream msg;
        msg << "SDK Initialization completed successfully.\r\n"
            << "Environment: " << EnvironmentToString(val.environment) << "\r\n"
            << "FormFactor: " << FormFactorToString(val.form_factor) << "\r\n"
            << "BillingClient & IntegrityClient created.";
        GameLogging::Info(msg.str());
        SetResultText(msg.str());
        SetForegroundWindow(m_hWnd);
      };
    });
  }

  // 2. Query Product Details
  void OnClickQueryProductDetails() {
    if (!mBillingClient) {
      SetResultText("Error: SDK not initialized.");
      return;
    }

    int nProdIdx = 0;
    int nProdTypeIdx = 0;
    std::string sProdID;
    std::string sProdType;
    GetComboBoxText(hComboBox1, nProdIdx, sProdID);
    GetComboBoxText(hComboBox2, nProdTypeIdx, sProdType);

    if (sProdID.empty()) {
      SetResultText("Error: Product ID is empty. Check <SKU> in manifest.xml.");
      return;
    }

    ProductId prodId;
    prodId.id = sProdID;
    prodId.product_type = ProductType::kTypeInApp;

    QueryProductDetailsParams queryParams;
    queryParams.product_ids.push_back(prodId);

    SetResultText("Querying product details for '" + sProdID + "'...");
    GameLogging::Info(std::format(
        "QueryProductDetails: product_id='{}', product_type=kTypeInApp(1)",
        sProdID));

    auto client = mBillingClient;
    RunAsyncOperation(
        [this, client, queryParams, sProdID]() -> std::function<void()> {
          auto promise =
              std::make_shared<std::promise<QueryProductDetailsResult>>();
          auto settled = std::make_shared<std::atomic_bool>(false);
          client->QueryProductDetails(
              queryParams, [promise, settled](QueryProductDetailsResult r) {
                SetPromiseOnce(promise, settled, std::move(r));
              });
          QueryProductDetailsResult res = promise->get_future().get();

          return [this, sProdID, result = std::move(res)]() {
            if (!result.ok()) {
              std::ostringstream err;
              err << "QueryProductDetails failed for '" << sProdID << "'\r\n"
                  << "Code: " << static_cast<int>(result.code()) << " ("
                  << GetBillingErrorDescription(result.code()) << ")\r\n"
                  << "Message: " << result.error_message();
              GameLogging::Error(err.str());
              SetResultText(err.str());
              return;
            }

            mQueryProdResValue = result.value();
            std::ostringstream out;
            out << "QueryProductDetails succeeded ("
                << mQueryProdResValue.product_details.size()
                << " product(s)).";

            if (mQueryProdResValue.product_details.empty()) {
              out << "\r\nNo product details found for SKU '" << sProdID
                  << "' in package '" << manifest.App.PackageName << "'.";
            }

            for (const auto& detail : mQueryProdResValue.product_details) {
              out << "\r\nProduct:"
                  << "\r\n  id: " << detail.id
                  << "\r\n  title: " << detail.title
                  << "\r\n  name: " << detail.name
                  << "\r\n  description: " << detail.description;

              for (const auto& offer : detail.product_offers) {
                double priceUnits =
                    static_cast<double>(offer.price_micros) / 1000000.0;
                out << "\r\n  Offer: " << offer.formatted_price << " ("
                    << std::format("{:.2f}", priceUnits) << " "
                    << offer.currency_code << ")";
                if (offer.full_price_micros > 0) {
                  double fullUnits =
                      static_cast<double>(offer.full_price_micros) / 1000000.0;
                  out << " [Full Price: "
                      << std::format("{:.2f}", fullUnits) << " "
                      << offer.currency_code << "]";
                }
                out << "\r\n    offer_token: " << offer.offer_token;
              }
            }

            GameLogging::Info(out.str());
            SetResultText(out.str());
            SetForegroundWindow(m_hWnd);
          };
        });
  }

  // Helper to resolve selected BillingProgram from ComboBox index (0 -> 1, 1 -> 2)
  bool ReadSelectedBillingProgram(BillingProgram& outProgram) {
    int selIndex = -1;
    std::string selText;
    GetComboBoxText(hComboBox1, selIndex, selText);
    if (selIndex == 0) {
      outProgram = BillingProgram::kBillingProgramAlternativeBillingOnly;
      return true;
    }
    if (selIndex == 1) {
      outProgram = BillingProgram::kBillingProgramUserChoice;
      return true;
    }
    SetResultText("Error: Please select a valid BillingProgram (1 or 2).");
    return false;
  }

  // 3. IsBillingProgramAvailable
  void OnClickIsBillingProgramAvailable() {
    if (!mBillingClient) {
      SetResultText("Error: SDK not initialized.");
      return;
    }

    BillingProgram program{};
    if (!ReadSelectedBillingProgram(program)) {
      return;
    }

    IsBillingProgramAvailableParams param{program};
    SetResultText("Checking IsBillingProgramAvailable for " +
                  BillingProgramToString(program) + "...");

    auto client = mBillingClient;
    RunAsyncOperation(
        [this, client, param, program]() -> std::function<void()> {
          auto promise =
              std::make_shared<std::promise<IsBillingProgramAvailableResult>>();
          auto settled = std::make_shared<std::atomic_bool>(false);
          client->IsBillingProgramAvailable(
              param, [promise, settled](IsBillingProgramAvailableResult r) {
                SetPromiseOnce(promise, settled, std::move(r));
              });
          IsBillingProgramAvailableResult res = promise->get_future().get();

          return [this, program, result = std::move(res)]() {
            std::ostringstream msg;
            if (!result.ok()) {
              msg << "IsBillingProgramAvailable ("
                  << BillingProgramToString(program) << ") -> NOT AVAILABLE\r\n"
                  << "Code: " << static_cast<int>(result.code()) << " ("
                  << GetBillingErrorDescription(result.code()) << ")\r\n"
                  << "Message: " << result.error_message();
              GameLogging::Warning(msg.str());
            } else {
              msg << "IsBillingProgramAvailable ("
                  << BillingProgramToString(program)
                  << ") -> AVAILABLE and eligible.";
              GameLogging::Info(msg.str());
            }
            SetResultText(msg.str());
          };
        });
  }

  // 4. CreateBillingProgramReportingDetails
  void OnClickCreateBillingProgramReportingDetails() {
    if (!mBillingClient) {
      SetResultText("Error: SDK not initialized.");
      return;
    }

    BillingProgram program{};
    if (!ReadSelectedBillingProgram(program)) {
      return;
    }

    CreateBillingProgramReportingDetailsParams params{program};
    SetResultText("Creating billing program reporting details for " +
                  BillingProgramToString(program) + "...");

    auto client = mBillingClient;
    RunAsyncOperation([this, client, params, program]() -> std::function<void()> {
      auto promise = std::make_shared<
          std::promise<CreateBillingProgramReportingDetailsResult>>();
      auto settled = std::make_shared<std::atomic_bool>(false);
      client->CreateBillingProgramReportingDetails(
          params,
          [promise, settled](CreateBillingProgramReportingDetailsResult r) {
            SetPromiseOnce(promise, settled, std::move(r));
          });
      CreateBillingProgramReportingDetailsResult res =
          promise->get_future().get();

      return [this, program, result = std::move(res)]() {
        std::ostringstream msg;
        if (!result.ok()) {
          msg << "CreateBillingProgramReportingDetails ("
              << BillingProgramToString(program) << ") failed.\r\n"
              << "Code: " << static_cast<int>(result.code()) << " ("
              << GetBillingErrorDescription(result.code()) << ")\r\n"
              << "Message: " << result.error_message();
          GameLogging::Error(msg.str());
        } else {
          mLastExternalTransactionToken =
              result.value().external_transaction_token;
          msg << "CreateBillingProgramReportingDetails succeeded!\r\n"
              << "Program: " << BillingProgramToString(program) << "\r\n"
              << "external_transaction_token: "
              << RedactTokenForLog(mLastExternalTransactionToken);
          GameLogging::Info(msg.str());
        }
        SetResultText(msg.str());
      };
    });
  }

  // 5. Launch Purchase Flow
  void OnClickLaunchPurchaseFlow() {
    if (!mBillingClient) {
      SetResultText("Error: SDK not initialized.");
      return;
    }

    int selIdx = -1;
    std::string sOfferToken;
    GetComboBoxText(hComboBox1, selIdx, sOfferToken);
    if (sOfferToken.empty()) {
      SetResultText(
          "Error: Missing offer_token. Run 'Query Product Details' first.");
      return;
    }

    LaunchPurchaseFlowParams flowParams;
    flowParams.offer_token = sOfferToken;
    flowParams.quantity = 1;

    SetResultText(
        "Launching Google Play purchase overlay...\r\n(Complete or cancel "
        "checkout in the Google Play Games window.)");
    GameLogging::Info("LaunchPurchaseFlow started");

    auto client = mBillingClient;
    RunAsyncOperation([this, client, flowParams]() -> std::function<void()> {
      auto promise = std::make_shared<std::promise<LaunchPurchaseFlowResult>>();
      auto settled = std::make_shared<std::atomic_bool>(false);
      client->LaunchPurchaseFlow(flowParams,
                                 [promise, settled](LaunchPurchaseFlowResult r) {
                                   SetPromiseOnce(promise, settled, std::move(r));
                                 });
      LaunchPurchaseFlowResult res = promise->get_future().get();

      return [this, result = std::move(res)]() {
        if (!result.ok()) {
          std::ostringstream err;
          err << "LaunchPurchaseFlow failed.\r\n"
              << "Code: " << static_cast<int>(result.code()) << " ("
              << GetBillingErrorDescription(result.code()) << ")\r\n"
              << "Message: " << result.error_message();
          GameLogging::Error(err.str());
          SetResultText(err.str());
          SetForegroundWindow(m_hWnd);
          return;
        }

        const ProductPurchaseDetails& purchase =
            result.value().product_purchase_details;
        UpsertKnownPurchase(purchase);

        std::ostringstream out;
        out << "LaunchPurchaseFlow completed!\r\n"
            << "  product_id: " << purchase.product_id << "\r\n"
            << "  order_id: "
            << (purchase.order_id.empty() ? "(pending)" : purchase.order_id)
            << "\r\n"
            << "  purchase_state: "
            << PurchaseStateToString(purchase.purchase_state) << "\r\n"
            << "  quantity: " << purchase.quantity << "\r\n"
            << "  is_acknowledged: "
            << (purchase.is_acknowledged ? "true" : "false") << "\r\n"
            << "  purchase_token: " << RedactTokenForLog(purchase.purchase_token);

        if (purchase.purchase_state == PurchaseState::kPurchaseStatePending) {
          out << "\r\n\r\nNOTE: Purchase is PENDING. Do not grant entitlement "
                 "or acknowledge/consume until state becomes PURCHASED.";
        }

        GameLogging::Info(out.str());
        SetResultText(out.str());
        SetForegroundWindow(m_hWnd);
      };
    });
  }

  // 6. Query Purchases (Purchase Restoration)
  void OnClickQueryPurchases() {
    if (!mBillingClient) {
      SetResultText("Error: SDK not initialized.");
      return;
    }

    SetResultText("Querying existing purchases for current user...");
    GameLogging::Info("QueryPurchases started");

    auto client = mBillingClient;
    RunAsyncOperation([this, client]() -> std::function<void()> {
      auto promise = std::make_shared<std::promise<QueryPurchasesResult>>();
      auto settled = std::make_shared<std::atomic_bool>(false);
      client->QueryPurchases([promise, settled](QueryPurchasesResult r) {
        SetPromiseOnce(promise, settled, std::move(r));
      });
      QueryPurchasesResult res = promise->get_future().get();

      return [this, result = std::move(res)]() {
        if (!result.ok()) {
          std::ostringstream err;
          err << "QueryPurchases failed.\r\n"
              << "Code: " << static_cast<int>(result.code()) << " ("
              << GetBillingErrorDescription(result.code()) << ")\r\n"
              << "Message: " << result.error_message();
          GameLogging::Error(err.str());
          SetResultText(err.str());
          return;
        }

        const auto& purchases = result.value().product_purchase_details;
        mKnownPurchases = purchases;

        std::ostringstream out;
        out << "QueryPurchases succeeded: " << purchases.size()
            << " active purchase(s) returned.";
        for (size_t i = 0; i < purchases.size(); ++i) {
          const auto& p = purchases[i];
          out << "\r\n[" << (i + 1) << "] product_id: " << p.product_id
              << " | state: " << PurchaseStateToString(p.purchase_state)
              << " | ack: " << (p.is_acknowledged ? "true" : "false")
              << " | qty: " << p.quantity
              << "\r\n    order_id: " << p.order_id
              << "\r\n    token: " << RedactTokenForLog(p.purchase_token);
        }
        GameLogging::Info(out.str());
        SetResultText(out.str());
        SetForegroundWindow(m_hWnd);
      };
    });
  }

  // 7. Acknowledge Purchase
  void OnClickAcknowledgePurchase() {
    if (!mBillingClient) {
      SetResultText("Error: SDK not initialized.");
      return;
    }

    int selIdx = -1;
    std::string sPurchaseToken;
    GetComboBoxText(hComboBox1, selIdx, sPurchaseToken);
    if (sPurchaseToken.empty()) {
      SetResultText(
          "Error: Missing purchase_token. Complete a purchase or run 'Query "
          "Purchases' first.");
      return;
    }

    AcknowledgePurchaseParams ackParams{sPurchaseToken};
    SetResultText("Acknowledging purchase...");
    GameLogging::Info("AcknowledgePurchase started");

    auto client = mBillingClient;
    RunAsyncOperation(
        [this, client, ackParams, sPurchaseToken]() -> std::function<void()> {
          auto promise =
              std::make_shared<std::promise<AcknowledgePurchaseResult>>();
          auto settled = std::make_shared<std::atomic_bool>(false);
          client->AcknowledgePurchase(
              ackParams, [promise, settled](AcknowledgePurchaseResult r) {
                SetPromiseOnce(promise, settled, std::move(r));
              });
          AcknowledgePurchaseResult res = promise->get_future().get();

          return [this, sPurchaseToken, result = std::move(res)]() {
            if (!result.ok()) {
              std::ostringstream err;
              err << "AcknowledgePurchase failed.\r\n"
                  << "Code: " << static_cast<int>(result.code()) << " ("
                  << GetBillingErrorDescription(result.code()) << ")\r\n"
                  << "Message: " << result.error_message();
              GameLogging::Error(err.str());
              SetResultText(err.str());
              SetForegroundWindow(m_hWnd);
              return;
            }

            for (auto& p : mKnownPurchases) {
              if (p.purchase_token == sPurchaseToken) {
                p.is_acknowledged = true;
              }
            }

            std::string msg =
                "Successfully acknowledged purchase.\r\npurchase_token: " +
                RedactTokenForLog(sPurchaseToken);
            GameLogging::Info(msg);
            SetResultText(msg);
            SetForegroundWindow(m_hWnd);
          };
        });
  }

  // 8. Consume Purchase
  void OnClickConsumePurchase() {
    if (!mBillingClient) {
      SetResultText("Error: SDK not initialized.");
      return;
    }

    int selIdx = -1;
    std::string sPurchaseToken;
    GetComboBoxText(hComboBox1, selIdx, sPurchaseToken);
    if (sPurchaseToken.empty()) {
      SetResultText(
          "Error: Missing purchase_token. Complete a purchase or run 'Query "
          "Purchases' first.");
      return;
    }

    ConsumePurchaseParams consumeParams{sPurchaseToken};
    SetResultText("Consuming purchase...");
    GameLogging::Info("ConsumePurchase started");

    auto client = mBillingClient;
    RunAsyncOperation([this, client, consumeParams,
                       sPurchaseToken]() -> std::function<void()> {
      auto promise = std::make_shared<std::promise<ConsumePurchaseResult>>();
      auto settled = std::make_shared<std::atomic_bool>(false);
      client->ConsumePurchase(consumeParams,
                              [promise, settled](ConsumePurchaseResult r) {
                                SetPromiseOnce(promise, settled, std::move(r));
                              });
      ConsumePurchaseResult res = promise->get_future().get();

      return [this, sPurchaseToken, result = std::move(res)]() {
        if (!result.ok()) {
          std::ostringstream err;
          err << "ConsumePurchase failed.\r\n"
              << "Code: " << static_cast<int>(result.code()) << " ("
              << GetBillingErrorDescription(result.code()) << ")\r\n"
              << "Message: " << result.error_message();
          GameLogging::Error(err.str());
          SetResultText(err.str());
          SetForegroundWindow(m_hWnd);
          return;
        }

        RemoveKnownPurchase(sPurchaseToken);
        std::string msg =
            "Successfully consumed purchase.\r\npurchase_token: " +
            RedactTokenForLog(sPurchaseToken);
        GameLogging::Info(msg);
        SetResultText(msg);
        SetForegroundWindow(m_hWnd);
      };
    });
  }

  // 9. Prepare Integrity Token
  void OnClickPrepareIntegrityToken() {
    if (!mSdkInitialized) {
      SetResultText(
          "Error: Run 'Initialize Billing SDK' first to initialize the Play "
          "PC SDK runtime.");
      return;
    }
    if (!mIntegrityClient) {
      mIntegrityClient = std::make_shared<IntegrityClient>();
    }
    if (manifest.App.cloudProjectNumber <= 0) {
      SetResultText(
          "Error: Set a valid <CloudProjectNumber> in manifest.xml before "
          "calling PrepareIntegrityToken.");
      return;
    }

    PrepareIntegrityTokenParams param;
    param.cloud_project_number = manifest.App.cloudProjectNumber;

    SetResultText("Preparing Play Integrity token...");
    GameLogging::Info("PrepareIntegrityToken started");

    auto integrityClient = mIntegrityClient;
    RunAsyncOperation([this, integrityClient, param]() -> std::function<void()> {
      auto promise =
          std::make_shared<std::promise<PrepareIntegrityTokenResult>>();
      auto settled = std::make_shared<std::atomic_bool>(false);
      integrityClient->PrepareIntegrityToken(
          param, [promise, settled](PrepareIntegrityTokenResult r) {
            SetPromiseOnce(promise, settled, std::move(r));
          });
      PrepareIntegrityTokenResult res = promise->get_future().get();

      return [this, result = std::move(res)]() {
        if (!result.ok()) {
          std::string errMsg = std::format(
              "PrepareIntegrityToken failed (code {}): {}",
              static_cast<int>(result.code()), result.error_message());
          GameLogging::Error(errMsg);
          SetResultText(errMsg);
          return;
        }

        mPrepareIntegrityTokenResValue.emplace(result.value());
        std::string msg =
            "PrepareIntegrityToken completed successfully.\r\nWarm-up session "
            "ready.";
        GameLogging::Info(msg);
        SetResultText(msg);
      };
    });
  }

  // 10. Request Integrity Token
  void OnClickRequestIntegrityToken() {
    if (!mIntegrityClient || !mPrepareIntegrityTokenResValue.has_value()) {
      SetResultText(
          "Error: Call 'Prepare Integrity Token' successfully before "
          "requesting an integrity token.");
      return;
    }

    mCachedRequestHash = GenerateIntegrityRequestHash(
        manifest.App.PackageName, "verify_integrity");
    RequestIntegrityTokenParams param{
        mPrepareIntegrityTokenResValue->request_token_data, mCachedRequestHash};

    SetResultText("Requesting Play Integrity token with SHA-256 request_hash...");
    GameLogging::Info("RequestIntegrityToken started with request_hash=" +
                      mCachedRequestHash);

    auto integrityClient = mIntegrityClient;
    RunAsyncOperation([this, integrityClient, param]() -> std::function<void()> {
      auto promise =
          std::make_shared<std::promise<RequestIntegrityTokenResult>>();
      auto settled = std::make_shared<std::atomic_bool>(false);
      integrityClient->RequestIntegrityToken(
          param, [promise, settled](RequestIntegrityTokenResult r) {
            SetPromiseOnce(promise, settled, std::move(r));
          });
      RequestIntegrityTokenResult res = promise->get_future().get();

      return [this, result = std::move(res)]() {
        if (!result.ok()) {
          std::string errMsg = std::format(
              "RequestIntegrityToken failed (code {}): {}",
              static_cast<int>(result.code()), result.error_message());
          GameLogging::Error(errMsg);
          SetResultText(errMsg);
          return;
        }

        mRequestIntegrityTokenResValue.emplace(result.value());
        std::ostringstream out;
        out << "RequestIntegrityToken completed successfully.\r\n"
            << "SHA-256 request_hash: " << mCachedRequestHash << "\r\n"
            << "Token: "
            << RedactTokenForLog(mRequestIntegrityTokenResValue->token_bytes);
        GameLogging::Info(out.str());
        SetResultText(out.str());
      };
    });
  }

  // 11. Verify Integrity Token (via api-server)
  void OnClickVerifyIntegrityToken() {
    if (!mRequestIntegrityTokenResValue.has_value()) {
      SetResultText(
          "Error: Call 'Request Integrity Token' successfully before verifying "
          "with the backend server.");
      return;
    }

    nlohmann::json jsonPayload;
    jsonPayload["packageName"] = manifest.App.PackageName;
    jsonPayload["token"] = mRequestIntegrityTokenResValue->token_bytes;
    std::string payload = jsonPayload.dump();
    std::string baseUrl = manifest.App.backendBaseUrl;
    std::string url = baseUrl + "/api/verifyIntegrityToken";
    std::string expectedHash = mCachedRequestHash;
    std::string apiAuthToken = TrimAsciiWhitespace(GetEnvVarString("API_AUTH_TOKEN"));
    if (apiAuthToken.empty()) {
      apiAuthToken = manifest.App.apiAuthToken;
    }

    SetResultText("Verifying integrity token via " + url + "...");
    GameLogging::Info("POST " + url);

    RunAsyncOperation(
        [this, baseUrl, payload, expectedHash,
         apiAuthToken]() -> std::function<void()> {
          static std::once_flag restClientInitOnce;
          std::call_once(restClientInitOnce, []() { RestClient::init(); });

          RestClient::Connection conn(baseUrl);
          conn.AppendHeader("Content-Type", "application/json");
          if (!apiAuthToken.empty()) {
            conn.AppendHeader("Authorization", "Bearer " + apiAuthToken);
          }
          RestClient::Response response =
              conn.post("/api/verifyIntegrityToken", payload);

          return [this, response, expectedHash]() {
            if (response.code == 401) {
              std::string errMsg =
                  "Backend HTTP Error: 401 Unauthorized.\r\n"
                  "Set the API_AUTH_TOKEN environment variable (or "
                  "<ApiAuthToken> in manifest.local.xml) to a valid Firebase "
                  "Auth ID token.";
              GameLogging::Error(errMsg);
              SetResultText(errMsg);
              return;
            }
            if (response.code != 200) {
              std::string errMsg =
                  "Backend HTTP Error: " + std::to_string(response.code);
              GameLogging::Error(errMsg);
              SetResultText(errMsg);
              return;
            }

            try {
              auto resJson = nlohmann::json::parse(response.body);
              if (!resJson.value("result", false)) {
                std::string err = resJson.value("error", "Unknown server error");
                std::string msg = "Verification failed: " + err;
                GameLogging::Error(msg);
                SetResultText(msg);
                return;
              }

              std::string receivedHash = resJson.value("hash", "");
              if (receivedHash != expectedHash) {
                std::string msg =
                    "Verification failed: requestHash mismatch!\r\nExpected: " +
                    expectedHash + "\r\nReceived: " + receivedHash;
                GameLogging::Error(msg);
                SetResultText(msg);
                return;
              }

              std::string licensingVerdict = "UNSPECIFIED";
              if (resJson.contains("payload") &&
                  resJson["payload"].contains("tokenPayloadExternal") &&
                  resJson["payload"]["tokenPayloadExternal"].contains(
                      "accountDetails") &&
                  resJson["payload"]["tokenPayloadExternal"]["accountDetails"]
                      .contains("appLicensingVerdict")) {
                licensingVerdict =
                    resJson["payload"]["tokenPayloadExternal"]["accountDetails"]
                           ["appLicensingVerdict"]
                               .get<std::string>();
              }

              std::ostringstream out;
              if (licensingVerdict == "LICENSED") {
                out << "Verification successful!\r\n"
                    << "Device Verdict: MEETS_PC_INTEGRITY\r\n"
                    << "Licensing Verdict: LICENSED\r\n"
                    << "Matched SHA-256 requestHash: " << receivedHash;
                GameLogging::Info(out.str());
              } else {
                out << "Device & requestHash verified, but appLicensingVerdict="
                    << licensingVerdict;
                GameLogging::Warning(out.str());
              }
              SetResultText(out.str());
            } catch (const std::exception& ex) {
              std::string msg = std::string("Failed to parse server JSON: ") +
                                ex.what();
              GameLogging::Error(msg);
              SetResultText(msg);
            }
          };
        });
  }
};

}  // namespace GameApp

#endif  // GAME_APP_BILLING_SDK_DIALOG_HPP_
