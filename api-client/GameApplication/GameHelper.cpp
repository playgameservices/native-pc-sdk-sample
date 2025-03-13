#include "pch.h"
#include "GameHelper.h"
#include "GameApplication.h"
using namespace System::Text::RegularExpressions;
using namespace System::Xml;

namespace GameApplication {
    GameApplication::GameApplication(void)
    {
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
        LoadWindow(sender, e);
    }
    System::Void GameApplication::btnExecute_Click(System::Object^ sender, System::EventArgs^ e) {
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
            //SetValueForKey("redirect_uri", Uri::EscapeDataString(mRedirectUri));
            SetValueForKey("client_id", appSettingFiles->mConfig->client_id);
            SetValueForKey("state", appSettingFiles->mState);
            SetValueForKey("code_challenge", "");
            SetValueForKey("Scopes", appSettingFiles->mConfig->Scopes);
            //txtBuiltUrl->Text = UpdateUrl();
        }
        break;
        /*case HttpOps::GetToken:	//1
        {
            tabGetPage->Text = "Method - POST for GetToken";
            lblParameterPostData->Text = "Post Data:";
        }
        break;

        case HttpOps::GetUserDetails:	//2
        {
            tabGetPage->Text = "Method - GET for Get user details";
            lblParameterPostData->Text = "Parameters:";
            txtGetBuiltUrl->Text = mCurrentApi->URL;
        }
        break;
        case HttpOps::GetSKUs:	//3
        {
            tabGetPage->Text = "Method - GET for Get SKU";
            lblParameterPostData->Text = "Parameters:";
            SetTableData();
            SetValueForKey("packageName", mPackageName);
            SetValueForKey("languageCode", "en-US");
            SetValueForKey("ipAddress", GetLocalIPAddress());
            SetValueForKey("skuTypeName", "inapp");
            SetValueForKey("sku1", mSku);
            SetValueForKey("productId", mSku);
            SetValueForKey("token", txtAccessToken->Text);//mSd->sdServiceAccessToken);// txtAccessToken->Text);

            txtGetBuiltUrl->Text = UpdateUrl();
        }
        break;
        case HttpOps::GetPurchases:
        {
            tabGetPage->Text = "Method - GET for Get in app products SKU";
            lblParameterPostData->Text = "Parameters:";
            SetTableData();
            SetValueForKey("pName", mPackageName);
            SetValueForKey("token", mUserAccessToken);
            txtGetBuiltUrl->Text = UpdateUrl();
        }
        break;
        case HttpOps::VerifyThePurchaseToken:
        {
            tabGetPage->Text = "Method - Get inapp products";
            lblParameterPostData->Text = "Parameters:";
            SetTableData();
            SetValueForKey("packageName", mPackageName);
            SetValueForKey("token", mUserAccessToken);
            SetValueForKey("productId", mSku);
            txtGetBuiltUrl->Text = UpdateUrl();
        }
        break;
        case HttpOps::AcknowledgeThePurchase:
        {
            tabGetPage->Text = "Method - Get for Get SKUs details with IP";
            lblParameterPostData->Text = "Parameters:";
            SetTableData();
            SetValueForKey("packageName", mPackageName);
            SetValueForKey("token", mUserAccessToken);
            SetValueForKey("productId", mSku);
            txtGetBuiltUrl->Text = UpdateUrl();
        }
        break;
        case HttpOps::VoidedPurchases:
        {
            tabGetPage->Text = "Method - Get for Get SKUs details with IP";
            lblParameterPostData->Text = "Parameters:";
            SetTableData();
            SetValueForKey("packageName", mPackageName);
            SetValueForKey("token", mUserAccessToken);
            SetValueForKey("sku1", mSku);
            SetValueForKey("obfuscatedProfileId", "Enter the details");
            txtGetBuiltUrl->Text = UpdateUrl();
        }
        break;
        case HttpOps::SkuUpdate1:
        {
            tabGetPage->Text = "Method - Get for Get SKUs details with IP";
            lblParameterPostData->Text = "Parameters:";
            SetTableData();
            SetValueForKey("packageName", mPackageName);
            txtGetBuiltUrl->Text = UpdateUrl();
        }
        break;
        case HttpOps::SkuUpdate2:
        {
            tabGetPage->Text = "Method - Get for Get SKUs details with IP";
            lblParameterPostData->Text = "Parameters:";
            SetTableData();
            SetValueForKey("packageName", mPackageName);
            SetValueForKey("productId", mSku);
            SetValueForKey("token", mUserAccessToken);
            txtGetBuiltUrl->Text = UpdateUrl();
        }
        break;*/

        default:
            break;
        }
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