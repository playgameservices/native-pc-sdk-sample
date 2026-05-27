#ifndef __GAMEAPPBILLINGSDKDIALOG_HPP__
#define __GAMEAPPBILLINGSDKDIALOG_HPP__
#include <windows.h>
#include "resource.h"
#include "GameAppGenericClasses.hpp"
#include "billing\client.h"
#include <initialization/models.h>
#include <initialization/initialization.h>
#include <billing\enums.h>
#include <format>
#include <billing/models.h>
#include <future>
using namespace google::play::initialization;
using namespace google::play::billing;

namespace GameApp {
    // Define the list items as an array of strings
    static const wstring ListBoxItems[NUM_LIST_ITEMS] = {
        L"Inititialize Billing SDK",
        L"Query Product Details",
        L"IsBillingProgramAvailable",
        L"CreateBillingProgramReportingDetails",
        L"Launch Purchase Flow",
        L"Acknowledge Purchase",
        L"Consume Purchase"
    };

    static const wstring IsBillingOptions[2] = {
        L"1 - kBillingProgramAlternativeBillingOnly",
        L"2 - kBillingProgramUserChoice"
    };

    class GameAppBillingSdkDialog {
    public:
        GameAppBillingSdkDialog(HINSTANCE hInstance) : m_hWnd(nullptr), m_hInstance(hInstance) {
            appEvents[BillingOps::kSdkInit].onSelect = bind(&GameAppBillingSdkDialog::OnSelectedInitializeSdk, this, std::placeholders::_1);
            appEvents[BillingOps::kSdkInit].onClick = bind(&GameAppBillingSdkDialog::OnClickInitializeSdk, this, std::placeholders::_1);
            appEvents[BillingOps::kBillingClientInit].onSelect = bind(&GameAppBillingSdkDialog::OnSelectedQueryProductDetails, this, std::placeholders::_1);
            appEvents[BillingOps::kBillingClientInit].onClick = bind(&GameAppBillingSdkDialog::OnClickQueryProductDetails, this, std::placeholders::_1);
            appEvents[BillingOps::kIsBillingProgramAvailable].onSelect = bind(&GameAppBillingSdkDialog::OnSelectedIsBillingProgramAvailable, this, std::placeholders::_1);
            appEvents[BillingOps::kIsBillingProgramAvailable].onClick = bind(&GameAppBillingSdkDialog::OnClickIsBillingProgramAvailable, this, std::placeholders::_1);
            appEvents[BillingOps::kCreateBillingProgramReportingDetails].onSelect = bind(&GameAppBillingSdkDialog::OnSelectedCreateBillingProgramReportingDetails, this, std::placeholders::_1);
            appEvents[BillingOps::kCreateBillingProgramReportingDetails].onClick = bind(&GameAppBillingSdkDialog::OnClickCreateBillingProgramReportingDetails, this, std::placeholders::_1);
            appEvents[BillingOps::kLaunchPurchaseFlow].onSelect = bind(&GameAppBillingSdkDialog::OnSelectedLaunchPurchaseFlow, this, std::placeholders::_1);
            appEvents[BillingOps::kLaunchPurchaseFlow].onClick = bind(&GameAppBillingSdkDialog::OnClickLaunchPurchaseFlow, this, std::placeholders::_1);
            appEvents[BillingOps::kAcknowledgePurchase].onSelect = bind(&GameAppBillingSdkDialog::OnSelectedAcknowledgePurchase, this, std::placeholders::_1);
            appEvents[BillingOps::kAcknowledgePurchase].onClick = bind(&GameAppBillingSdkDialog::OnClickAcknowledgePurchase, this, std::placeholders::_1);
            appEvents[BillingOps::kConsumePurchase].onSelect = bind(&GameAppBillingSdkDialog::OnSelectedConsumePurchase, this, std::placeholders::_1);
            appEvents[BillingOps::kConsumePurchase].onClick = bind(&GameAppBillingSdkDialog::OnClickConsumePurchase, this, std::placeholders::_1);
        }

        ~GameAppBillingSdkDialog() {}

        // This function shows the dialog box
        void ShowDialog() {
            DialogBoxParam(m_hInstance, MAKEINTRESOURCE(IDD_SDK), nullptr, DialogProc, reinterpret_cast<LPARAM>(this));
        }

    private:
        HWND m_hWnd;            // Handle to the dialog
        HINSTANCE m_hInstance;  // Application instance handle
        int m_nSelectedOperation = -1;
        HWND hComboBox1 = nullptr;
        HWND hComboBox2 = nullptr;
        HWND hComboBox3 = nullptr;
        HWND hParamLabel1 = nullptr;
        HWND hParamLabel2 = nullptr;
        HWND hParamLabel3 = nullptr;
        HWND hParamStatus = nullptr;
        HWND hResult = nullptr;
        bool m_bCanMoveToStage = false;


