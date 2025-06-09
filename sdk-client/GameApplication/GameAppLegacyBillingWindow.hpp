#pragma once
#ifndef _BILLINGWINDOW_H__
#define _BILLINGWINDOW_H__

#include "GameAppBaseWin.hpp"
#include "GameAppGenericClasses.hpp"

namespace GameApp {
    using namespace GameApp;

    class LegacyBillingWindow : public BaseWindow {
        enum HttpOps {
            GLogin,             	
            GetSKUs,				
            GetPurchases,			
            ValidatePurchaseToken,	
            AcknowledgePurchase,	
            ConsumePurchases,		
            ConsumePurchasesAcknowledge,
            ConsumePurchasesObfuscatedAccountId,
            PurchasesInAppProduct,
            PurchasesProduct,
            Count                   //For number operations
        };
        EPair<FPEvents>appEvents[HttpOps::Count];

    public:
        LegacyBillingWindow(HINSTANCE hInstance) : BaseWindow(hInstance){
            mClassName = L"BillingWindow";
            mDialogTitle = L"Billing Window";
            SendMessage(lstBoxOperations, LB_ADDSTRING, 0, (LPARAM)L"Billing()");

            appEvents[HttpOps::GLogin].onClick = bind(&LegacyBillingWindow::onClickGLogin,this, std::placeholders::_1);
            appEvents[HttpOps::GLogin].onSelect = bind(&LegacyBillingWindow::onSelectGLogin, this, std::placeholders::_1);
            appEvents[HttpOps::GetSKUs].onClick = bind(&LegacyBillingWindow::onClickGetSkus, this, std::placeholders::_1);
            appEvents[HttpOps::GetSKUs].onSelect = bind(&LegacyBillingWindow::onSelectGetSKUs, this, std::placeholders::_1);
            appEvents[HttpOps::GetPurchases].onClick = bind(&LegacyBillingWindow::onCLickGetPurchases, this, std::placeholders::_1);
            appEvents[HttpOps::GetPurchases].onSelect = bind(&LegacyBillingWindow::onSelectGetPurchases, this, std::placeholders::_1);
            appEvents[HttpOps::ValidatePurchaseToken].onClick = bind(&LegacyBillingWindow::onClickGetValidatePurchaseToken, this, std::placeholders::_1);
            appEvents[HttpOps::ValidatePurchaseToken].onSelect = bind(&LegacyBillingWindow::onSelectValidatePurchaseToken, this, std::placeholders::_1);
            appEvents[HttpOps::AcknowledgePurchase].onClick = bind(&LegacyBillingWindow::onClickAcknowledgePurchases, this, std::placeholders::_1);
            appEvents[HttpOps::AcknowledgePurchase].onSelect = bind(&LegacyBillingWindow::onSelectAcknowledgePurchase, this, std::placeholders::_1);
            appEvents[HttpOps::ConsumePurchases].onClick = bind(&LegacyBillingWindow::onClickConsumePurchases, this, std::placeholders::_1);
            appEvents[HttpOps::ConsumePurchases].onSelect = bind(&LegacyBillingWindow::onSelectConsumePurchases, this, std::placeholders::_1);
            appEvents[HttpOps::ConsumePurchasesAcknowledge].onClick = bind(&LegacyBillingWindow::onClickConsumePurchasesAck, this, std::placeholders::_1);
            appEvents[HttpOps::ConsumePurchasesAcknowledge].onSelect = bind(&LegacyBillingWindow::onSelectConsumePurchasesAck, this, std::placeholders::_1);
            appEvents[HttpOps::ConsumePurchasesObfuscatedAccountId].onClick = bind(&LegacyBillingWindow::onClickConsumePurchasesObfuscated, this, std::placeholders::_1);
            appEvents[HttpOps::ConsumePurchasesObfuscatedAccountId].onSelect = bind(&LegacyBillingWindow::onSelectConsumePurchasesObfuscated, this, std::placeholders::_1);
            appEvents[HttpOps::PurchasesInAppProduct].onClick = bind(&LegacyBillingWindow::onClickPurchaseInApp, this, std::placeholders::_1);
            appEvents[HttpOps::PurchasesInAppProduct].onSelect = bind(&LegacyBillingWindow::onSelectPurchaseInApp, this, std::placeholders::_1);
            appEvents[HttpOps::PurchasesProduct].onClick = bind(&LegacyBillingWindow::onClickPurchaseProduct, this, std::placeholders::_1);
            appEvents[HttpOps::PurchasesProduct].onSelect = bind(&LegacyBillingWindow::onSelectPurchaseProduct, this, std::placeholders::_1);
        }
        ~LegacyBillingWindow(){}
        void Show() override
        {
            mApiList = PopulateBillingApiList("LegacyGameApis.json");
            mApiIterator = mApiList->begin();
            BaseWindow::Show();
            SetWindowText(lblGoogleTitle, L"Google Gaming Billing APIs");

            PopulateList();
        }

