#include "pch.h"
#include "GameHelper.h"
#include "..\GameApplication.h"
#include "../resource.h"

using namespace System::Reflection;
using namespace System::Text::RegularExpressions;
using namespace System::Xml;

namespace GameApplication {
    using namespace System;
    using namespace System::Runtime::Serialization;
    using namespace System::Runtime::Serialization::Json;
    using namespace System::Configuration;

    GameApplication::GameApplication(void)
    {
        mAuthInFlight = false;
        listenerFuncThread = nullptr;
        listenerBackgroundThread = nullptr;
        InitializeComponent();
        appSettingFiles = gcnew AppSettingsFiles();
        PopulateTable();
    }

    void GameApplication::PopulateTable() {
        for each (GameApis ^ api in appSettingFiles->mGameApiList)
        {
            String^ listName = String::Format("{0}-{1} Method", api->MethodName, api->Method);
            lstBoxOperations->Items->Add(listName);
        }
    }

    void GameApplication::ShowText()
    {
        throw gcnew System::NotImplementedException();
    }

    String^ GameApplication::ParseXmlWithReader(String^ filePath)
    {
        try {
            XmlReader^ reader = XmlReader::Create(filePath);
            try {
                while (reader->Read()) {
                    if (reader->NodeType == XmlNodeType::Element) {
                        String^ elementName = reader->Name;
                        if (reader->Read() && reader->NodeType == XmlNodeType::Text) {
                            if (elementName == "PackageName") {
                                mPackageName = reader->Value;
                            }
                            if (elementName == "SKU") {
                                mSku = reader->Value;
                            }
                        }
                    }
                }
            }
            finally {
                reader->Close();
            }
        }
        catch (System::Exception^ ex) {
            System::Console::WriteLine("Exception creating XmlReader: " + ex->Message);
        }
        Console::WriteLine("PackageName : {0}", mPackageName);
        return "";
    }

    void GameApplication::StopActiveListener()
    {
        if (listenerFuncThread != nullptr) {
            listenerFuncThread->Stop();
            listenerFuncThread = nullptr;
        }
        listenerBackgroundThread = nullptr;
    }

    String^ GameApplication::StripQueryString(String^ url)
    {
        if (String::IsNullOrEmpty(url)) {
            return "";
        }
        int queryIdx = url->IndexOf("?");
        return (queryIdx > 0) ? url->Substring(0, queryIdx) : url;
    }

    void GameApplication::AttachBackendAuthHeader(HttpRequestMessage^ requestMessage)
    {
        String^ authToken = Environment::GetEnvironmentVariable("API_AUTH_TOKEN");
        if (String::IsNullOrWhiteSpace(authToken) && appSettingFiles != nullptr) {
            authToken = appSettingFiles->mApiAuthToken;
        }
        if (!String::IsNullOrWhiteSpace(authToken)) {
            requestMessage->Headers->Authorization =
                gcnew Headers::AuthenticationHeaderValue("Bearer", authToken->Trim());
        }
    }

    System::Void GameApplication::GameApplication_Load(System::Object^ sender, System::EventArgs^ e) {
        checkBtnBearerToken->Checked = true;
        mRedirectUri = "http://127.0.0.1/";
        mSd = gcnew SharedData();
        mSd->sdConfig = appSettingFiles->mConfig;
        mSd->sdLocalhostAddress = mRedirectUri;
        LoadWindow(sender, e);
    }

    String^ GameApplication::BuildAuthorizationUrl(String^ state, String^ codeChallenge)
    {
        String^ scope = "https://www.googleapis.com/auth/googleplay.products%20https://www.googleapis.com/auth/googleplay.purchases";

        return String::Format(
            "{0}?response_type=code&scope={5}&redirect_uri={1}&client_id={2}&state={3}&code_challenge={4}&code_challenge_method=S256",
            appSettingFiles->mConfig->authorization_Endpoint,
            Uri::EscapeDataString(mRedirectUri),
            Uri::EscapeDataString(appSettingFiles->mConfig->client_id),
            Uri::EscapeDataString(state),
            Uri::EscapeDataString(codeChallenge),
            scope);
    }