        // Billing SDK
        BillingClient* mBillingClient = nullptr;
        ProductId mProductid;
        QueryProductDetailsResult* mQueryProductDetailsResult;
        QueryProductDetailsResultValue mQueryPrudResValue;
        LaunchPurchaseFlowResult* mLaunchPurchaseFlowResult;
        LaunchPurchaseFlowResultValue mLaunchPurchaseFlowResValue;
        AcknowledgePurchaseResult* mAcknowledgePurchaseResult;
        AcknowledgePurchaseResultValue mAcknowledgePurchaseResultValue;
        ConsumePurchaseResult* mConsumePurchaseResult;
        ConsumePurchaseResultValue mConsumePurchaseResultValue;
        IsBillingProgramAvailableResult* mIsBillingProgramAvailableResult;
        IsBillingProgramAvailableResultValue mIsBillingProgramAvailableResultValue;

        // Application Settings
        Manifest manifest;

        // Events Architecture
        using FPEvents = std::function<void(void*)>;

        template <typename FP>
        struct EPair : public std::pair<FP, FP> {
            using std::pair<FP, FP>::pair;
            FP onSelect;
            FP onClick;
            EPair(FP e1, FP e2) : onSelect(e1), onClick(e2) {}
        };

        enum BillingOps {
            kSdkInit,
            kBillingClientInit,
            kIsBillingProgramAvailable,
            kCreateBillingProgramReportingDetails,
            kLaunchPurchaseFlow,
            kAcknowledgePurchase,
            kConsumePurchase,
            Count
        };
        EPair<FPEvents>appEvents[BillingOps::Count];