        void CreateControls() {
        }

        void OnLoadWindow() {

        }

        void onSelectGLogin(void* ptr) {
            LegacyBillingWindow* win = (LegacyBillingWindow*)ptr;
            (*mMappedParams.find(L"authorization_Endpoint")).second = stringToWstring(mAppConfig.authorization_Endpoint);
            (*mMappedParams.find(L"authorization_Endpoint")).second = stringToWstring(mAppConfig.authorization_Endpoint);
            (*mMappedParams.find(L"redirect_uri")).second = stringToWstring(mAppConfig.redirect_uri);
            (*mMappedParams.find(L"client_id")).second = stringToWstring(mAppConfig.client_id);
            (*mMappedParams.find(L"Scopes")).second = stringToWstring(mAppConfig.Scopes);
        }

        void onSelectGetSKUs(void* ptr) {
            (*mMappedParams.find(L"languageCode")).second = L"en-US";
            (*mMappedParams.find(L"ipAddress")).second = stringToWstring(GetIPAddress());// stringToWstring(getIPAddress());
            (*mMappedParams.find(L"skuTypeName")).second = L"inapp";
            (*mMappedParams.find(L"sku1")).second = stringToWstring(manifest.App.sku);
            (*mMappedParams.find(L"productId")).second = stringToWstring(manifest.App.sku);
            (*mMappedParams.find(L"token")).second = stringToWstring(mUserAccessToken);// GetText(txtAccessToken);
        }
        void onSelectGetPurchases(void* ptr) {
            (*mMappedParams.find(L"pName")).second = stringToWstring(manifest.App.sku);
            (*mMappedParams.find(L"token")).second = stringToWstring(mUserAccessToken);
        }
        void onSelectValidatePurchaseToken(void* ptr) {
            (*mMappedParams.find(L"packageName")).second = stringToWstring(manifest.App.PackageName);
            (*mMappedParams.find(L"token")).second = mPurchaseToken;
            (*mMappedParams.find(L"productId")).second = stringToWstring(manifest.App.sku);
        }
        void onSelectAcknowledgePurchase(void* ptr) {
            (*mMappedParams.find(L"packageName")).second = stringToWstring(manifest.App.PackageName);
            (*mMappedParams.find(L"token")).second = mPurchaseToken;
            (*mMappedParams.find(L"productId")).second = stringToWstring(manifest.App.sku);
        }

        void onSelectConsumePurchases(void* ptr) {
            (*mMappedParams.find(L"packageName")).second = stringToWstring(manifest.App.PackageName);
            (*mMappedParams.find(L"token")).second = mPurchaseToken;
            (*mMappedParams.find(L"productId")).second = stringToWstring(manifest.App.sku);
        }
        void onSelectConsumePurchasesAck(void* ptr) {
            (*mMappedParams.find(L"packageName")).second = stringToWstring(manifest.App.PackageName);
            (*mMappedParams.find(L"token")).second = mPurchaseToken;
            (*mMappedParams.find(L"productId")).second = stringToWstring(manifest.App.sku);
        }