    void GameApplication::AccessToken() {
        if (mAuthInFlight) {
            txtResult->Text = "An OAuth sign-in attempt is already in progress.";
            return;
        }
        mAuthInFlight = true;
        btnExecute->Enabled = false;
        try {
            txtResult->Text = "Starting loopback OAuth2 listener...";
            StopActiveListener();

            listenerFuncThread = gcnew HttpListenerThread();
            listenerFuncThread->BindLoopback();
            mRedirectUri = listenerFuncThread->mPrefix;

            appSettingFiles->RegenerateOAuthSecurityParams();
            Monitor::Enter(mSd->syncRoot);
            try {
                mSd->sdConfig = appSettingFiles->mConfig;
                mSd->sdLocalhostAddress = mRedirectUri;
                mSd->mState = appSettingFiles->mState;
                mSd->codeVerifier = appSettingFiles->codeVerifier;
                mSd->sdApiResponse = "";
            }
            finally {
                Monitor::Exit(mSd->syncRoot);
            }
            listenerFuncThread->sharedData = mSd;

            listenerBackgroundThread = gcnew Thread(gcnew ParameterizedThreadStart(&GameApplication::RunListener));
            listenerBackgroundThread->IsBackground = true;
            listenerBackgroundThread->Start(listenerFuncThread);

            SetValueForKey("redirect_uri", Uri::EscapeDataString(mRedirectUri));
            SetValueForKey("client_id", Uri::EscapeDataString(appSettingFiles->mConfig->client_id));
            SetValueForKey("state", Uri::EscapeDataString(appSettingFiles->mState));
            SetValueForKey("code_challenge", Uri::EscapeDataString(appSettingFiles->codeChallenge));
            String^ authUrl = BuildAuthorizationUrl(appSettingFiles->mState, appSettingFiles->codeChallenge);
            txtBuiltUrl->Text = authUrl;

            Uri^ parsedAuthUri = nullptr;
            if (!TryValidateAllowedHttpsUrl(authUrl, "accounts.google.com", parsedAuthUri)) {
                txtResult->Text = "Error: Authorization URL must be https://accounts.google.com.";
                return;
            }

            Console::WriteLine("Launching browser for OAuth2 sign-in via {0}", parsedAuthUri->Host);
            ProcessStartInfo^ psi = gcnew ProcessStartInfo(parsedAuthUri->AbsoluteUri);
            psi->UseShellExecute = true;
            System::Diagnostics::Process::Start(psi);

            txtResult->Text = "Waiting for OAuth2 callback in browser (up to 60s)...";
            String^ apiResponse = "";
            for (int elapsedMs = 0; elapsedMs < 60000; elapsedMs += 100) {
                Monitor::Enter(mSd->syncRoot);
                try {
                    apiResponse = mSd->sdApiResponse;
                }
                finally {
                    Monitor::Exit(mSd->syncRoot);
                }
                if (!String::IsNullOrEmpty(apiResponse)) {
                    break;
                }
                System::Windows::Forms::Application::DoEvents();
                System::Threading::Thread::Sleep(100);
            }

            if (!String::IsNullOrEmpty(apiResponse)) {
                mSd->sdResposne = ReadResponse(apiResponse);
                if (mSd->sdResposne != nullptr && !String::IsNullOrEmpty(mSd->sdResposne->access_token)) {
                    appSettingFiles->mUserAccessToken = mSd->sdResposne->access_token;
                    txtAccessToken->Text = appSettingFiles->mUserAccessToken;
                    txtResult->Text = String::Format(
                        "OAuth2 sign-in succeeded (access_token: {0}).",
                        RedactTokenForLog(appSettingFiles->mUserAccessToken));
                } else {
                    txtResult->Text = "OAuth2 token exchange failed. Check client_id and GOOGLE_OAUTH_CLIENT_SECRET.";
                }
            } else {
                txtResult->Text = "Timed out waiting for OAuth sign-in callback.";
            }
            this->Activate();
        }
        finally {
            StopActiveListener();
            if (mSd != nullptr) {
                Monitor::Enter(mSd->syncRoot);
                try {
                    mSd->mState = "";
                    mSd->codeVerifier = "";
                }
                finally {
                    Monitor::Exit(mSd->syncRoot);
                }
            }
            mAuthInFlight = false;
            btnExecute->Enabled = true;
        }
    }

