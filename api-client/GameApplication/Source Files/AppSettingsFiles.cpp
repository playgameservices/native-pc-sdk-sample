#include "pch.h"
#include <iostream>
#include "AppSettingsFiles.h"
#include "..\Resource.h"
#include <wtypes.h>

using namespace System::Security::Cryptography;
using namespace System::Net::NetworkInformation;
using namespace System::Reflection;
using namespace System;
using namespace System::Xml;
using namespace System::IO;

namespace GameApplication {

    AppSettingsFiles::AppSettingsFiles()
    {
        mBackendBaseUrl = "";
        mApiAuthToken = "";
        ParseManifestXml();
        GetLocalIPAddress();

        String^ envApiAuthToken = Environment::GetEnvironmentVariable("API_AUTH_TOKEN");
        if (!String::IsNullOrWhiteSpace(envApiAuthToken)) {
            mApiAuthToken = envApiAuthToken->Trim();
        }

        String^ exeDir = Path::GetDirectoryName(Assembly::GetExecutingAssembly()->Location);
        array<String^>^ cfgCandidates = gcnew array<String^>{
            "Resource Files\\AppConfig.local.json",
            Path::Combine(exeDir, "AppConfig.local.json"),
            Path::Combine(exeDir, "AppConfig.json"),
            "Resource Files\\AppConfig.json",
            "AppConfig.local.json",
            "AppConfig.json"
        };
        String^ chosenCfg = "Resource Files\\AppConfig.json";
        for each (String^ c in cfgCandidates) {
            if (File::Exists(c)) {
                chosenCfg = c;
                break;
            }
        }
        mConfig = ReadConfig(chosenCfg);
        if (mConfig != nullptr) {
            String^ envClientId = Environment::GetEnvironmentVariable("GOOGLE_OAUTH_CLIENT_ID");
            if (!String::IsNullOrWhiteSpace(envClientId)) {
                mConfig->client_id = envClientId->Trim();
            }
        }

        array<String^>^ apiCandidates = gcnew array<String^>{
            Path::Combine(exeDir, "GameApis.json"),
            "Resource Files\\GameApis.json",
            "GameApis.json"
        };
        String^ chosenApis = "Resource Files\\GameApis.json";
        for each (String^ c in apiCandidates) {
            if (File::Exists(c)) {
                chosenApis = c;
                break;
            }
        }
        mGameApiList = ReadApis(chosenApis);
        if (mGameApiList != nullptr && !String::IsNullOrEmpty(mBackendBaseUrl)) {
            String^ trimmedBase = mBackendBaseUrl->TrimEnd('/');
            for each (GameApis^ api in mGameApiList) {
                if (api != nullptr && !String::IsNullOrEmpty(api->URL)) {
                    api->URL = api->URL->Replace("https://your-project.web.app", trimmedBase);
                }
            }
        }
        RegenerateOAuthSecurityParams();
    }

    void AppSettingsFiles::RegenerateOAuthSecurityParams()
    {
        mState = GenerateRandomBase64Url(32);
        codeVerifier = GenerateRandomBase64Url(32);
        codeChallenge = ComputeCodeChallenge(codeVerifier);
    }

    void AppSettingsFiles::ParseManifestXml()
    {
        try {
            String^ exeDir = Path::GetDirectoryName(Assembly::GetExecutingAssembly()->Location);
            array<String^>^ manifestCandidates = gcnew array<String^>{
                "Resource Files\\manifest.local.xml",
                Path::Combine(exeDir, "manifest.local.xml"),
                Path::Combine(exeDir, "manifest.xml"),
                "Resource Files\\manifest.xml",
                "manifest.local.xml",
                "manifest.xml"
            };
            String^ chosenManifest = "Resource Files\\manifest.xml";
            for each (String^ c in manifestCandidates) {
                if (File::Exists(c)) {
                    chosenManifest = c;
                    break;
                }
            }
            XmlReader^ reader = XmlReader::Create(chosenManifest);
            try {
                while (reader->Read()) {
                    if (reader->NodeType == XmlNodeType::Element) {
                        String^ elementName = reader->Name;
                        if (reader->Read() && reader->NodeType == XmlNodeType::Text) {
                            if (elementName == "PackageName") {
                                mPackageName = reader->Value->Trim();
                            }
                            if (elementName == "SKU") {
                                String^ rawSku = reader->Value->Trim();
                                int commaIdx = rawSku->IndexOf(',');
                                mSku = (commaIdx > 0) ? rawSku->Substring(0, commaIdx)->Trim() : rawSku;
                            }
                            if (elementName == "BackendBaseUrl") {
                                mBackendBaseUrl = reader->Value->Trim();
                            }
                            if (elementName == "ApiAuthToken") {
                                mApiAuthToken = reader->Value->Trim();
                            }
                        }
                    }
                }
            }
            finally {
                reader->Close();
            }
            Console::WriteLine("PackageName: {0}, Sku: {1}", mPackageName, mSku);
        }
        catch (Exception^ ex)
        {
            Console::WriteLine("AppSettingsFiles::ParseManifestXml - Error: {0}", ex->Message);
        }
    }