        void onSelectConsumePurchasesObfuscated(void* ptr) {
            //{packageName}/skus?userAccessToken={token}&sku={sku1}&skuType=inapp&obfuscatedAccountId={obfuscatedAccountId}&obfuscatedProfileId={obfuscatedProfileId}
            (*mMappedParams.find(L"packageName")).second = stringToWstring(manifest.App.PackageName);
            (*mMappedParams.find(L"token")).second = GetText(txtAccessToken);//UserToken
            (*mMappedParams.find(L"sku1")).second = stringToWstring(manifest.App.sku);//sku
            (*mMappedParams.find(L"obfuscatedAccountId")).second = L"dummyID";
            (*mMappedParams.find(L"obfuscatedProfileId")).second = L"dummyProfile";
        }
        void onSelectPurchaseInApp(void* ptr) {
            (*mMappedParams.find(L"packageName")).second = stringToWstring(manifest.App.PackageName);
        }
        void onSelectPurchaseProduct(void* ptr) {
            (*mMappedParams.find(L"packageName")).second = stringToWstring(manifest.App.PackageName);
            (*mMappedParams.find(L"token")).second = mPurchaseToken;
            (*mMappedParams.find(L"productId")).second = stringToWstring(manifest.App.sku);
        }
        
        void OnListBoxSelectionChange() {
            SetWindowText(lblHttpMethod, L"");
            SetWindowText(txtResult, L"");
            mApiIterator = mApiList->begin();
            mApiSelected = (int)SendMessage(lstBoxOperations, LB_GETCURSEL, 0, 0);
            advance(mApiIterator, mApiSelected);
            string data = "Method-Type: " + (*mApiIterator)->methodType;
            wstring wdefaultData = stringToWstring(data);
            SetWindowText(lblHttpMethod, wdefaultData.c_str());
            wstring wdefaultUrl = stringToWstring((*mApiIterator)->url);
            SetWindowText(txtDefaulUrl, wdefaultUrl.c_str());
            for (HWND item : mListOfTxtboxs)
                SetWindowText(item, L"");
            mListOfTxtboxs.clear();
            mMappedParams.clear();
            mMappedParams = ExtractCurlyBracesBlocks(wdefaultUrl);
            SetTableData(mMappedParams);
            appEvents[(HttpOps)mApiSelected].onSelect(this);
            SetTableData(mMappedParams);
            UpdateUrl(mMappedParams);
        }
    protected:
        list<BillingApis*>* mApiList;
        list<BillingApis*>::iterator mApiIterator;
        map<wstring, wstring> mMappedParams;
        int mApiSelected = -1;
        
        void OnButtonClick(int buttonID)override {
            appEvents[(HttpOps)mApiSelected].onClick(this);
        }

        std::string extractDeepLink(const std::string& jsonString) {
            try {
                nlohmann::json j = nlohmann::json::parse(jsonString);

                if (j.is_object() && j.contains("result")) {
                    auto result = j["result"];
                    if (result.is_object() && result.contains("resources") && result["resources"].is_array()) {
                        auto resources = result["resources"];
                        if (!resources.empty() && resources[0].is_object() && resources[0].contains("deepLink")) {
                            return resources[0]["deepLink"].get<std::string>();
                        }
                    }
                }
                return ""; // deepLink not found
            }
            catch (const nlohmann::json::parse_error& e) {
                std::cerr << "Error parsing JSON: " << e.what() << std::endl;
                return "";
            }
            catch (const std::exception& e) {
                std::cerr << "General Error: " << e.what() << std::endl;
                return "";
            }
        }