    String^ GameApplication::UpdateUrl()
    {
        String^ data = txtDefaulUrl->Text;
        for (int row = 0; row < tableParameters->RowCount; row++) {
            String^ searchStr = String::Format("{0}", tableParameters->GetControlFromPosition(0, row)->Text);
            data = data->Replace("{" + searchStr + "}", tableParameters->GetControlFromPosition(1, row)->Text);
        }
        return data;
    }

    System::Void GameApplication::btnExecute_Click(System::Object^ sender, System::EventArgs^ e) {

        switch (lstBoxOperations->SelectedIndex)
        {
        case HttpOps::GLogin:                 // 0 -> maps to Initialize Billing SDK
            AccessToken();
            break;
        case HttpOps::GetSKUs:                // 1 -> maps to QueryProductDetails + LaunchPurchaseFlow
            GetSkusDetails();
            break;
        case HttpOps::GetPurchases:           // 2 -> maps to QueryPurchases
            GetPurchasesDetails();
            break;
        case HttpOps::VerifyThePurchaseToken: // 3 -> Server purchase token verification
            PostValidatePurchase();
            break;
        case HttpOps::AcknowledgeThePurchase: // 4 -> maps to AcknowledgePurchase
            GetAcknowledgePurchase();
            break;
        case HttpOps::ConsumePurchases:       // 5 -> maps to ConsumePurchase
            PostConsumePurchase();
            break;
        case HttpOps::VerifyIntegrityToken:   // 6 -> maps to VerifyIntegrityToken
            PostVerifyIntegrityToken();
            break;
        default:
            break;
        }
        this->Activate();
        OnExecute(sender, e);
    }