    String^ AppSettingsFiles::GetLocalIPAddress()
    {
        try
        {
            array<NetworkInterface^>^ networkInterfaces = NetworkInterface::GetAllNetworkInterfaces();
            for each (NetworkInterface ^ nic in networkInterfaces)
            {
                if (nic->OperationalStatus == OperationalStatus::Up)
                {
                    IPInterfaceProperties^ ipProperties = nic->GetIPProperties();
                    for each (UnicastIPAddressInformation ^ ipInfo in ipProperties->UnicastAddresses)
                    {
                        if (ipInfo->Address->AddressFamily == System::Net::Sockets::AddressFamily::InterNetwork)
                        {
                            Console::WriteLine("IPv4 Address: {0}", ipInfo->Address);
                            mIPAddress = ipInfo->Address->ToString();
                            return mIPAddress;
                        }
                    }
                }
            }
        }
        catch (Exception^ ex)
        {
            Console::WriteLine("AppSettingsFiles::GetLocalIPAddress - Error: {0}", ex->Message);
        }
        return "";
    }

    String^ AppSettingsFiles::ReadFile(String^ filePath) {
        try {
            StreamReader^ reader = gcnew StreamReader(filePath);
            try {
                return reader->ReadToEnd();
            }
            finally {
                reader->Close();
            }
        }
        catch (Exception^ ex) {
            Console::WriteLine("AppSettingsFiles::ReadFile - error occurred while reading the file: {0}", ex->Message);
        }
        return "";
    }

    List<GameApis^>^ AppSettingsFiles::ReadApis(String^ fileName)
    {
        List<GameApis^>^ apiList = nullptr;
        try {
            String^ fileData = ReadFile(fileName);
            MemoryStream^ stream = gcnew MemoryStream(System::Text::Encoding::UTF8->GetBytes(fileData));
            DataContractJsonSerializer^ serializer = gcnew DataContractJsonSerializer(List<GameApis^>::typeid);
            apiList = dynamic_cast<List<GameApis^>^>(serializer->ReadObject(stream));
        }
        catch (Exception^ ex) {
            Console::WriteLine("AppSettingsFiles::ReadApis - error occurred while reading the file: {0}", ex->Message);
        }
        return apiList;
    }

    Config^ AppSettingsFiles::ReadConfig(String^ fileName)
    {
        Config^ config = nullptr;
        try {
            String^ fileData = ReadFile(fileName);
            MemoryStream^ stream = gcnew MemoryStream(System::Text::Encoding::UTF8->GetBytes(fileData));
            DataContractJsonSerializer^ serializer = gcnew DataContractJsonSerializer(Config::typeid);
            config = static_cast<Config^>(serializer->ReadObject(stream));
        }
        catch (Exception^ ex) {
            Console::WriteLine("AppSettingsFiles::ReadConfig - error occurred while reading the file: {0}", ex->Message);
        }
        return config;
    }

    String^ AppSettingsFiles::GenerateRandomBase64Url(int length)
    {
        array<Byte>^ randomBytes = gcnew array<Byte>(length);
        RNGCryptoServiceProvider^ rng = gcnew RNGCryptoServiceProvider();
        try {
            rng->GetBytes(randomBytes);
        }
        finally {
            delete rng;
        }
        return Base64UrlEncodeNoPadding(randomBytes);
    }

    array<Byte>^ AppSettingsFiles::ComputeSha256(String^ input)
    {
        SHA256^ sha256 = SHA256::Create();
        try {
            array<Byte>^ inputBytes = Encoding::UTF8->GetBytes(input);
            return sha256->ComputeHash(inputBytes);
        }
        finally {
            delete sha256;
        }
    }

    String^ AppSettingsFiles::Base64UrlEncodeNoPadding(array<Byte>^ bytes)
    {
        String^ base64 = Convert::ToBase64String(bytes);
        return base64->Replace("+", "-")->Replace("/", "_")->TrimEnd('=');
    }

    String^ AppSettingsFiles::ComputeCodeChallenge(String^ verifier)
    {
        array<Byte>^ hash = ComputeSha256(verifier);
        return Base64UrlEncodeNoPadding(hash);
    }

}