        void onClickGetSkus(void* ptr) {
            try {
                string url = wstringTostring(mBuildUrl);
                RestClient::Response response = RestClient::get(url);
                std::stringstream mess;
                mess << "url\nRestClient::get(url);\n";
                if (response.code == 200) {
                    mess << "GET - LegacyGetSkus request successful\nResponse body: " 
                        << response.body << std::endl;
                    GameLogging::Info(mess.str());
                    string deeplink = extractDeepLink(response.body);
                    SetWindowText(txtResult, stringToWstring(deeplink).c_str());
                    ShellExecuteA(NULL, "open", deeplink.c_str(), NULL, NULL, SW_SHOWNORMAL);
                    return;
                }
                else {
                    mess << "GET request on GetSKUs failed. Status code: " << response.code << std::endl;
                    mess << "Error: " << response.body << std::endl;
                    GameLogging::Error(mess.str());
                }
            }
            catch (exception e) {
                GameLogging::Error("LegacyGetSkus-Exception: " + string(e.what()));
            }
        }

        std::string extractPurchaseToken(const std::string& jsonString) {
            try {
                nlohmann::json j = nlohmann::json::parse(jsonString);

                if (j.contains("purchasesresult") && j["purchasesresult"].is_object() &&
                    j["purchasesresult"].contains("resources") && j["purchasesresult"]["resources"].is_array() &&
                    !j["purchasesresult"]["resources"].empty() &&
                    j["purchasesresult"]["resources"][0].is_object() &&
                    j["purchasesresult"]["resources"][0].contains("purchaseToken")) {

                    return j["purchasesresult"]["resources"][0]["purchaseToken"].get<std::string>();
                }
                return ""; // purchaseToken not found
            }
            catch (const nlohmann::json::parse_error& e) {
                std::cerr << "Error parsing JSON: " << e.what() << std::endl;
                return "";
            }
            catch (const std::exception& e) {
                std::cerr << "General Error: " << e.what() << std::endl;
                return "";
            }
        }

        void onCLickGetPurchases(void* ptr) {
            try {
                string url = wstringTostring(mBuildUrl);
                RestClient::Response response = RestClient::get(url);
                std::stringstream mess;
                mess << "url\nRestClient::get(url);\n";
                if (response.code == 200) {
                    string sPurchaseToken = extractPurchaseToken(response.body);
                    mPurchaseToken = stringToWstring(extractPurchaseToken(response.body));
                    mess << "GET-LegacyGetPurchases request successful\nResponse body: "
                        << response.body << std::endl<<"PurchaseToken: "<< sPurchaseToken;
                    GameLogging::Info(mess.str());
                    SetWindowText(txtResult, stringToWstring("PurchaseToken: "+ sPurchaseToken).c_str());
                    return;
                }
                else {
                    mess << "GET request on LegacyGetPurchases failed. Status code: " << response.code << std::endl;
                    mess << "Error: " << response.body << std::endl;
                    GameLogging::Error(mess.str());
                }
            }
            catch (exception e) {
                GameLogging::Error("LegacyGetPurchases-Exception: " + string(e.what()));
            }
        }

        void onClickGetValidatePurchaseToken(void* ptr) {
            try {
                string url = wstringTostring(mBuildUrl);
                RestClient::Response response = RestClient::get(url);
                std::stringstream mess;
                mess << "url\nRestClient::get(url);\n";
                if (response.code == 200) {
                    mess << "GET-LegacyGetValidatePurchaseToken request successful\nResponse body: "
                        << response.body << std::endl;
                    GameLogging::Info(mess.str());
                    SetWindowText(txtResult, stringToWstring(response.body).c_str());
                    return;
                }
                else {
                    mess << "GET request on LegacyGetValidatePurchaseToken failed. Status code: " << response.code << std::endl;
                    mess << "Error: " << response.body << std::endl;
                    GameLogging::Error(mess.str());
                }
            }
            catch (exception e) {
                GameLogging::Error("LegacyGetValidatePurchaseToken-Exception: " + string(e.what()));
            }
        }