    System::Void GameApplication::lstBox_SelectedIndexChanged(System::Object^ sender, System::EventArgs^ e) {
        if (lstBoxOperations->SelectedIndex < 0) return;
        txtDefaulUrl->Text = "";
        txtResult->Text = "";
        txtBuiltUrl->Text = "";
        GameApis^ curApi = appSettingFiles->mGameApiList->ToArray()[lstBoxOperations->SelectedIndex];
        lblHttpMethod->Text = "Method - " + curApi->Method;
        String^ selectedUrl = curApi->URL;
        txtDefaulUrl->Text = selectedUrl;
        SetTableData();
        switch (lstBoxOperations->SelectedIndex)
        {
        case HttpOps::GLogin:
        {
            SetValueForKey("authorization_Endpoint", appSettingFiles->mConfig->authorization_Endpoint);
            SetValueForKey("redirect_uri", Uri::EscapeDataString(mRedirectUri));
            SetValueForKey("client_id", Uri::EscapeDataString(appSettingFiles->mConfig->client_id));
            SetValueForKey("state", Uri::EscapeDataString(appSettingFiles->mState));
            SetValueForKey("code_challenge", Uri::EscapeDataString(appSettingFiles->codeChallenge));
            SetValueForKey("Scopes", appSettingFiles->mConfig->Scopes);
            txtBuiltUrl->Text = UpdateUrl();
        }
        break;
        case HttpOps::GetSKUs:
        {
            SetValueForKey("packageName", appSettingFiles->mPackageName);
            SetValueForKey("languageCode", "en-US");
            SetValueForKey("ipAddress", GetLocalIPAddress());
            SetValueForKey("skuTypeName", "inapp");
            SetValueForKey("sku1", appSettingFiles->mSku);
            SetValueForKey("productId", appSettingFiles->mSku);
            SetValueForKey("token", txtAccessToken->Text);
            txtBuiltUrl->Text = StripQueryString(UpdateUrl());
        }
        break;
        case HttpOps::GetPurchases:
        {
            SetValueForKey("pName", appSettingFiles->mPackageName);
            SetValueForKey("token", txtAccessToken->Text);
            txtBuiltUrl->Text = StripQueryString(UpdateUrl());
        }
        break;
        case HttpOps::VerifyThePurchaseToken:
        {
            SetValueForKey("packageName", appSettingFiles->mPackageName);
            SetValueForKey("token", appSettingFiles->mPurchaseToken);
            SetValueForKey("productId", appSettingFiles->mSku);
            txtBuiltUrl->Text = StripQueryString(UpdateUrl());
        }
        break;
        case HttpOps::ConsumePurchases:
        {
            SetValueForKey("packageName", appSettingFiles->mPackageName);
            SetValueForKey("token", appSettingFiles->mPurchaseToken);
            SetValueForKey("productId", appSettingFiles->mSku);
            txtBuiltUrl->Text = StripQueryString(UpdateUrl());
        }
        break;
        case HttpOps::AcknowledgeThePurchase:
        {
            SetValueForKey("packageName", appSettingFiles->mPackageName);
            SetValueForKey("token", appSettingFiles->mPurchaseToken);
            SetValueForKey("productId", appSettingFiles->mSku);
            txtBuiltUrl->Text = StripQueryString(UpdateUrl());
        }
        break;
        case HttpOps::VerifyIntegrityToken:
        {
            SetValueForKey("packageName", appSettingFiles->mPackageName);
            SetValueForKey("token", "");
            txtBuiltUrl->Text = StripQueryString(UpdateUrl());
        }
        break;

        default:
            break;
        }
        OnGameListSelected(sender, e);
    }

    /*
     * POST /api/getSkuDetails with JSON body (tokens never placed in URL query strings).
     */
    void GameApplication::GetSkusDetails()
    {
        String^ postUrl = StripQueryString(txtBuiltUrl->Text);
        String^ userAccessToken = SetValueForKey("token");
        if (String::IsNullOrEmpty(userAccessToken)) {
            userAccessToken = txtAccessToken->Text;
        }
        String^ sku = SetValueForKey("sku1");
        if (String::IsNullOrEmpty(sku)) {
            sku = appSettingFiles->mSku;
        }
        String^ skuType = SetValueForKey("skuTypeName");
        if (String::IsNullOrEmpty(skuType)) {
            skuType = "inapp";
        }
        String^ userIp = SetValueForKey("ipAddress");
        String^ langCode = SetValueForKey("languageCode");
        if (String::IsNullOrEmpty(langCode)) {
            langCode = "en-US";
        }
        if (String::IsNullOrEmpty(userAccessToken)) {
            txtResult->Text = "Error: Sign in via Google Login first to obtain an access token.";
            return;
        }

        Console::WriteLine("POST {0} (sku={1})", postUrl, sku);
        String^ jsonData = Serialize(
            gcnew SkuDetailsRequestData(userAccessToken, sku, skuType, userIp, langCode));

        HttpClient^ client = gcnew HttpClient();
        try {
            HttpRequestMessage^ requestMessage = gcnew HttpRequestMessage(HttpMethod::Post, postUrl);
            AttachBackendAuthHeader(requestMessage);
            requestMessage->Content = gcnew StringContent(jsonData, Encoding::UTF8, "application/json");
            Task<HttpResponseMessage^>^ responseTask = client->SendAsync(requestMessage);
            responseTask->Wait();
            HttpResponseMessage^ response = responseTask->Result;
            Task<String^>^ readTask = response->Content->ReadAsStringAsync();
            readTask->Wait();
            String^ readData = readTask->Result;

            if (!response->IsSuccessStatusCode) {
                String^ errorStatus = String::Format("HTTP Error ({0}): {1}", response->StatusCode, readData);
                txtResult->Text = errorStatus;
                Console::WriteLine("GetSkusDetails failed with HTTP {0}", response->StatusCode);
                return;
            }
            if (readData->Contains("\"result\":false")) {
                txtResult->Text = String::Format("Error returned by API: {0}", readData);
                Console::WriteLine("GetSkusDetails returned API error.");
                return;
            }

            String^ res = GetSkuObj(readData);
            txtResult->Text = String::IsNullOrEmpty(res)
                ? "Error: Backend response did not contain a valid https://play.google.com deepLink."
                : res;
            Uri^ checkoutUri = nullptr;
            if (TryValidateAllowedHttpsUrl(res, "play.google.com", checkoutUri)) {
                Console::WriteLine("Launching checkout URL on {0}", checkoutUri->Host);
                ProcessStartInfo^ psi = gcnew ProcessStartInfo(checkoutUri->AbsoluteUri);
                psi->UseShellExecute = true;
                System::Diagnostics::Process::Start(psi);
            }
        }
        finally {
            delete client;
        }
    }

