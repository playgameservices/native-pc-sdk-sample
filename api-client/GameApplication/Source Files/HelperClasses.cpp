#include "pch.h"
#include "HelperClasses.h"
#include <fstream>

//#include <google/cloud/access_token.h>

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
            String^ content = reader->ReadToEnd();
            reader->Close();
            return content;
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
        // Deserialize the JSON data
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

    String^ GetSkuObj(String^ jsonString)
    {
        String^ res = "";
        Match^ nameMatch = Regex::Match(jsonString, "\"deepLink\"\\s*:\\s*\"([^\"]*)\"");
        if (nameMatch->Success) {
            res = nameMatch->Groups[1]->ToString();
        }
        return res;
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
    }

  

    Dictionary<String^, String^>^ ParseQueryParameters(String^ url) {
        try {
            // Create a Uri instance from the URL
            Uri^ uri = gcnew Uri(url);

            // Extract the query string (removing the '?' character)
            String^ query = uri->Query->TrimStart('?');

            // Check if the query string is empty
            if (String::IsNullOrWhiteSpace(query)) {
                Console::WriteLine("No query parameters found.");
                return nullptr;
            }

            // Split the query string into key-value pairs
            array<String^>^ pairs = query->Split('&');
            Dictionary<String^, String^>^ queryParams = gcnew Dictionary<String^, String^>(1);

            // Iterate through each key-value pair
            for each (String ^ pair in pairs) {
                array<String^>^ keyValue = pair->Split('=');
                if (keyValue->Length == 2) {
                    String^ key = Uri::UnescapeDataString(keyValue[0]);
                    String^ value = Uri::UnescapeDataString(keyValue[1]);
                    queryParams->Add(key, value);
                }
                else if (keyValue->Length == 1) {
                    // Handle keys with no value
                    String^ key = Uri::UnescapeDataString(keyValue[0]);
                    queryParams->Add(key, "");
                }
            }

            // Output the parsed query parameters
            for each (KeyValuePair<String^, String^> kvp in queryParams) {
                Console::WriteLine("{0}: {1}", kvp.Key, kvp.Value);
            }
            return queryParams;
        }
        catch (UriFormatException^ ex) {
            Console::WriteLine("Invalid URL: {0}", ex->Message);
        }
        catch (Exception^ ex) {
            Console::WriteLine("An error occurred: {0}", ex->Message);
        }
    }

    String^ GetServiceAccountToken()
    {
        String^ output = nullptr;
        try {
            Process^ process = gcnew Process();
            process->StartInfo->FileName = "cmd.exe";  // Running cmd.exe
            process->StartInfo->Arguments = "/C gcloud auth print-access-token --impersonate-service-account=<project-number>-compute@developer.gserviceaccount.com --project=<project-name> "; // Command to run  --scopes=https://www.googleapis.com/auth/androidpublisher,https://www.googleapis.com/auth/cloud-platform,openid,https://www.googleapis.com/auth/userinfo.email
            process->StartInfo->RedirectStandardOutput = true;
            process->StartInfo->RedirectStandardError = true;
            process->StartInfo->UseShellExecute = false;
            process->StartInfo->CreateNoWindow = true;

            if (!process->Start()) {
                Console::WriteLine("Failed to start process.");
                return nullptr;
            }
            output = process->StandardOutput->ReadToEnd();
            String^ error = process->StandardError->ReadToEnd();
            process->WaitForExit();
        }
        catch (Exception^ ex) {
            Console::WriteLine("An error occurred: {0}", ex->Message);
            return nullptr;
        }
        Console::WriteLine("length of token {0} token {1}", output->Length, output);
        output = output->Substring(0, output->Length - 1);
        Console::WriteLine("length of token {0} token {1}", output->Length,output);
        return output;
    }

    String^ GetServerAccessToken(String^ userToken)
    {
        String^ url = "https://iamcredentials.googleapis.com/v1/projects/-/serviceAccounts/multiplatform-demo@vx-pse-development.iam.gserviceaccount.com:generateAccessToken";
        HttpClient^ client = gcnew HttpClient();
        HttpRequestMessage^ request = gcnew HttpRequestMessage(HttpMethod::Get, url);
        //request->Headers->Add("charset", "text/html; charset=UTF-8");
        request->Headers->Add("Authorization", "Bearer " + userToken);
        String^ data = ReadFile("userToken.json");
        request->Content = gcnew StringContent(data, Encoding::UTF8, "application/json");
        //response = client->SendAsync(request)->Result;
        //HttpResponseMessage^ response = client->SendAsync(request)->Result;
        //Task<HttpResponseMessage^>^ responseTask = client->SendAsync(request);
        Task<HttpResponseMessage^>^ responseTask = client->GetAsync(url);
        responseTask->Wait();
        HttpResponseMessage^ response = responseTask->Result;
        if (response->IsSuccessStatusCode)
        {
            Task<String^>^ readTask = response->Content->ReadAsStringAsync();
            readTask->Wait();
            Console::WriteLine("Response: {0}", readTask->Result);
        }
        else
        {
            Console::WriteLine("HTTP Error: {0}\n{1}", response->StatusCode,response->ToString());
        }
        return response->ToString();

    }

};