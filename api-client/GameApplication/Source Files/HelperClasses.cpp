#include "pch.h"
#include "HelperClasses.h"
#include <fstream>

using namespace System::Threading::Tasks;
using namespace System::Text::RegularExpressions;
using namespace System::Net::NetworkInformation;
using namespace System::Diagnostics;

namespace GameApplication {
	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Net;
	using namespace System::Net::Sockets;
	using namespace System::Collections::Generic;
	using namespace System::Net::Http;
    using namespace System::Text;
    using namespace System::Runtime::Serialization::Json;
    using namespace System::Runtime::Serialization;
    using namespace System::IO;

    RequestData::RequestData()
    {
    }

    RequestData::RequestData(String^ ssku, String^ stoken)
    {
        productId = ssku;
        sku = ssku;
        token = stoken;
        packageName = nullptr;
    }

    RequestData::RequestData(String^ ssku, String^ stoken, String^ spkg)
    {
        productId = ssku;
        sku = ssku;
        token = stoken;
        packageName = spkg;
    }

    List<String^>^ ExtractCurlyBracesBlocks(String^ data)
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

    String^ ReadFile(String^ filePath) {
        if (String::IsNullOrEmpty(filePath)) {
            throw gcnew ArgumentException("File path cannot be null or empty.");
        }

        if (!File::Exists(filePath)) {
            throw gcnew FileNotFoundException("The specified file does not exist.", filePath);
        }

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
            throw gcnew IOException("An error occurred while reading the file: " + ex->Message, ex);
        }
    }

    List<GameApis^>^ ReadApis(String^ fileName)
    {
        String^ fileData = ReadFile(fileName);
        MemoryStream^ stream = gcnew MemoryStream(System::Text::Encoding::UTF8->GetBytes(fileData));
        DataContractJsonSerializer^ serializer = gcnew DataContractJsonSerializer(List<GameApis^>::typeid);
        List<GameApis^>^ apiList = dynamic_cast<List<GameApis^>^>(serializer->ReadObject(stream));
        return apiList;
    }

    Config^ ReadConfig(String^ fileName)
    {
        String^ fileData = ReadFile(fileName);
        MemoryStream^ stream = gcnew MemoryStream(System::Text::Encoding::UTF8->GetBytes(fileData));
        DataContractJsonSerializer^ serializer = gcnew DataContractJsonSerializer(Config::typeid);
        Config^ config = static_cast<Config^>(serializer->ReadObject(stream));
        return config;
    }

    Response^ ReadResponse(String^ jsonString)
    {
        MemoryStream^ stream = gcnew MemoryStream(System::Text::Encoding::UTF8->GetBytes(jsonString));
        DataContractJsonSerializer^ serializer = gcnew DataContractJsonSerializer(Response::typeid);
        Response^ response = static_cast<Response^>(serializer->ReadObject(stream));
        return response;
    }

    bool TryValidateAllowedHttpsUrl(String^ rawUrl, String^ expectedHost, Uri^% outUri)
    {
        outUri = nullptr;
        if (String::IsNullOrWhiteSpace(rawUrl) || String::IsNullOrWhiteSpace(expectedHost)) {
            return false;
        }
        Uri^ parsed = nullptr;
        if (!Uri::TryCreate(rawUrl->Trim(), UriKind::Absolute, parsed) || parsed == nullptr) {
            return false;
        }
        if (parsed->Scheme != Uri::UriSchemeHttps) {
            return false;
        }
        if (!String::IsNullOrEmpty(parsed->UserInfo)) {
            return false;
        }
        if (!parsed->Host->Equals(expectedHost, StringComparison::OrdinalIgnoreCase)) {
            return false;
        }
        outUri = parsed;
        return true;
    }

    String^ RedactTokenForLog(String^ secret)
    {
        if (String::IsNullOrEmpty(secret)) {
            return "(empty)";
        }
        return String::Format("<redacted:{0} chars>", secret->Length);
    }

    String^ GetSkuObj(String^ jsonString)
    {
        String^ rawLink = "";
        Match^ nameMatch = Regex::Match(jsonString, "\"deepLink\"\\s*:\\s*\"([^\"]*)\"");
        if (nameMatch->Success) {
            rawLink = nameMatch->Groups[1]->ToString();
        }
        Uri^ validatedUri = nullptr;
        if (!TryValidateAllowedHttpsUrl(rawLink, "play.google.com", validatedUri)) {
            Console::WriteLine("Rejected deepLink outside https://play.google.com allowlist.");
            return "";
        }
        return validatedUri->AbsoluteUri;
    }

    String^ GetLocalIPAddress()
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
                            Console::WriteLine("Interface: {0}", nic->Name);
                            Console::WriteLine("IPv4 Address: {0}", ipInfo->Address);
                            return ipInfo->Address->ToString();
                        }
                    }
                }
            }
        }
        catch (Exception^ ex)
        {
            Console::WriteLine("Error: {0}", ex->Message);
        }
        return "";
    }

};