    /*
     * POST /api/getPurchases with JSON body (tokens never placed in URL query strings).
     */
    void GameApplication::GetPurchasesDetails()
    {
        appSettingFiles->mPurchaseToken = "";
        String^ postUrl = StripQueryString(txtBuiltUrl->Text);
        String^ userAccessToken = SetValueForKey("token");
        if (String::IsNullOrEmpty(userAccessToken)) {
            userAccessToken = txtAccessToken->Text;
        }
        if (String::IsNullOrEmpty(userAccessToken)) {
            txtResult->Text = "Error: Sign in via Google Login first to obtain an access token.";
            return;
        }

        Console::WriteLine("POST {0}", postUrl);
        String^ jsonData = Serialize(gcnew PurchasesRequestData(userAccessToken));

        HttpClient^ client = gcnew HttpClient();
        try {
            HttpRequestMessage^ requestMessage = gcnew HttpRequestMessage(HttpMethod::Post, postUrl);
            AttachBackendAuthHeader(requestMessage);
            requestMessage->Content = gcnew StringContent(jsonData, Encoding::UTF8, "application/json");
            Task<HttpResponseMessage^>^ responseTask = client->SendAsync(requestMessage);
            responseTask->Wait();
            HttpResponseMessage^ response = responseTask->Result;
            Task<String^>^ readTask = response->Content->ReadAsStringAsync();
            readTask->Wait();
            String^ readData = readTask->Result;

            if (!response->IsSuccessStatusCode) {
                String^ errorStatus = String::Format("HTTP Error ({0}): {1}", response->StatusCode, readData);
                txtResult->Text = errorStatus;
                Console::WriteLine("GetPurchasesDetails failed with HTTP {0}", response->StatusCode);
                return;
            }
            if (!readData->Contains("purchasesresult")) {
                txtResult->Text = String::Format("Error data return for API: {0}", readData);
                Console::WriteLine("GetPurchasesDetails returned unexpected payload.");
                return;
            }

            MemoryStream^ stream = gcnew MemoryStream(Encoding::UTF8->GetBytes(readData));
            DataContractJsonSerializer^ serializer = gcnew DataContractJsonSerializer(RootObject::typeid);
            RootObject^ result = (RootObject^)serializer->ReadObject(stream);
            Purchase^ purchase = nullptr;
            if (result != nullptr && result->purchasesresult != nullptr && result->purchasesresult->resources != nullptr) {
                Console::WriteLine("Purchases returned: {0}", result->purchasesresult->resources->Length);
                for (int i = 0; i < result->purchasesresult->resources->Length; ++i)
                {
                    purchase = result->purchasesresult->resources[i];
                    Console::WriteLine("Order ID: {0}, Product ID: {1}", purchase->orderId, purchase->productId);
                }
            }
            String^ message = "No purchases found.";
            if (purchase != nullptr)
            {
                appSettingFiles->mPurchaseToken = purchase->purchaseToken;
                if (!String::IsNullOrEmpty(purchase->productId)) {
                    appSettingFiles->mSku = purchase->productId;
                }
                message = String::Format("Product ID: {0}\r\nPurchase Token: {1}",
                    appSettingFiles->mSku, RedactTokenForLog(appSettingFiles->mPurchaseToken));
            }
            txtResult->Text = message;
            this->Activate();
        }
        finally {
            delete client;
        }
    }