        void onClickAcknowledgePurchases(void* ptr) {
            try {
                string url = wstringTostring(mBuildUrl);
                cout << "txtBuiltUrl: " << url;
                GameLogging::Info("Legacy Post AcknowledgePurchases:\nurl: " + url);
                string pToken = wstringTostring(mPurchaseToken);
                json jsonData;
                jsonData["productId"] = manifest.App.sku;
                jsonData["sku"] = manifest.App.sku;
                jsonData["token"] = pToken;
                std::string content_type = "application/json";
                std::string content = jsonData.dump(4);
                RestClient::Response response = RestClient::post(url, content_type, content);
                std::stringstream mess;
                mess << "url" << url << endl;
                mess << "content_type" << content_type << endl;
                mess << "RestClient::post(url, content_type, payload);" << endl;
                if (response.code == 200) {
                    string res = "Post Acknowledge Purchases - Successfully done";
                    mess << "POST - LegacyPostAcknowledgePurchases request successful" << std::endl;
                    mess << "Response body: " << response.body << std::endl;
                    GameLogging::Info(mess.str());
                    if (response.body != "\"\"")
                        res = response.body;
                    SetWindowText(txtResult, stringToWstring(res).c_str());
                    return;
                }
                else {
                    mess << "POST - Legacy Post Acknowledge Purchases request on GetSKUs failed. Status code: " << response.code << std::endl;
                    mess << "Error: " << response.body << std::endl;
                    GameLogging::Error(mess.str());
                }
            }
            catch (exception e) {
                GameLogging::Error("LegacyPostAcknowledgePurchases-Exception: " + string(e.what()));
            }
        }

        void onClickConsumePurchases(void* ptr) {
            try {
                string url = wstringTostring(mBuildUrl);
                cout << "txtBuiltUrl: " << url;
                GameLogging::Info("Legacy Post ConsumePurchases:\nurl: " + url);
                string pToken = wstringTostring(mPurchaseToken);
                json jsonData;
                jsonData["productId"] = manifest.App.sku;
                jsonData["sku"] = manifest.App.sku;
                jsonData["token"] = pToken;
                std::string content_type = "application/json";
                std::string content = jsonData.dump(4);
                RestClient::Response response = RestClient::post(url, content_type, content);
                std::stringstream mess;
                mess << "url" << url << endl;
                mess << "content_type" << content_type << endl;
                mess << "RestClient::post(url, content_type, payload);" << endl;
                if (response.code == 200) {
                    string res = "Legacy Post ConsumePurchases - Successfully done";
                    mess << "POST - Legacy Post ConsumePurchases request successful" << std::endl;
                    mess << "Response body: " << response.body << std::endl;
                    GameLogging::Info(mess.str());
                    int len = (int)std::strlen(response.body.c_str());
                    if (response.body != "\"\"")
                        res = response.body;
                    SetWindowText(txtResult, stringToWstring(res).c_str());
                    return;
                }
                else {
                    mess << "POST - LegacyPostConsumePurchasesrequest on GetSKUs failed. Status code: " << response.code << std::endl;
                    mess << "Error: " << response.body << std::endl;
                    GameLogging::Error(mess.str());
                }
            }
            catch (exception e) {
                GameLogging::Error("LegacyPostConsumePurchases-Exception: " + string(e.what()));
            }
        }

        void onClickConsumePurchasesAck(void* ptr) {
            //(*mMappedParams.find(L"packageName")).second = stringToWstring(manifest.App.PackageName);
            //(*mMappedParams.find(L"token")).second = mPurchaseToken;
            //(*mMappedParams.find(L"productId")).second = stringToWstring(manifest.App.sku);

            try {
                string url = wstringTostring(mBuildUrl);
                cout << "txtBuiltUrl: " << url;
                GameLogging::Info("Legacy Post ConsumePurchases:\nurl: " + url);
                string pToken = wstringTostring(mPurchaseToken);
                json jsonData;
                jsonData["packageName"] = (*mMappedParams.find(L"packageName")).second;
                jsonData["productId"] = (*mMappedParams.find(L"token")).second;
                jsonData["token"] = pToken;
                std::string content_type = "application/json";
                std::string content = jsonData.dump(4);
                RestClient::Response response = RestClient::post(url, content_type, content);
                std::stringstream mess;
                mess << "url" << url << endl;
                mess << "content_type" << content_type << endl;
                mess << "RestClient::post(url, content_type, payload);" << endl;
                if (response.code == 200) {
                    string res = "Legacy Post ConsumePurchases - Successfully done";
                    mess << "POST - Legacy Post ConsumePurchases request successful" << std::endl;
                    mess << "Response body: " << response.body << std::endl;
                    GameLogging::Info(mess.str());
                    int len = (int)std::strlen(response.body.c_str());
                    if (response.body != "\"\"")
                        res = response.body;
                    SetWindowText(txtResult, stringToWstring(res).c_str());
                    return;
                }
                else {
                    mess << "POST - LegacyPostConsumePurchasesrequest on GetSKUs failed. Status code: " << response.code << std::endl;
                    mess << "Error: " << response.body << std::endl;
                    GameLogging::Error(mess.str());
                }
            }
            catch (exception e) {
                GameLogging::Error("LegacyPostConsumePurchases-Exception: " + string(e.what()));
            }

        }

