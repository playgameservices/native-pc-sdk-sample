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
using namespace System;

namespace GameApplication {

    AppSettingsFiles::AppSettingsFiles()
    {
        ParseManifestXml();
        GetLocalIPAddress();
        mConfig = ReadConfig("AppConfig.json");
        mGameApiList = ReadApis("GameApis.json");
        mState = GenerateRandomBase64Url(32);
    }

    void AppSettingsFiles::ParseManifestXml()
    {
        try {
            XmlReader^ reader = XmlReader::Create("manifest.xml");
            while (reader->Read()) {
                if (reader->NodeType == XmlNodeType::Element) {
                    String^ elementName = reader->Name;
                    if (reader->Read() && reader->NodeType == XmlNodeType::Text) {
                        if (elementName == "PackageName") {
                            mPackageName = reader->Value; //mPackageName
                        }
                        if (elementName == "SKU") {
                            mSku = reader->Value;//mSku
                        }
                    }
                }
            }
            Console::WriteLine("PackageNmae: {0} and Sku: {1}", mPackageName, mSku);
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
    }

    String^ AppSettingsFiles::ReadFile(String^ filePath) {
        try {
            StreamReader^ reader = gcnew StreamReader(filePath);
            String^ content = reader->ReadToEnd();
            reader->Close();
            return content;
        }
        catch (Exception^ ex) {
            Console::WriteLine("AppSettingsFiles::ReadFile - error occurred while reading the file: {0}", ex->Message);
        }
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
            //fileName = "GameApplication.Resources.AppConfig.json";
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
    //PKCE Functions start
    String^ AppSettingsFiles::GenerateRandomBase64Url(int length)
    {
        array<Byte>^ randomBytes = gcnew array<Byte>(length);
        RNGCryptoServiceProvider^ rng = gcnew RNGCryptoServiceProvider();
        rng->GetBytes(randomBytes);
        return Base64UrlEncodeNoPadding(randomBytes);
    }
    array<Byte>^ AppSettingsFiles::ComputeSha256(String^ input)
    {
        SHA256^ sha256 = SHA256::Create();
        array<Byte>^ inputBytes = Encoding::UTF8->GetBytes(input);
        return sha256->ComputeHash(inputBytes);
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