    /*
     * POST /api/consumePurchase
     */
    void GameApplication::PostConsumePurchase()
    {
        String^ postUrl = StripQueryString(txtBuiltUrl->Text);
        String^ tok = SetValueForKey("token");
        if (String::IsNullOrEmpty(tok)) {
            tok = appSettingFiles->mPurchaseToken;
        }
        String^ prodId = SetValueForKey("productId");
        if (String::IsNullOrEmpty(prodId)) {
            prodId = appSettingFiles->mSku;
        }
        String^ pkg = SetValueForKey("packageName");
        if (String::IsNullOrEmpty(pkg)) {
            pkg = appSettingFiles->mPackageName;
        }
        if (String::IsNullOrEmpty(tok)) {
            String^ message = "Error: Purchases Token is not found";
            Console::WriteLine(message);
            txtResult->Text = message;
            return;
        }

        Console::WriteLine("POST {0} (productId={1}, token={2})", postUrl, prodId, RedactTokenForLog(tok));
        String^ jsonData = Serialize(gcnew RequestData(prodId, tok, pkg));

        HttpClient^ client = gcnew HttpClient();
        try {
            HttpRequestMessage^ requestMessage = gcnew HttpRequestMessage(HttpMethod::Post, postUrl);
            AttachBackendAuthHeader(requestMessage);
            requestMessage->Content = gcnew StringContent(jsonData, Encoding::UTF8, "application/json");
            Task<HttpResponseMessage^>^ responseTask = client->SendAsync(requestMessage);
            responseTask->Wait();
            HttpResponseMessage^ response = responseTask->Result;
            Task<String^>^ readTask = response->Content->ReadAsStringAsync();
            readTask->Wait();
            String^ readData = readTask->Result;

            if (!response->IsSuccessStatusCode) {
                String^ errorStatus = String::Format("HTTP Error ({0}): {1}", response->StatusCode, readData);
                txtResult->Text = errorStatus;
                Console::WriteLine("PostConsumePurchase failed with HTTP {0}", response->StatusCode);
                return;
            }
            Console::WriteLine("PostConsumePurchase succeeded (HTTP {0}).", response->StatusCode);
            txtResult->Text = readData;
        }
        finally {
            delete client;
        }
    }

    /*
     * POST /api/verifyPurchaseToken
     */
    void GameApplication::PostValidatePurchase()
    {
        String^ postUrl = StripQueryString(txtBuiltUrl->Text);
        String^ tok = SetValueForKey("token");
        if (String::IsNullOrEmpty(tok)) {
            tok = appSettingFiles->mPurchaseToken;
        }
        String^ prodId = SetValueForKey("productId");
        if (String::IsNullOrEmpty(prodId)) {
            prodId = appSettingFiles->mSku;
        }
        String^ pkg = SetValueForKey("packageName");
        if (String::IsNullOrEmpty(pkg)) {
            pkg = appSettingFiles->mPackageName;
        }
        if (String::IsNullOrEmpty(tok)) {
            String^ message = "Error: Purchases Token is not found";
            Console::WriteLine(message);
            txtResult->Text = message;
            return;
        }

        Console::WriteLine("POST {0} (productId={1}, token={2})", postUrl, prodId, RedactTokenForLog(tok));
        String^ jsonData = Serialize(gcnew RequestData(prodId, tok, pkg));

        HttpClient^ client = gcnew HttpClient();
        try {
            HttpRequestMessage^ requestMessage = gcnew HttpRequestMessage(HttpMethod::Post, postUrl);
            AttachBackendAuthHeader(requestMessage);
            requestMessage->Content = gcnew StringContent(jsonData, Encoding::UTF8, "application/json");
            Task<HttpResponseMessage^>^ responseTask = client->SendAsync(requestMessage);
            responseTask->Wait();
            HttpResponseMessage^ response = responseTask->Result;
            Task<String^>^ readTask = response->Content->ReadAsStringAsync();
            readTask->Wait();
            String^ readData = readTask->Result;

            if (!response->IsSuccessStatusCode) {
                String^ errorStatus = String::Format("HTTP Error ({0}): {1}", response->StatusCode, readData);
                txtResult->Text = errorStatus;
                Console::WriteLine("PostValidatePurchase failed with HTTP {0}", response->StatusCode);
                return;
            }
            Console::WriteLine("PostValidatePurchase succeeded (HTTP {0}).", response->StatusCode);
            txtResult->Text = readData;
        }
        finally {
            delete client;
        }
    }