        void onClickConsumePurchasesObfuscated(void* ptr) {
            //{packageName}/skus?userAccessToken={token}&sku={sku1}&skuType=inapp&obfuscatedAccountId={obfuscatedAccountId}&obfuscatedProfileId={obfuscatedProfileId}
            //(*mMappedParams.find(L"packageName")).second = stringToWstring(manifest.App.PackageName);
            //(*mMappedParams.find(L"token")).second = GetText(txtAccessToken);//UserToken
            //(*mMappedParams.find(L"sku1")).second = stringToWstring(manifest.App.sku);//sku
            //(*mMappedParams.find(L"obfuscatedAccountId")).second = L"dummyID";
            //(*mMappedParams.find(L"obfuscatedProfileId")).second = L"dummyProfile";

            try {
                string url = wstringTostring(mBuildUrl);
                cout << "txtBuiltUrl: " << url;
                GameLogging::Info("Legacy Post ConsumePurchases:\nurl: " + url);
                string pToken = wstringTostring(mPurchaseToken);
                json jsonData;
                jsonData["packageName"] = stringToWstring(manifest.App.PackageName);
                jsonData["sku"] = manifest.App.sku;
                jsonData["token"] = pToken;
                jsonData["obfuscatedAccountId"] = (*mMappedParams.find(L"obfuscatedAccountId")).second;
                jsonData["obfuscatedProfileId"] = (*mMappedParams.find(L"obfuscatedProfileId")).second;
                std::string content_type = "application/json";
                std::string content = jsonData.dump(4);
                RestClient::Response response = RestClient::post(url, content_type, content);
                std::stringstream mess;
                mess << "url" << url << endl;
                mess << "content_type" << content_type << endl;
                mess << "RestClient::post(url, content_type, payload);" << endl;
                if (response.code == 200) {
                    string res = "Legacy Post ConsumePurchases - Successfully done";
                    mess << "POST - Legacy Post ConsumePurchases request successful" << std::endl;
                    mess << "Response body: " << response.body << std::endl;
                    GameLogging::Info(mess.str());
                    int len = (int)std::strlen(response.body.c_str());
                    if (response.body != "\"\"")
                        res = response.body;
                    SetWindowText(txtResult, stringToWstring(res).c_str());
                    return;
                }
                else {
                    mess << "POST - LegacyPostConsumePurchasesrequest on GetSKUs failed. Status code: " << response.code << std::endl;
                    mess << "Error: " << response.body << std::endl;
                    GameLogging::Error(mess.str());
                }
            }
            catch (exception e) {
                GameLogging::Error("LegacyPostConsumePurchases-Exception: " + string(e.what()));
            }
        }
        void onClickPurchaseInApp(void* ptr) {
            //(*mMappedParams.find(L"packageName")).second = stringToWstring(manifest.App.PackageName);
            try {
                string url = wstringTostring(mBuildUrl);
                cout << "txtBuiltUrl: " << url;
                GameLogging::Info("Legacy Post onClickPurchaseInApp:\nurl: " + url);
                string pToken = wstringTostring(mPurchaseToken);
                json jsonData;
                jsonData["packageName"] = stringToWstring(manifest.App.PackageName);

                std::string content_type = "application/json";
                std::string content = jsonData.dump(4);
                RestClient::Response response = RestClient::post(url, content_type, content);
                std::stringstream mess;
                mess << "url" << url << endl;
                mess << "content_type" << content_type << endl;
                mess << "RestClient::post(url, content_type, payload);" << endl;
                if (response.code == 200) {
                    string res = "Legacy Post onClickPurchaseInApp - Successfully done";
                    mess << "POST - Legacy Post onClickPurchaseInApp request successful" << std::endl;
                    mess << "Response body: " << response.body << std::endl;
                    GameLogging::Info(mess.str());
                    int len = (int)std::strlen(response.body.c_str());
                    if (response.body != "\"\"")
                        res = response.body;
                    SetWindowText(txtResult, stringToWstring(res).c_str());
                    return;
                }
                else {
                    mess << "POST - LegacyPostonClickPurchaseInApp on GetSKUs failed. Status code: " << response.code << std::endl;
                    mess << "Error: " << response.body << std::endl;
                    GameLogging::Error(mess.str());
                }
            }
            catch (exception e) {
                GameLogging::Error("LegacyPostonClickPurchaseInApp-Exception: " + string(e.what()));
            }
        }
        