        map<BillingError, string> sdkErrorMsg = {
            {BillingError::kOk,"The operation completed successfully"},//kOk = 0
            {BillingError::kBillingUnavailable,"A user billing error occurred during processing."},//kBillingUnavailable = 3
            {BillingError::kDeveloperError,"Error resulting from incorrect usage of the API."},//kDeveloperError = 5
            {BillingError::kError,"Fatal error during the API action."},//kError = 6
            {BillingError::kItemAlreadyOwned,"The purchase failed because the item is already owned."},//kItemAlreadyOwned = 7,
            {BillingError::kItemNotOwned,"Requested action on the item failed since it is not owned by the user."},//kItemNotOwned = 8,
            {BillingError::kItemUnavailable,"The requested product is not available for purchase."}, //kItemUnavailable = 4,
            {BillingError::kNetworkError,"A network error occurred during the operation."},//kNetworkError = 12
            {BillingError::kServiceDisconnected,"The app is not connected to the Google Play Billing Library."},//kServiceDisconnected = -1,
            {BillingError::kServiceUnavailable,"The service is currently unavailable."}, //kServiceUnavailable = 2,
            {BillingError::kUserCanceled,"Transaction was canceled by the user."}
        };
        // Static message handler for dialog events
        static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
            GameAppBillingSdkDialog* pThis = reinterpret_cast<GameAppBillingSdkDialog*>(GetWindowLongPtr(hwndDlg, GWLP_USERDATA));
            switch (uMsg) {
            case WM_INITDIALOG: {
                pThis = reinterpret_cast<GameAppBillingSdkDialog*>(lParam);
                SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)pThis);
                if (pThis) {
                    HWND hListBox = GetDlgItem(hwndDlg, IDC_ListOperation);
                    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
                    SendMessage(hListBox, WM_SETFONT, (WPARAM)hFont, TRUE);
                    for (int i = 0; i < NUM_LIST_ITEMS; i++) {
                        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)ListBoxItems[i].c_str());
                    }
                    pThis->OnInitDialog(hwndDlg);
                    }
                }
                return TRUE;

            case WM_COMMAND: {
                WORD notificationCode = HIWORD(wParam);
                if (pThis) {
                    if (LOWORD(wParam) == IDC_Run) {
                        pThis->OnRunButtonClicked();
                        return TRUE;
                    }
                    else if (LOWORD(wParam) == IDC_ListOperation && notificationCode == LBN_SELCHANGE) {
                        pThis->OnListItemSelected(hwndDlg);
                        return TRUE;
                    }
                    else if (LOWORD(wParam) == 2)
                    {
                        EndDialog(hwndDlg, 0);
                        return TRUE;
                    }
                }
            }
            break;
            case WM_CLOSE:
                EndDialog(hwndDlg, 0);
                return TRUE;
            default:
                //return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
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
            SendMessage(hComboBox1, CB_RESETCONTENT, 0, 0);
            SendMessage(hComboBox2, CB_RESETCONTENT, 0, 0);
            SendMessage(hComboBox3, CB_RESETCONTENT, 0, 0);
            switch (numPar) {
            case 3:
                ShowWindow(hParamLabel3, SW_SHOW);
                ShowWindow(hComboBox3, SW_SHOW);
                //break;
            case 2:
                ShowWindow(hParamLabel2, SW_SHOW);
                ShowWindow(hComboBox2, SW_SHOW);
                //break;
            case 1:
                ShowWindow(hParamLabel1, SW_SHOW);
                ShowWindow(hComboBox1, SW_SHOW);
                break;
            default:
                ShowWindow(hParamStatus, SW_SHOW);
                break;
            }
        }

        void OnListItemSelected(HWND hwndDlg) {
            HWND hListBox = GetDlgItem(hwndDlg, IDC_ListOperation);
            HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            SendMessage(hComboBox1, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hComboBox2, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hComboBox2, WM_SETFONT, (WPARAM)hFont, TRUE);
            SetWindowText(hResult, L"");
            m_nSelectedOperation = (int)SendMessage(hListBox, LB_GETCURSEL, 0, 0);
            if (m_nSelectedOperation != LB_ERR) {
                wchar_t text[256] = { 0 };
                SendMessage(hListBox, LB_GETTEXT, (WPARAM)m_nSelectedOperation, (LPARAM)text);
                SetWindowText(GetDlgItem(hwndDlg, IDC_Function), text);
                appEvents[m_nSelectedOperation].onSelect(this);
            }
        }

        // OnInitDialog implemented inside the class
        void OnInitDialog(HWND hwndDlg) {
            m_hWnd = hwndDlg;
            ParseXmlFile("manifest.xml", manifest);
            hComboBox1 = GetDlgItem(hwndDlg, IDC_Param1);
            hComboBox2 = GetDlgItem(hwndDlg, IDC_Param2);
            hComboBox3 = GetDlgItem(hwndDlg, IDC_Param3);
            hParamLabel1 = GetDlgItem(hwndDlg, IDC_Parameter1);
            hParamLabel2 = GetDlgItem(hwndDlg, IDC_Parameter2);
            hParamLabel3 = GetDlgItem(hwndDlg, IDC_Parameter3);
            hParamStatus = GetDlgItem(hwndDlg, IDC_ParamStatus);
            hResult = GetDlgItem(hwndDlg, IDC_Result);
            HideLabelsComboBox(0);
        }

        void GetComboBoxText(HWND combo, int& index, string& ret) {
            index = (int)SendMessage(combo, CB_GETCURSEL, 0, 0);
            if (index != CB_ERR) {
                // CB_GETLBTEXT does NOT truncate. Purchase/offer tokens can
                // exceed 255 chars, so size the buffer based on value from
                // CB_GETLBTEXTLEN.
                LRESULT len = SendMessage(combo, CB_GETLBTEXTLEN, index, 0);
                if (len == CB_ERR) return;
                std::wstring wstr(static_cast<size_t>(len) + 1, L'\0');
                SendMessage(combo, CB_GETLBTEXT, index, (LPARAM)wstr.data());
                wstr.resize(static_cast<size_t>(len));
                ret = wstringTostring(wstr);
            }
        }

        // OnRunButtonClicked implemented inside the class
        void OnRunButtonClicked() {
            try {
                if (m_nSelectedOperation == -1) {
                    string mess = "Warning - Select an operation";
                    GameLogging::Info(mess);
                    SetWindowText(hResult, stringToWstring(mess).c_str());
                    return;
                }
                appEvents[m_nSelectedOperation].onClick(this);
            }
            catch (exception e) {
                GameLogging::Error("Exception in " + wstringTostring(
                    ListBoxItems[m_nSelectedOperation]) + " at " + e.what());
            }
        }
        void OnSelectedInitializeSdk(void* ptr) {
            GameLogging::Info("Selected Initialize Sdk");
            HideLabelsComboBox(0);
        }

        void OnSelectedQueryProductDetails(void* ptr) {
            if (mBillingClient == nullptr) {
                string mess = "Error - Initization the SDK";
                GameLogging::Info(mess);
                SetWindowText(hResult, stringToWstring(mess).c_str());
                return;
            }
            GameLogging::Info("Selected Query Product Details");
            GameAppBillingSdkDialog* win = (GameAppBillingSdkDialog*)ptr;
            HideLabelsComboBox(2);
            mProductid.id = win->manifest.App.sku;
            mProductid.product_type = ProductType::kTypeInApp;
            SetWindowText(hParamLabel1, L"Product ID:");
            wstring proId = stringToWstring(manifest.App.sku.c_str());
            SendMessage(hComboBox1, CB_ADDSTRING, 0, (LPARAM)proId.c_str());
            SendMessage(hComboBox1, CB_SETCURSEL, 0, 0);
            SetWindowText(hParamLabel2, L"Product Type:");
            SendMessage(hComboBox2, CB_ADDSTRING, 0, (LPARAM)stringToWstring("0 - NULL").c_str());
            SendMessage(hComboBox2, CB_ADDSTRING, 0, (LPARAM)stringToWstring("1 - TypeInApp").c_str());// L"NULL");
            SendMessage(hComboBox2, CB_SETCURSEL, 1, 1);
        }

        void OnSelectedIsBillingProgramAvailable(void* ptr) {
            if (mBillingClient == nullptr) {
                string mess = "Error - Initization the SDK";
                GameLogging::Info(mess);
                SetWindowText(hResult, stringToWstring(mess).c_str());
                return;
            }
            GameLogging::Info("Selected Is Billing Program Available");
            GameAppBillingSdkDialog* win = (GameAppBillingSdkDialog*)ptr;
            HideLabelsComboBox(1);
            SetWindowText(hParamLabel1, L"billing_program:");
            SendMessage(hComboBox1, CB_ADDSTRING, 0, (LPARAM)L"0 - No billing program");
            SendMessage(hComboBox1, CB_ADDSTRING, 0, (LPARAM)L"1 - kBillingProgramAlternativeBillingOnly");
            SendMessage(hComboBox1, CB_ADDSTRING, 0, (LPARAM)L"2 - kBillingProgramUserChoice");
            SendMessage(hComboBox1, CB_ADDSTRING, 0, (LPARAM)L"3 - kBillingProgramExternalOffers");
            SendMessage(hComboBox1, CB_SETCURSEL, 1, 1);
        }

        void OnSelectedCreateBillingProgramReportingDetails(void* ptr) {
            if (mBillingClient == nullptr) {
                string mess = "Error - Initization the SDK";
                GameLogging::Info(mess);
                SetWindowText(hResult, stringToWstring(mess).c_str());
                return;
            }
            GameLogging::Info("Selected Create Billing Program Report");
            GameAppBillingSdkDialog* win = (GameAppBillingSdkDialog*)ptr;
            HideLabelsComboBox(1);
            SetWindowText(hParamLabel1, L"billing_program:");
            SendMessage(hComboBox1, CB_ADDSTRING, 0, (LPARAM)L"0 - No billing program");
            SendMessage(hComboBox1, CB_ADDSTRING, 0, (LPARAM)L"1 - kBillingProgramAlternativeBillingOnly");
            SendMessage(hComboBox1, CB_ADDSTRING, 0, (LPARAM)L"2 - kBillingProgramUserChoice");
            SendMessage(hComboBox1, CB_ADDSTRING, 0, (LPARAM)L"3 - kBillingProgramExternalOffers");
            SendMessage(hComboBox1, CB_SETCURSEL, 1, 1);
        }

        void OnSelectedLaunchPurchaseFlow(void* ptr) {
            if (mBillingClient == nullptr || mQueryPrudResValue.product_details.size() == 0) {
                string mess = "Error - \r\n1.Initization the SDK\r\n2. Missing product details";
                GameLogging::Info(mess);
                SetWindowText(hResult, stringToWstring(mess).c_str());
                return;
            }
            GameLogging::Info("Selected Launch Purchase Flow");
            GameAppBillingSdkDialog* win = (GameAppBillingSdkDialog*)ptr;
            HideLabelsComboBox(1);
            SetWindowText(hParamLabel1, L"offer_token:");

            string offerToken = mQueryPrudResValue.product_details[0].product_offers[0].offer_token;

            SendMessage(hComboBox1, CB_ADDSTRING, 0, (LPARAM)stringToWstring(offerToken.c_str()).c_str());
            SendMessage(hComboBox1, CB_SETCURSEL, 0, 0);
        }

        void OnSelectedAcknowledgePurchase(void* ptr) {
            if (mBillingClient == nullptr) {
                string mess = "Error - Initization the SDK";
                GameLogging::Info(mess);
                SetWindowText(hResult, stringToWstring(mess).c_str());
                return;
            }
            GameLogging::Info("Selected Acknowledge Purchase");
            GameAppBillingSdkDialog* win = (GameAppBillingSdkDialog*)ptr;
            HideLabelsComboBox(1);
            SetWindowText(hParamLabel1, L"Purchase token:");
            string purchaseToken = mLaunchPurchaseFlowResValue.product_purchase_details.purchase_token;
            SendMessage(hComboBox1, CB_ADDSTRING, 0, (LPARAM)stringToWstring(purchaseToken.c_str()).c_str());
            SendMessage(hComboBox1, CB_SETCURSEL, 0, 0);
        }

        void OnSelectedConsumePurchase(void* ptr) {
            if (mBillingClient == nullptr) {
                string mess = "Error - Initization the SDK";
                GameLogging::Info(mess);
                SetWindowText(hResult, stringToWstring(mess).c_str());
                return;
            }
            GameLogging::Info("Selected Consume Purchase");
            GameAppBillingSdkDialog* win = (GameAppBillingSdkDialog*)ptr;
            HideLabelsComboBox(1);
            SetWindowText(hParamLabel1, L"Purchase token:");
            string purchaseToken = mLaunchPurchaseFlowResValue.product_purchase_details.purchase_token;
            SendMessage(hComboBox1, CB_ADDSTRING, 0, (LPARAM)stringToWstring(purchaseToken.c_str()).c_str());
            SendMessage(hComboBox1, CB_SETCURSEL, 0, 0);
        }


        void OnClickInitializeSdk(void* ptr)
        {
            GameAppBillingSdkDialog* win = (GameAppBillingSdkDialog*)ptr;
            try {
                // Play_SDK+0.8.0 dll & libs
                // Initialize the SDK as part of the startup sequence of your application.
                auto promise = std::make_shared<std::promise<InitializeResult>>();
                GooglePlayInitialize(
                    [promise](InitializeResult result) {
                        promise->set_value(std::move(result));
                    });

                auto initialize_result = promise->get_future().get();
                int initReturn = initialize_result.ok();
                wstring message = L"";

                if (!initialize_result.ok()) {
                    switch (initReturn) {
                    case (int)InitializationError::kError: {
                        message = L"Fatal error during initialization.";
                    }break;
                    case (int)InitializationError::kActionRequiredShutdownClientProcess: {
                        message = L"The user will need to update Google Play Games and the SDK runtime in order to proceed.";
                    }break;
                    case (int)InitializationError::kSdkRuntimeUpdateRequired: {
                        message = L"The user will need to update Google Play Games and the SDK runtime in order to proceed.";
                    }break;
                    default:
                        message = L"Unknow SDK initialzation error";
                    }
                    MessageBox(win->m_hWnd, message.c_str(), L"Error", MB_OK);
                    return;
                }
                m_bCanMoveToStage = true;
                win->mBillingClient = new BillingClient();
                message = L"SDK Initialization completed successfully. \r\nBilling client created.";
                GameLogging::Info(wstringTostring(message.c_str()));
                SetWindowText(win->hResult, message.c_str());
                SetForegroundWindow(m_hWnd);
            }
            catch (exception e) {
                GameLogging::Error(std::format("SDK initialization Failed with exception {}", e.what()));
            }
        }

		string GetBillingProgram(int nProgID) {
			switch (nProgID) {
			case 1:
                return "1 - kBillingProgramAlternativeBillingOnly";
			case 2:
                return "2 - kBillingProgramUserChoice";
			default:
                return "Unknown billing program";
			}
		}

        void OnClickIsBillingProgramAvailable(void* ptr)
        {
            GameAppBillingSdkDialog* win = (GameAppBillingSdkDialog*)ptr;
			if (win->mBillingClient == nullptr) {
				string mess = "Error - SDK not initialized";
				GameLogging::Info(mess);
				SetWindowText(hResult, stringToWstring(mess).c_str());
				return;
			}

            stringstream message;
            SetWindowText(win->hResult, L"IsBillingProgramAvailable initialized");
            try {
                std::atomic_bool bIsBillingProgramAvailableDone = false;
                IsBillingProgramAvailableParams param;
                int nProgID;
                string sProgID;
                GetComboBoxText(hComboBox1, nProgID, sProgID);
                param.billing_program = (BillingProgram)nProgID;
                IsBillingProgramAvailableResult billingProgramAvailableResult(
                    google::play::billing::BillingError::kError, "unset");
                win->mBillingClient->IsBillingProgramAvailable(
                    param, [&bIsBillingProgramAvailableDone,
                    &billingProgramAvailableResult](IsBillingProgramAvailableResult r) {
                        billingProgramAvailableResult = std::move(r);
                        bIsBillingProgramAvailableDone = true;
                    }
                );
                Sleep(100);
                if (!billingProgramAvailableResult.ok()) {
                    message << "\rWarning in IsBillingProgramAvailable: with billing program option:" << GetBillingProgram(nProgID) << "\r\nError:" << billingProgramAvailableResult.error_message();
                    message << "\r\n\t error_message: " << sdkErrorMsg[(BillingError)billingProgramAvailableResult.code()];;
                    SetWindowText(hResult, stringToWstring(message.str()).c_str());
                    GameLogging::Error(message.str());
                    m_bCanMoveToStage = false;
                }
                else {
                    message << "\r\nIsBillingProgramAvailable: Successfull and ready to buy";
                    SetWindowText(hResult, stringToWstring(message.str()).c_str());
                    m_bCanMoveToStage = true;
                    GameLogging::Info(message.str());
                }
            }
            catch (exception e) {
                GameLogging::Error(std::format("IsBillingProgramAvailable Failed with excetion {}", e.what()));
            }
        }

        void OnClickCreateBillingProgramReportingDetails(void* ptr)
        {
            GameAppBillingSdkDialog* win = (GameAppBillingSdkDialog*)ptr;
            if (win->mBillingClient == nullptr) {
                string mess = "Error - SDK not initialized";
                GameLogging::Info(mess);
                SetWindowText(hResult, stringToWstring(mess).c_str());
                return;
            }

            stringstream message;
            SetWindowText(win->hResult, L"CreateBillingProgramReportingDetails initialized");
            try {
                //message << "product id: " << mProductid.id << " product type: " << mProductid.product_type;
                std::atomic_bool bCreateBillingProgramDone = false;
                CreateBillingProgramReportingDetailsParams params;
                int nProgID;
                string sProgID;
                GetComboBoxText(hComboBox1, nProgID, sProgID);
                params.billing_program = (BillingProgram)nProgID;
                CreateBillingProgramReportingDetailsResult createBillingReport(
                    google::play::billing::BillingError::kError, "unset");
                win->mBillingClient->CreateBillingProgramReportingDetails(
                    params, [&bCreateBillingProgramDone, &createBillingReport]
                    (CreateBillingProgramReportingDetailsResult r) {
                        createBillingReport = std::move(r);
                        bCreateBillingProgramDone = true;
                    });
                Sleep(100);
                if (!createBillingReport.ok()) {
                    m_bCanMoveToStage = false;
                    message << "\rWarning CreateBillingProgramReportingDetails with option:" << GetBillingProgram(nProgID) ;
                    message << "\r\nt error_message: " << sdkErrorMsg[(BillingError)createBillingReport.code()] <<" error message " << createBillingReport.Error;
                    SetWindowText(hResult, stringToWstring(message.str()).c_str());
                    GameLogging::Error(message.str());
                }
                else {
                    m_bCanMoveToStage = true;
                    message << "\r\nCreateBillingProgramReportingDetails: Successfull and ready to buy";
                    SetWindowText(hResult, stringToWstring(message.str()).c_str());
                    GameLogging::Info(message.str());
                }
            }
            catch (exception e) {
                GameLogging::Error(std::format("CreateBillingProgramReportingDetails Failed with excetion {}", e.what()));
            }
        }

        void OnClickQueryProductDetails(void* ptr)
        {
            GameAppBillingSdkDialog* win = (GameAppBillingSdkDialog*)ptr;
            if (!win->mBillingClient) {
                wstring mess = L"Error: \r\nSDK not initialized";
                SetWindowText(win->hResult, mess.c_str());
                MessageBox(win->m_hWnd, mess.c_str(), L"Error", MB_OK);
                return;
            }
            SetWindowText(win->hResult, L"Billingclient initialized");
            GameLogging::Info("Fetching details of a product I'd like to purchase...");
            stringstream message;
            int nProdId = 0, nProdType = 0;
            string sProdID, sProdType;
            GetComboBoxText(hComboBox1, nProdId, sProdID);
            GetComboBoxText(hComboBox2, nProdType, sProdType);

            message << "\r\n\tproduct id: " << nProdId << "\r\n\tproduct type: " << nProdType;
            QueryProductDetailsParams param;
            mProductid.id = sProdID;
            mProductid.product_type = (ProductType)nProdType;
            param.product_ids.push_back(mProductid);
            GameLogging::Info(message.str());
            std::atomic_bool queryProductDetailsDone = false;
            QueryProductDetailsResult queryProductDetailsResult(
                google::play::billing::BillingError::kError, "unset");
            ((BillingClient*)win->mBillingClient)->QueryProductDetails(
                param, [&queryProductDetailsDone,
                &queryProductDetailsResult](QueryProductDetailsResult r) {
                    queryProductDetailsResult = std::move(r);
                    queryProductDetailsDone = true;
                });

            while (!queryProductDetailsDone) {
                Sleep(1000);
            }
            if (!queryProductDetailsResult.ok()) {
                int code = (int)queryProductDetailsResult.code();
                stringstream message;
                message << "Failed to query \r\n\tproductId: " << mProductid.id << "\r\n\tProduct Type: ";
                message << mProductid.product_type << "\r\nstatus was: " << (int)queryProductDetailsResult.code();
                string m = sdkErrorMsg[(BillingError)code];
                message << "\r\n\tmess - " << m;// queryProductDetailsResult.error_message();
                GameLogging::Error(message.str());
                string emessage = "Error: " + message.str();
                SetWindowText(win->hResult, stringToWstring(emessage).c_str());
                m_bCanMoveToStage = false;
                return;// FALSE;
            }

            const auto& queryProductDetailsResultValue =
                queryProductDetailsResult.value();
            GameLogging::Info("Done.\nNow let's try to purchase this:");
            SetWindowText(win->hResult, L"queryProductDetailsResultValue - Done");
            stringstream messProd;
            messProd << "queryProductDetailsResultValue - Done";
            if (queryProductDetailsResultValue.product_details.size() == 0) {
                messProd << "\r\nNo product details for the Package Name: " + manifest.App.PackageName;
                SetWindowText(win->hResult, stringToWstring(messProd.str()).c_str());
                MessageBox(win->m_hWnd, stringToWstring(messProd.str()).c_str(), L"Error", MB_OK);
            }
            for (const auto& detail : queryProductDetailsResultValue.product_details) {
                messProd << "\r\nProduct: \r\n\tid: " << detail.id << "\n\ttitle: " << detail.title;
                messProd << "\r\n\tname: " << detail.name << "\r\n\tdescription: " << detail.description;
                //detail.product_offers[0].formatted_price
                for (const auto& offer : detail.product_offers) {

                    string doubleString = std::to_string((double)offer.price_micros / 1000000);
                    string currency = offer.currency_code;
                    string formatedPrice = offer.formatted_price;
                    string offer = std::format("\r\n\tOffer: {} {}", doubleString, currency);
                    messProd << offer<<"\r\n\tformatted_price: "<< formatedPrice;
                }
            }
            mQueryProductDetailsResult = &queryProductDetailsResult;
            mQueryPrudResValue = queryProductDetailsResultValue;
            SetWindowText(win->hResult, stringToWstring(messProd.str()).c_str());
            GameLogging::Info(messProd.str());
            m_bCanMoveToStage = true;
            SetForegroundWindow(m_hWnd);
        }

        void OnClickLaunchPurchaseFlow(void* ptr)
        {
            GameAppBillingSdkDialog* win = (GameAppBillingSdkDialog*)ptr;
            if (!win->mBillingClient || mQueryPrudResValue.product_details.size() == 0) {
                wstring mess = L"Error may be \r\n1. SDK not initialized or\r\n2. Missing Product details";
                SetWindowText(win->hResult, mess.c_str());
                MessageBox(win->m_hWnd, mess.c_str(), L"Error", MB_OK);
                return;
            }
            GameLogging::Info("Launch Purchase Flow");
            const auto& queryProductDetailsResultValue = mQueryProductDetailsResult->value();
            int nOfferToken;
            string sOfferToken;
            GetComboBoxText(hComboBox1, nOfferToken, sOfferToken);
            string off = mQueryPrudResValue.product_details[0].product_offers[0].offer_token;
            ProductOffer offer_token;
            LaunchPurchaseFlowParams purchaseFlowParams = { sOfferToken,"","" };
            std::atomic_bool launchPurchaseFlowDone = false;
            LaunchPurchaseFlowResult launchPurchaseFlowResult(
                BillingError::kError, "unset");
            win->mBillingClient->LaunchPurchaseFlow(
                purchaseFlowParams, [&launchPurchaseFlowDone, &launchPurchaseFlowResult](
                    LaunchPurchaseFlowResult r) {
                        launchPurchaseFlowResult = std::move(r);
                        launchPurchaseFlowDone = true;
                });
            mLaunchPurchaseFlowResult = &launchPurchaseFlowResult;
            while (!launchPurchaseFlowDone) {
                Sleep(1000);
            }
            Sleep(5000);

            if (!launchPurchaseFlowResult.ok()) {
                m_bCanMoveToStage = false;
                stringstream mess;
                mess << "purchase flow failed" << std::endl
                    << (int)launchPurchaseFlowResult.code() << " "
                    << sdkErrorMsg[(BillingError)launchPurchaseFlowResult.code()] << std::endl;//launchPurchaseFlowResult.error_message() << std::endl;
                GameLogging::Error(mess.str());
                SetWindowText(win->hResult, stringToWstring(mess.str()).c_str());
                SetForegroundWindow(m_hWnd);
                return;
            }
            else {
                const auto& launchPurchaseFlowResultValue =
                    launchPurchaseFlowResult.value();
                mLaunchPurchaseFlowResValue = launchPurchaseFlowResult.value();
                stringstream mess;
                m_bCanMoveToStage = true;
                mess << "Successfully completed launch purchase flow purchase" << endl;
                mess << "\r\nThe purchase token is: " << mLaunchPurchaseFlowResValue.product_purchase_details.purchase_token;
                GameLogging::Info(mess.str());
                SetWindowText(win->hResult, stringToWstring(mess.str()).c_str());
            }
        }

        void OnClickAcknowledgePurchase(void* ptr)
        {
            GameAppBillingSdkDialog* win = (GameAppBillingSdkDialog*)ptr;
            GameLogging::Info("Acknowledge Purchase");
            int nPurchaseTokenId;
            string sPurchaseTokenId;
            GetComboBoxText(hComboBox1, nPurchaseTokenId, sPurchaseTokenId);
            if (!win->mBillingClient || sPurchaseTokenId.length() == 0) {
                wstring mess = L"Error may be\r\n1. SDK not initialized or\r\n2. Missing Purchase token";
                SetWindowText(win->hResult, mess.c_str());
                MessageBox(win->m_hWnd, mess.c_str(), L"Error", MB_OK);
                return;
            }
            AcknowledgePurchaseParams ackonwledgeParam{
                sPurchaseTokenId
            };
            std::atomic_bool acknowledgeDone = false;
            AcknowledgePurchaseResult acknowledgePurchaseResult(
                BillingError::kError, "unset");
            win->mBillingClient->AcknowledgePurchase(ackonwledgeParam, 
                [&acknowledgeDone, &acknowledgePurchaseResult](
                AcknowledgePurchaseResult r) {
                    acknowledgePurchaseResult = std::move(r);
                    acknowledgeDone = true;
                });
            mAcknowledgePurchaseResult = &acknowledgePurchaseResult;
            mAcknowledgePurchaseResultValue = mAcknowledgePurchaseResult->value();
            while (!acknowledgeDone) {
                Sleep(1000);
            }

            if (!acknowledgePurchaseResult.ok()) {
                stringstream mess;
                mess << "Acknowledge flow failed" << std::endl
                    << (int)acknowledgePurchaseResult.code() << " "
                    << sdkErrorMsg[(BillingError)acknowledgePurchaseResult.code()] << std::endl;
                GameLogging::Error(mess.str());
                m_bCanMoveToStage = false;
                SetWindowText(win->hResult, stringToWstring(mess.str()).c_str());
                SetForegroundWindow(m_hWnd);
                return;
            }
            else {
                const auto& acknowledgePurchaseFlowResultValue =
                    acknowledgePurchaseResult.value();
                mAcknowledgePurchaseResultValue = acknowledgePurchaseResult.value();
                stringstream mess;
                m_bCanMoveToStage = true;
                mess << "Successfully completed launch purchase flow purchase" << endl;
                GameLogging::Info(mess.str());
                SetWindowText(win->hResult, stringToWstring(mess.str()).c_str());
                SetForegroundWindow(m_hWnd);
            }

        }
        void OnClickConsumePurchase(void* ptr)
        {
            GameAppBillingSdkDialog* win = (GameAppBillingSdkDialog*)ptr;
            GameLogging::Info("Consume Purchase");
            int nPurchaseTokenId;
            string sPurchaseTokenId;
            GetComboBoxText(hComboBox1, nPurchaseTokenId, sPurchaseTokenId);
            if (!win->mBillingClient || sPurchaseTokenId.length() == 0) {
                wstring mess = L"Error may be\r\n1. SDK not initialized or\r\n2. Missing Purchase token";
                SetWindowText(win->hResult, mess.c_str());
                MessageBox(win->m_hWnd, mess.c_str(), L"Error", MB_OK);
                return;
            }
            ConsumePurchaseParams consumePurchaseParams{
                sPurchaseTokenId
            };
            std::atomic_bool consumeDone = false;
            ConsumePurchaseResult consumePurchaseResult(BillingError::kError, "unset");

            win->mBillingClient->ConsumePurchase(consumePurchaseParams, [&consumeDone, &consumePurchaseResult](
                ConsumePurchaseResult r) {
                    consumePurchaseResult = std::move(r);
                    consumeDone = true;
                });
            mConsumePurchaseResult = &consumePurchaseResult;
            mConsumePurchaseResultValue = mConsumePurchaseResult->value();
            while (!consumeDone) {
                Sleep(1000);
            }

            if (!consumePurchaseResult.ok()) {
                stringstream mess;
                mess << "Consume flow failed" << std::endl
                    << (int)consumePurchaseResult.code() << " "
                    << sdkErrorMsg[(BillingError)consumePurchaseResult.code()] << std::endl;
                GameLogging::Error(mess.str());
                SetWindowText(win->hResult, stringToWstring(mess.str()).c_str());
                SetForegroundWindow(m_hWnd);
                m_bCanMoveToStage = false;
                return;
            }
            else {
                mConsumePurchaseResultValue = consumePurchaseResult.value();
                stringstream mess;

                mess << "Successfully completed consume purchase flow Done." << endl;
                GameLogging::Info(mess.str());
                SetWindowText(win->hResult, stringToWstring(mess.str()).c_str());
                SetForegroundWindow(m_hWnd);
                m_bCanMoveToStage = true;
            }
        }
    };

}

#endif // !__GAMEAPPBILLINGSDKDIALOG_HPP__