    /*
     * POST /api/acknowledgePurchase
     */
    void GameApplication::GetAcknowledgePurchase()
    {
        String^ postUrl = StripQueryString(txtBuiltUrl->Text);
        String^ tok = SetValueForKey("token");
        if (String::IsNullOrEmpty(tok)) {
            tok = appSettingFiles->mPurchaseToken;
        }
        String^ prodId = SetValueForKey("productId");
        if (String::IsNullOrEmpty(prodId)) {
            prodId = appSettingFiles->mSku;
        }
        String^ pkg = SetValueForKey("packageName");
        if (String::IsNullOrEmpty(pkg)) {
            pkg = appSettingFiles->mPackageName;
        }
        if (String::IsNullOrEmpty(tok)) {
            String^ message = "Error: Purchases Token is not found";
            Console::WriteLine(message);
            txtResult->Text = message;
            return;
        }

        Console::WriteLine("POST {0} (productId={1}, token={2})", postUrl, prodId, RedactTokenForLog(tok));
        String^ jsonData = Serialize(gcnew RequestData(prodId, tok, pkg));

        HttpClient^ client = gcnew HttpClient();
        try {
            HttpRequestMessage^ requestMessage = gcnew HttpRequestMessage(HttpMethod::Post, postUrl);
            AttachBackendAuthHeader(requestMessage);
            requestMessage->Content = gcnew StringContent(jsonData, Encoding::UTF8, "application/json");
            Task<HttpResponseMessage^>^ responseTask = client->SendAsync(requestMessage);
            responseTask->Wait();
            HttpResponseMessage^ response = responseTask->Result;
            Task<String^>^ readTask = response->Content->ReadAsStringAsync();
            readTask->Wait();
            String^ readData = readTask->Result;

            if (!response->IsSuccessStatusCode) {
                String^ errorStatus = String::Format("HTTP Error ({0}): {1}", response->StatusCode, readData);
                txtResult->Text = errorStatus;
                Console::WriteLine("GetAcknowledgePurchase failed with HTTP {0}", response->StatusCode);
                return;
            }
            Console::WriteLine("GetAcknowledgePurchase succeeded (HTTP {0}).", response->StatusCode);
            txtResult->Text = readData;
        }
        finally {
            delete client;
        }
    }