        void onClickPurchaseProduct(void* ptr) {
        //    (*mMappedParams.find(L"packageName")).second = stringToWstring(manifest.App.PackageName);
        //    (*mMappedParams.find(L"token")).second = mPurchaseToken;
        //    (*mMappedParams.find(L"productId")).second = stringToWstring(manifest.App.sku);

            try {
                string url = wstringTostring(mBuildUrl);
                cout << "txtBuiltUrl: " << url;
                GameLogging::Info("Legacy onClickPurchaseProduct:\nurl: " + url);
                string pToken = wstringTostring(mPurchaseToken);
                json jsonData;
                jsonData["packageName"] = stringToWstring(manifest.App.PackageName);
                jsonData["productId"] = manifest.App.sku;
                jsonData["token"] = mPurchaseToken;
                std::string content_type = "application/json";
                std::string content = jsonData.dump(4);
                RestClient::Response response = RestClient::post(url, content_type, content);
                std::stringstream mess;
                mess << "url" << url << endl;
                mess << "content_type" << content_type << endl;
                mess << "RestClient::post(url, content_type, payload);" << endl;
                if (response.code == 200) {
                    string res = "Legacy Post onClickPurchaseProduct - Successfully done";
                    mess << "POST - Legacy Post onClickPurchaseProduct request successful" << std::endl;
                    mess << "Response body: " << response.body << std::endl;
                    GameLogging::Info(mess.str());
                    int len = (int)std::strlen(response.body.c_str());
                    if (response.body != "\"\"")
                        res = response.body;
                    SetWindowText(txtResult, stringToWstring(res).c_str());
                    return;
                }
                else {
                    mess << "POST - LegacyPostonClickPurchaseProduct failed. Status code: " << response.code << std::endl;
                    mess << "Error: " << response.body << std::endl;
                    GameLogging::Error(mess.str());
                }
            }
            catch (exception e) {
                GameLogging::Error("LegacyPostonClickPurchaseProduct-Exception: " + string(e.what()));
            }
        }

        void OnRebuildUrlButtonClick() {
            SetWindowText(txtBuiltUrl, L"");
            UpdateUrl(mMappedParams);
        }

        void PopulateList() {
            for (std::list<BillingApis*>::iterator it = mApiList->begin();
                it != mApiList->end(); ++it) {
                string data = to_string(((BillingApis*)*it)->id) + "  " +
                    ((BillingApis*)*it)->methodName;
                wstring line = stringToWstring(data);
                SendMessage(lstBoxOperations, LB_ADDSTRING, 0,
                    (LPARAM)stringToWstring(data).c_str());
            }
        }
    private:
        //string mPurchasesToken;
    };
}

#endif // !_BILLINGWINDOW_H__
