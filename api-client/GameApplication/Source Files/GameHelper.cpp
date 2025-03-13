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
    using namespace CLogging;

    GameApplication::GameApplication(void)
    {
        logger = Logger::getInstance();
        InitializeComponent();
        appSettingFiles = gcnew AppSettingsFiles();
        PopulateTable();
        HRSRC hgameXml = FindResource(NULL, MAKEINTRESOURCE(GAME_XML_ID), RT_RCDATA);
        HRSRC hConfigJson = FindResource(NULL, MAKEINTRESOURCE(IDR_JSON2), RT_RCDATA);
        HRSRC hGameJson = FindResource(NULL, MAKEINTRESOURCE(IDR_JSON1), RT_RCDATA);
        HRSRC hServiceJson = FindResource(NULL, MAKEINTRESOURCE(IDR_JSON3), RT_RCDATA);
        if (!hgameXml) {
            Console::WriteLine("Resource not found!");
        }
        else {
            HGLOBAL hResData = LoadResource(NULL, hgameXml);
            if (!hResData) Console::WriteLine("Failed to load resource!");
            else
            {
                DWORD size = SizeofResource(NULL, hgameXml);
                const char* data = static_cast<const char*>(LockResource(hResData));
            }
        }

        Assembly^ assembly = Assembly::GetExecutingAssembly();
        array<String^>^ resources = assembly->GetManifestResourceNames();
        for (int i = 0; i < resources->Length; i++)
        {
            Console::WriteLine("Found resource: " + resources[i]);  // Log the available resources
        }
        String^ resourceName = "GameApplication.GameApplication.GameApis.json";// "YourNamespace.Resources." + fileName;
        Stream^ resourceStream = assembly->GetManifestResourceStream(resourceName);
        //StreamReader^ reader = gcnew StreamReader(resourceStream, Encoding::UTF8);
        //reader->ReadToEnd();
    }

    void GameApplication::PopulateTable() {
        for each (GameApis ^ api in appSettingFiles->mGameApiList)
        {
            //Console::WriteLine("id {0}: url{1}", api->ID,api->URL);
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
            String^ output = "";
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
                        output += String::Format("{0}: {1}\n", elementName, reader->Value);
                    }
                }
            }
        }
        catch (System::Exception^ ex) {
            System::Console::WriteLine("Exception creating XmlReader: " + ex->Message);
        }
        Console::WriteLine("PackageName : {0}", mPackageName);
        return "";
    }

    System::Void GameApplication::GameApplication_Load(System::Object^ sender, System::EventArgs^ e) {
        checkBtnBearerToken->Checked = true;
        listenerFuncThread = gcnew HttpListenerThread();

        listenerBackgroundThread = gcnew Thread(gcnew ParameterizedThreadStart(&GameApplication::RunListener));
        listenerBackgroundThread->IsBackground = true; // Make it a background thread
        listenerBackgroundThread->Start(listenerFuncThread);

        mRedirectUri = listenerFuncThread->mPrefix;
        Console::WriteLine("Auth::AuthWindow_Load - mState - {0}", appSettingFiles->mState);
        //mRedirectUri = String::Format("http://{0}:{1}/", IPAddress::Loopback->ToString(), 8080);
        mSd = gcnew SharedData();
        //listenerFuncThread->
        mSd->sdConfig = appSettingFiles->mConfig;
        mSd->mState = appSettingFiles->mState;
        listenerFuncThread->sharedData = mSd;
        mSd->sdLocalhostAddress = mRedirectUri;
        LoadWindow(sender, e);
    }

    String^ GameApplication::BuildAuthorizationUrl(String^ state, String^ codeChallenge)
    {
        //String^ scope = "openid%20email%20profile%20https://www.googleapis.com/auth/calendar.readonly";//https://www.googleapis.com/auth/androidpublisher%20profile";
        String^ scope = "https://www.googleapis.com/auth/googleplay.products%20https://www.googleapis.com/auth/googleplay.purchases";

        return String::Format("{0}?response_type=code&scope={5}&redirect_uri={1}&client_id={2}&state={3}&code_challenge={4}&code_challenge_method=S256",
            appSettingFiles->mConfig->authorization_Endpoint,
            Uri::EscapeDataString(mRedirectUri),
            appSettingFiles->mConfig->client_id,
            appSettingFiles->mState,
            codeChallenge, scope);
    }

    void GameApplication::AccessToken() {
        txtResult->Text = "Started Auth";
        String^ authUrl = BuildAuthorizationUrl(appSettingFiles->mState, codeChallenge);
        if (txtBuiltUrl->Text != authUrl) {
            Console::WriteLine("Both are differnt \n{0}\n{1}", txtBuiltUrl->Text, authUrl);
        }

        authUrl = txtBuiltUrl->Text;

        //HttpClient^ client = gcnew HttpClient();
        //Task<HttpResponseMessage^>^ responseTask = client->GetAsync(authUrl);
        //responseTask->Wait();
        //HttpResponseMessage^ response = responseTask->Result;
        //if (!response->IsSuccessStatusCode) {
        //    String^ errorStatus = String::Format("HTTP Error: {0}", response->StatusCode);
        //    txtResult->Text = errorStatus;
        //    Console::WriteLine(errorStatus);
        //    return;
        //}

        Console::WriteLine("Auth URL: {0}", authUrl);
        System::Diagnostics::Process::Start(authUrl);
        HttpListenerContext^ context = listenerFuncThread->mListenerOnThread->GetContext();

        mSd->sdResposne = ReadResponse(mSd->sdApiResponse);
        appSettingFiles->mUserAccessToken = mSd->sdResposne->access_token;
        txtAccessToken->Text = appSettingFiles->mUserAccessToken;

        txtResult->Text = mSd->sdApiResponse;
        this->Activate();
        return;
    }

    String^ GameApplication::UpdateUrl()
    {
        String^ data = txtDefaulUrl->Text;
        for (int row = 0; row < tableParameters->RowCount; row++) {
            String^ searchStr = String::Format("{0}", tableParameters->GetControlFromPosition(0, row)->Text);
            //Console::WriteLine("row - {0}", "{" + searchStr + "}");
            data = data->Replace("{" + searchStr + "}", tableParameters->GetControlFromPosition(1, row)->Text);
            //Console::WriteLine("url: {0}", data);
        }
        return data;
    }

    System::Void GameApplication::btnExecute_Click(System::Object^ sender, System::EventArgs^ e) {

        switch (lstBoxOperations->SelectedIndex)
        {
        case HttpOps::GLogin:  //0
        {
            AccessToken();
        }
        break;
        case HttpOps::GetPurchases:
        {
            GetPurchasesDetails();
        }
        break;
        case HttpOps::GetSKUs:
        {
            GetSkusDetails();
        }
        break;
        case HttpOps::AcknowledgeThePurchase:
            GetAcknowledgePurchase();
            break;
        case HttpOps::VerifyThePurchaseToken:
            GetValidatePurchase();
            break;
        case HttpOps::ConsumePurchases:
            PostConsumePurchase();
            break;
        default:
            break;
        }
        this->Activate();
        OnExecute(sender, e);
    }

    System::Void GameApplication::lstBox_SelectedIndexChanged(System::Object^ sender, System::EventArgs^ e) {
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
        case HttpOps::GLogin:	//0
        {
            SetValueForKey("authorization_Endpoint", appSettingFiles->mConfig->authorization_Endpoint);
            SetValueForKey("redirect_uri", Uri::EscapeDataString(mRedirectUri));
            //SetValueForKey("redirect_uri", "https://dynasty-teapot-sample.web.app/");
            SetValueForKey("client_id", appSettingFiles->mConfig->client_id);
            SetValueForKey("state", appSettingFiles->mState);
            SetValueForKey("code_challenge", "");
            SetValueForKey("Scopes", appSettingFiles->mConfig->Scopes);
            txtBuiltUrl->Text = UpdateUrl();
        }
        break;
        break;
        case HttpOps::GetSKUs:	//3
        {
            SetValueForKey("packageName", appSettingFiles->mPackageName);
            SetValueForKey("languageCode", "en-US");
            SetValueForKey("ipAddress", GetLocalIPAddress());
            SetValueForKey("skuTypeName", "inapp");
            SetValueForKey("sku1", appSettingFiles->mSku);
            SetValueForKey("productId", appSettingFiles->mSku);
            SetValueForKey("token", txtAccessToken->Text);
            txtBuiltUrl->Text = UpdateUrl();
        }
        break;
        case HttpOps::GetPurchases:
        {
            SetValueForKey("pName", appSettingFiles->mPackageName);
            SetValueForKey("token", txtAccessToken->Text);
            txtBuiltUrl->Text = UpdateUrl();
        }
        break;
        case HttpOps::VerifyThePurchaseToken:
        {
            SetValueForKey("packageName", appSettingFiles->mPackageName);
            SetValueForKey("token", appSettingFiles->mPurchaseToken);// txtAccessToken->Text);
            SetValueForKey("productId", appSettingFiles->mSku);
            txtBuiltUrl->Text = UpdateUrl();
        }
        break;
        case HttpOps::ConsumePurchases:
        {
            SetValueForKey("packageName", appSettingFiles->mPackageName);
            SetValueForKey("token", appSettingFiles->mPurchaseToken);// txtAccessToken->Text);
            SetValueForKey("productId", appSettingFiles->mSku);
            txtBuiltUrl->Text = UpdateUrl();
        }
        break;
        case HttpOps::AcknowledgeThePurchase:
        {
            SetValueForKey("packageName", appSettingFiles->mPackageName);
            SetValueForKey("token", appSettingFiles->mPurchaseToken);
            SetValueForKey("productId", appSettingFiles->mSku);
            txtBuiltUrl->Text = UpdateUrl();
        }
        break;

        default:
            break;
        }
        OnGameListSelected(sender, e);
    }

    /*
     * httpclient server to GetSkusDetails call
    */
    void GameApplication::GetSkusDetails()
    {
        String^ url = txtBuiltUrl->Text;
        Console::WriteLine("GetSkusDetails\nurl :{0} ", url);
        HttpClient^ client = gcnew HttpClient();
        Task<HttpResponseMessage^>^ responseTask = client->GetAsync(url);
        responseTask->Wait();
        HttpResponseMessage^ response = responseTask->Result;
        if (!response->IsSuccessStatusCode) {
            String^ errorStatus = String::Format("HTTP Error: {0}", response->StatusCode);
            txtResult->Text = errorStatus;
            Console::WriteLine(errorStatus);
            return;
        }
        Task<String^>^ readTask = response->Content->ReadAsStringAsync();
        readTask->Wait();
        String^ readData = readTask->Result;
        if (readData->Contains("false")) {
            String^ errorStatus = String::Format("Error data return for API: {0}", readData);
            txtResult->Text = errorStatus;
            Console::WriteLine(errorStatus);
            return;
        }
        if (!readData->Contains("http")) {
            String^ errorStatus = String::Format("Error data return for API: {0}", readData);
            txtResult->Text = errorStatus;
            Console::WriteLine(errorStatus);
            return;
        }
        String^ res = GetSkuObj(readData);
        Console::WriteLine("Response: {0}", res);
        txtResult->Text = res;
        //WriteResponse(mContext->Response, res, 200);
        System::Diagnostics::Process::Start(res);
    }

    /*
     * httpclient server to GetPurchasesDetails call
    */
    void GameApplication::GetPurchasesDetails()
    {
        appSettingFiles->mPurchaseToken = "";
        String^ url = txtBuiltUrl->Text;
        Console::WriteLine("GetPurchasesDetails\nurl :{0} ", url);
        HttpClient^ client = gcnew HttpClient();
        Task<HttpResponseMessage^>^ responseTask = client->GetAsync(url);
        responseTask->Wait();
        HttpResponseMessage^ response = responseTask->Result;
        if (!response->IsSuccessStatusCode) {
            String^ errorStatus = String::Format("HTTP Error: {0}", response->StatusCode);
            txtResult->Text = errorStatus;
            Console::WriteLine(errorStatus);
            return;
        }
        Task<String^>^ readTask = response->Content->ReadAsStringAsync();
        readTask->Wait();
        String^ readData = readTask->Result;
        if (!readData->Contains("purchasesresult")) {
            String^ errorStatus = String::Format("Error data return for API: {0}", readData);
            txtResult->Text = errorStatus;
            Console::WriteLine(errorStatus);
            return;
        }
        Console::WriteLine("Response: {0}", readData);
        MemoryStream^ stream = gcnew MemoryStream();
        StreamWriter^ writer = gcnew StreamWriter(stream);
        writer->Write(readData);
        writer->Flush();
        stream->Position = 0;
        DataContractJsonSerializer^ serializer = gcnew DataContractJsonSerializer(RootObject::typeid);
        RootObject^ result = (RootObject^)serializer->ReadObject(stream);
        Purchase^ purchase = nullptr;
        if (result != nullptr && result->purchasesresult != nullptr && result->purchasesresult->resources != nullptr) {
            for (int i = 0; i < result->purchasesresult->resources->Length; ++i)
            {
                purchase = result->purchasesresult->resources[i];
                Console::WriteLine("Order ID: " + purchase->orderId);
                Console::WriteLine("Product ID: " + purchase->productId);
            }
        }
        String^ message = "No purchases found.";
        if (purchase != nullptr)
        {
            appSettingFiles->mPurchaseToken = purchase->purchaseToken;
            message = "Purchase Token: " + appSettingFiles->mPurchaseToken;
        }
        txtResult->Text = message;
        this->Activate();
    }

    /*
    * httpclient server to PostConsumePurchase call
   */
    void GameApplication::PostConsumePurchase()
    {
        String^ url = txtBuiltUrl->Text;
        Console::WriteLine("PostConsumePurchase\nurl :{0} ", url);
        HttpClient^ client = gcnew HttpClient();
        String^ message = "";
        Console::WriteLine("PostValidatePurchase\nurl :{0} ", url);
        if (appSettingFiles->mPurchaseToken == "") {
            message = "Error: Purchases Token is not found";
            Console::WriteLine(message);
            txtResult->Text = message;
            return;
        }
        String^ postUrl = url->Substring(0, url->IndexOf("?"));
        String^ jsonData = Serialize(gcnew RequestData(
            appSettingFiles->mSku, appSettingFiles->mPurchaseToken));
        Console::WriteLine("Request data content: {0}", jsonData);
        HttpRequestMessage^ requestMessage = gcnew HttpRequestMessage(HttpMethod::Post, postUrl);
        requestMessage->Content = gcnew StringContent(jsonData, Encoding::UTF8, "application/json");
        Task<HttpResponseMessage^>^ responseTask = client->SendAsync(requestMessage);
        HttpResponseMessage^ response = responseTask->Result;
        String^ responseBody = response->Content->ReadAsStringAsync()->Result;

        if (!response->IsSuccessStatusCode) {
            String^ errorStatus = String::Format("HTTP Error: {0}", response);
            txtResult->Text = errorStatus;
            Console::WriteLine(errorStatus);
            return;
        }
        Task<String^>^ readTask = response->Content->ReadAsStringAsync();
        readTask->Wait();
        String^ readData = readTask->Result;
        Console::WriteLine("Return data: {0}", readData);
        txtResult->Text = readData;
    }

    /*
     * httpclient server to PostValidatePurchase call
    */
    void GameApplication::PostValidatePurchase()
    {
        String^ url = txtBuiltUrl->Text;
        HttpClient^ client = gcnew HttpClient();
        String^ message = "";
        Console::WriteLine("PostValidatePurchase\nurl :{0} ", url);
        if (appSettingFiles->mPurchaseToken == "") {
            message = "Error: Purchases Token is not found";
            Console::WriteLine(message);
            txtResult->Text = message;
            return;
        }
        String^ jsonData = Serialize(gcnew RequestData(
            appSettingFiles->mSku, appSettingFiles->mPurchaseToken));
        Console::WriteLine("Request data content: {0}", jsonData);
        HttpRequestMessage^ requestMessage = gcnew HttpRequestMessage(HttpMethod::Post, url);
        requestMessage->Content = gcnew StringContent(jsonData, Encoding::UTF8, "application/json");
        Task<HttpResponseMessage^>^ responseTask = client->SendAsync(requestMessage);
        HttpResponseMessage^ response = responseTask->Result;
        String^ responseBody = response->Content->ReadAsStringAsync()->Result;

        if (!response->IsSuccessStatusCode) {
            String^ errorStatus = String::Format("HTTP Error: {0}", response);
            txtResult->Text = errorStatus;
            Console::WriteLine(errorStatus);
            return;
        }
        Task<String^>^ readTask = response->Content->ReadAsStringAsync();
        readTask->Wait();
        String^ readData = readTask->Result;
        Console::WriteLine("Return data: {0}", readData);
        txtResult->Text = readData;
    }

    /*
     * httpclient server to GetValidatePurchase call
    */
    void GameApplication::GetValidatePurchase()
    {
        String^ url = txtBuiltUrl->Text;
        HttpClient^ client = gcnew HttpClient();

        Console::WriteLine("GetValidatePurchase\nurl :{0} ", url);
        if (appSettingFiles->mPurchaseToken == "") {
            Console::WriteLine("Error: Purchases Token is not found");
            return;
        }
        String^ jsonData = Serialize(gcnew RequestData(
            appSettingFiles->mSku, appSettingFiles->mPurchaseToken));
        Console::WriteLine("Request data content: {0}", jsonData);
        //HttpRequestMessage^ requestMessage = gcnew HttpRequestMessage(HttpMethod::Post, url);
        //requestMessage->Content = gcnew StringContent(jsonData, Encoding::UTF8, "application/json");
        //Task<HttpResponseMessage^>^ responseTask = client->SendAsync(requestMessage);
        Task<HttpResponseMessage^>^ responseTask = client->GetAsync(url);
        responseTask->Wait();
        HttpResponseMessage^ response = responseTask->Result;
        //String^ responseBody = response->Content->ReadAsStringAsync()->Result;

        if (!response->IsSuccessStatusCode) {
            String^ errorStatus = String::Format("HTTP Error: {0}", response);
            txtResult->Text = errorStatus;
            Console::WriteLine(errorStatus);
            return;
        }
        Task<String^>^ readTask = response->Content->ReadAsStringAsync();
        readTask->Wait();
        String^ readData = readTask->Result;
        Console::WriteLine("Return data: {0}", readData);
        txtResult->Text = readData;
        //if (readData->Contains("false")) {
        //    String^ errorStatus = String::Format("Error data return for API: {0}", readData);
        //    txtResult->Text = errorStatus;
        //    Console::WriteLine(errorStatus);
        //    return;
        //}
        //if (!readData->Contains("http")) {
        //    String^ errorStatus = String::Format("Error data return for API: {0}", readData);
        //    txtResult->Text = errorStatus;
        //    Console::WriteLine(errorStatus);
        //    return;
        //}
        //String^ res = GetSkuObj(readData);
        //Console::WriteLine("Response: {0}", res);
        //txtResult->Text = res;
        //WriteResponse(mContext->Response, res, 200);
        //System::Diagnostics::Process::Start(res);
    }

    /*
     * httpclient server to GetAcknowledgePurchase call
    */
    void GameApplication::GetAcknowledgePurchase()
    {
        String^ url = txtBuiltUrl->Text;
        Console::WriteLine("GetAcknowledgePurchase\nurl :{0} ", url);
        HttpClient^ client = gcnew HttpClient();
        String^ message = "";
        Console::WriteLine("PostValidatePurchase\nurl :{0} ", url);
        if (appSettingFiles->mPurchaseToken == "") {
            message = "Error: Purchases Token is not found";
            Console::WriteLine(message);
            txtResult->Text = message;
            return;
        }
        String^ postUrl = url->Substring(0, url->IndexOf("?"));
        String^ jsonData = Serialize(gcnew RequestData(
            appSettingFiles->mSku, appSettingFiles->mPurchaseToken));
        Console::WriteLine("Request data content: {0}", jsonData);
        HttpRequestMessage^ requestMessage = gcnew HttpRequestMessage(HttpMethod::Post, postUrl);
        requestMessage->Content = gcnew StringContent(jsonData, Encoding::UTF8, "application/json");
        Task<HttpResponseMessage^>^ responseTask = client->SendAsync(requestMessage);
        HttpResponseMessage^ response = responseTask->Result;
        String^ responseBody = response->Content->ReadAsStringAsync()->Result;

        if (!response->IsSuccessStatusCode) {
            String^ errorStatus = String::Format("HTTP Error: {0}", response);
            txtResult->Text = errorStatus;
            Console::WriteLine(errorStatus);
            return;
        }
        Task<String^>^ readTask = response->Content->ReadAsStringAsync();
        readTask->Wait();
        String^ readData = readTask->Result;
        Console::WriteLine("Return data: {0}", readData);
        txtResult->Text = readData;
    }


    int GameApplication::SetValueForKey(String^ key, String^ value)
    {
        int i = -1;
        for (i = 0; i < tableParameters->RowCount; i++)
        {
            String^ d = tableParameters->GetControlFromPosition(0, i)->Text;
            //Console::WriteLine("table data - {0}", d);
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
                    parameter->ReadOnly;
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
            Console::WriteLine("table data - {0}", d);
            if (d == key) {
                return tableParameters->GetControlFromPosition(1, i)->Text;
            }
        }
        return "";
    }

};