    /*
     * POST /api/verifyIntegrityToken
     */
    void GameApplication::PostVerifyIntegrityToken()
    {
        String^ postUrl = StripQueryString(txtBuiltUrl->Text);
        String^ pkg = SetValueForKey("packageName");
        String^ tok = SetValueForKey("token");
        if (String::IsNullOrEmpty(pkg)) {
            pkg = appSettingFiles->mPackageName;
        }
        if (String::IsNullOrEmpty(tok)) {
            txtResult->Text = "Error: Integrity token is empty. Paste a Play Integrity token into the 'token' parameter row.";
            return;
        }

        Console::WriteLine("POST {0} (packageName={1}, token={2})", postUrl, pkg, RedactTokenForLog(tok));
        String^ jsonData = Serialize(gcnew IntegrityRequestData(pkg, tok));

        HttpClient^ client = gcnew HttpClient();
        try {
            HttpRequestMessage^ requestMessage = gcnew HttpRequestMessage(HttpMethod::Post, postUrl);
            AttachBackendAuthHeader(requestMessage);
            requestMessage->Content = gcnew StringContent(jsonData, Encoding::UTF8, "application/json");

            Task<HttpResponseMessage^>^ responseTask = client->SendAsync(requestMessage);
            responseTask->Wait();
            HttpResponseMessage^ response = responseTask->Result;
            Task<String^>^ readTask = response->Content->ReadAsStringAsync();
            readTask->Wait();
            String^ readData = readTask->Result;

            if (!response->IsSuccessStatusCode) {
                String^ errorStatus = String::Format("HTTP Error ({0}): {1}", response->StatusCode, readData);
                txtResult->Text = errorStatus;
                Console::WriteLine("PostVerifyIntegrityToken failed with HTTP {0}", response->StatusCode);
                return;
            }
            Console::WriteLine("PostVerifyIntegrityToken succeeded (HTTP {0}).", response->StatusCode);
            txtResult->Text = readData;
        }
        finally {
            delete client;
        }
    }

    int GameApplication::SetValueForKey(String^ key, String^ value)
    {
        int i = -1;
        for (i = 0; i < tableParameters->RowCount; i++)
        {
            String^ d = tableParameters->GetControlFromPosition(0, i)->Text;
            if (d == key) {
                tableParameters->GetControlFromPosition(1, i)->Text = value;
                return i;
            }
        }
        return -1;
    }

    List<String^>^ GameApplication::ExtractCurlyBracesBlocks(String^ data)
    {
        String^ pattern = "\\{(.*?)\\}";
        MatchCollection^ matches = Regex::Matches(data, pattern);
        List<String^>^ blocks = gcnew List<String^>(1);
        for each (Match ^ match in matches)
        {
            blocks->Add(match->Groups[1]->Value);
        }
        return blocks;
    }

    void GameApplication::SetTableData() {
        List<String^>^ urlParameters = ExtractCurlyBracesBlocks(txtDefaulUrl->Text);
        tableParameters->Controls->Clear();
        tableParameters = gcnew TableLayoutPanel();
        tableParameters->RowCount = urlParameters->Count;
        tableParameters->ColumnCount = 2;
        tableParameters->Dock = DockStyle::Top;
        tableParameters->AutoSize = false;
        tableParameters->Size = Drawing::Size(480, 1000);

        for (int row = 0; row < tableParameters->RowCount; row++) {
            for (int col = 0; col < tableParameters->ColumnCount; col++) {
                if (col == 0)
                {
                    TextBox^ parameter = gcnew TextBox();
                    parameter->Text = urlParameters->ToArray()[row];
                    parameter->Dock = DockStyle::Fill;
                    parameter->BackColor = Color::LightGray;
                    parameter->ReadOnly = true;
                    tableParameters->Controls->Add(parameter, col, row);
                }
                else
                {
                    TextBox^ textbox = gcnew TextBox();
                    textbox->Text = String::Format("Row {0}, Col {1}", row, 1);
                    textbox->Dock = DockStyle::Fill;
                    textbox->BackColor = Color::LightGray;
                    tableParameters->Controls->Add(textbox, col, row);
                }
            }
        }
        scrollPanel->Controls->Add(tableParameters);
    }

    String^ GameApplication::SetValueForKey(String^ key)
    {
        int i = -1;
        for (i = 0; i < tableParameters->RowCount; i++)
        {
            String^ d = tableParameters->GetControlFromPosition(0, i)->Text;
            if (d == key) {
                return tableParameters->GetControlFromPosition(1, i)->Text;
            }
        }
        return "";
    }

};
