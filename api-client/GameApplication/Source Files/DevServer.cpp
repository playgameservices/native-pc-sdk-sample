#include "pch.h"
#include "DevServer.h"

using namespace System::Threading::Tasks;

using namespace System;
using namespace System::IO;
using namespace System::Security::Cryptography;
using namespace System::Net;
using namespace System::Threading;
using namespace System::Net::Sockets;

namespace GameApplication {
    using namespace System;
    using namespace System::Net;

    int HttpListenerThread::GetRandomUnusedPort()
    {
        TcpListener^ listener = gcnew TcpListener(IPAddress::Loopback, 0);
        listener->Start();
        int port = ((IPEndPoint^)listener->LocalEndpoint)->Port;
        listener->Stop();
        return port;
    }

    HttpListenerThread::HttpListenerThread() {
        mPrefix = String::Format("http://{0}:{1}/", IPAddress::Loopback, GetRandomUnusedPort());
        mListenerOnThread = gcnew HttpListener();
        mListenerOnThread->Prefixes->Add(mPrefix);
        stopRequested = false;
        Console::WriteLine("HttpListenerThread prefix: {0}", mPrefix);
    }

    void HttpListenerThread::StartListening() {
        try {
            mListenerOnThread->Start();
            Console::WriteLine("HttpListenerThread::StartListening()-Listener started at {0}", mListenerOnThread->Prefixes->ToString());//listener->Prefixes[0]);

            while (!stopRequested) {
                if (mListenerOnThread->IsListening) {
                    try {
                        // Wait for a client request
                        mContext = mListenerOnThread->GetContext(); // Blocks until a request is received
                        HandleRequest(mContext);
                    }
                    catch (Exception^ ex) {
                        Console::WriteLine("Error handling request: {0}", ex->Message);
                    }

                }
            }
            Console::WriteLine("Listener stopped.");
        }
        catch (Exception^ ex) {
            Console::WriteLine("Error: {0}", ex->Message);
        }
    }

    void HttpListenerThread::Stop() {
        stopRequested = true;
    }

    void HttpListenerThread::WriteResponse(HttpListenerResponse^ response, String^ content, int statusCode)
    {
        array<Byte>^ buffer = System::Text::Encoding::UTF8->GetBytes(content);
        response->StatusCode = statusCode;
        response->ContentLength64 = buffer->Length;
        response->OutputStream->Write(buffer, 0, buffer->Length);
        response->OutputStream->Close();
    }

    String^ HttpListenerThread::PerformCodeExchange(String^ code)
    {
        HttpClient^ client = gcnew HttpClient();
        Dictionary<String^, String^>^ formData = gcnew Dictionary<String^, String^>();
        String^ clientID = (sharedData != nullptr && sharedData->sdConfig != nullptr)
            ? sharedData->sdConfig->client_id
            : "";
        String^ clientSecret = (sharedData != nullptr && sharedData->sdConfig != nullptr)
            ? sharedData->sdConfig->client_secret
            : "";
        String^ verifier = (sharedData != nullptr && !String::IsNullOrEmpty(sharedData->codeVerifier))
            ? sharedData->codeVerifier
            : "";

        formData->Add("code", code);
        formData->Add("redirect_uri", mPrefix);
        formData->Add("client_id", clientID);
        formData->Add("code_verifier", verifier);
        formData->Add("grant_type", "authorization_code");
        if (!String::IsNullOrEmpty(clientSecret) && clientSecret != "<your-client-secret>") {
            formData->Add("client_secret", clientSecret);
        }

        FormUrlEncodedContent^ content = gcnew FormUrlEncodedContent(formData);
        HttpResponseMessage^ response = client->PostAsync(sharedData->sdConfig->tokenEndpoint, content)->Result;
        String^ retToken = response->Content->ReadAsStringAsync()->Result;
        sharedData->sdApiResponse = retToken;

        return retToken;
    }

    String^ HttpListenerThread::HttpGetCall(String^ url, bool bearer, bool serviceToken)
    {
        HttpResponseMessage^ response = nullptr;
        try {
            String^ escapeUrl = Uri::EscapeDataString(url);
            //Console::WriteLine("Escape URL {0}", escapeUrl);
            Console::WriteLine("Get url\n url :{0} \naccess token :{1}", url, sharedData->sdServiceAccessToken);
            cli::array<String^>^ urlparts = url->Split('?');
            Uri^ uri = gcnew Uri(urlparts[0]);
            urlparts[1] = "?" + urlparts[1];
            Console::WriteLine("urlparts: {0}\nparameter:{1}", urlparts[0], urlparts[1]);
            HttpClient^ client = gcnew HttpClient();
            client->BaseAddress = uri;

            Task<HttpResponseMessage^>^ responseTask = client->GetAsync(escapeUrl);//"&" + urlparts[1]);
            responseTask->Wait();
            response = responseTask->Result;
            if (response->IsSuccessStatusCode)
            {
                Task<String^>^ readTask = response->Content->ReadAsStringAsync();
                readTask->Wait();
                Console::WriteLine("Response: {0}", readTask->Result);
            }
            else
            {
                Console::WriteLine("HTTP Error: {0}", response->StatusCode);
            }
        }
        catch (Exception^ e) {
            Console::WriteLine("Exception in {0}", e->Message);
        }
        return response->ToString();
    }

    String^ HttpListenerThread::GetSKUsDetails(String^ url, bool bearer)
    {
        Console::WriteLine("GetSKUsDetails\nuser url :{0} \naccess token :{1}", url, sharedData->sdServiceAccessToken);
        HttpClient^ client = gcnew HttpClient();
        Task<HttpResponseMessage^>^ responseTask = client->GetAsync(url);
        responseTask->Wait();
        HttpResponseMessage^ response = responseTask->Result;

        if (response->IsSuccessStatusCode)
        {
            Task<String^>^ readTask = response->Content->ReadAsStringAsync();
            readTask->Wait();
            String^ res = GetSkuObj(readTask->Result);
            Console::WriteLine("Response: {0}", res);
            WriteResponse(mContext->Response, res, 200);
            System::Diagnostics::Process::Start(res);
        }
        else
        {
            Console::WriteLine("HTTP Error: {0}", response->StatusCode);
        }
        return response->ToString();
    }


    // Callback function to handle the response data
    size_t HttpListenerThread::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
        size_t totalSize = size * nmemb;
        output->append(static_cast<char*>(contents), totalSize);
        return totalSize;
    }

    // Function to perform HTTP GET using libcurl
    void HttpListenerThread::PerformCurlPurchasesRequest(const std::string& url, const std::string& bearerToken) {
        
    }

    String^ HttpListenerThread::GetPurchasesDetails(String^ url, bool bearer)
    {
       Console::WriteLine("GetPurchases\nuser url :{0} \naccess token :{1}", url, sharedData->sdServiceAccessToken);
        HttpClient^ client = gcnew HttpClient();
       /* client->DefaultRequestHeaders->Authorization = gcnew
                Headers::AuthenticationHeaderValue("Bearer", sharedData->sdServiceAccessToken);*/

        Task<HttpResponseMessage^>^ responseTask = client->GetAsync(url);
        responseTask->Wait();
        HttpResponseMessage^ response = responseTask->Result;

        if (response->IsSuccessStatusCode)
        {
            Task<String^>^ readTask = response->Content->ReadAsStringAsync();
            readTask->Wait();
            String^ res = GetSkuObj(readTask->Result);
            Console::WriteLine("Response: {0}", res);
            WriteResponse(mContext->Response, res, 200);
            System::Diagnostics::Process::Start(res);
        }
        else
        {
            Console::WriteLine("HTTP Error: {0}", response->StatusCode);
        }
        return response->ToString();
    }


    void HttpListenerThread::HandleRequest(HttpListenerContext^ context) {
        try
        {
            mContext = context;
            String^ urlPath = mContext->Request->Url->AbsolutePath;
            String^ receivedCode = mContext->Request->QueryString->Get("code");
            if (!String::IsNullOrEmpty(receivedCode))
            {
                String^ receivedState = mContext->Request->QueryString->Get("state");
                if (String::IsNullOrEmpty(receivedState) || receivedState != sharedData->mState)
                {
                    WriteResponse(mContext->Response, "OAuth state validation failed (CSRF check).", 400);
                    return;
                }

                PerformCodeExchange(receivedCode);
                WriteResponse(
                    mContext->Response,
                    "<html><body><h3>Authentication complete. You may close this browser tab and return to the sample app.</h3></body></html>",
                    200);
            }
            else if (urlPath->Equals("/", StringComparison::OrdinalIgnoreCase))
            {
                WriteResponse(context->Response, ReadFile("index.htm"), 200);
            }
            else
            {
                WriteResponse(context->Response, "Invalid endpoint.", 404);
            }
        }
        catch (Exception^ ex)
        {
            Console::WriteLine("Request Handling Error: {0}", ex->Message);
            WriteResponse(context->Response, "Internal Server Error", 500);
        }
    }
    
    DevServer::DevServer(SharedData^ sd)
    {
        prefix = String::Format("http://{0}:{1}/", IPAddress::Loopback, GetRandomUnusedPort());
        Console::Write("Address {0}", prefix);
        sharedData = sd;
        sharedData->sdLocalhostAddress = prefix;
        mlistener = gcnew HttpListener();
        mlistener->Prefixes->Add(prefix);
        DevThreadStart();
        //Start();
    }
    DevServer::~DevServer() 
    {
        DevThreadStop();
    }
	void DevServer::RunServer()
	{
        mlistener->Start();
	}

    void DevServer::DevThreadStop()
    {
        mlistener->Stop();
        if (devThread != nullptr && devThread->IsAlive)
        {
            devThread->Join();
        }
    }

    void DevServer::DevThreadStart() {
        devThread = gcnew Thread(gcnew ThreadStart(this, &DevServer::Start));//RunServer));
        devThread->IsBackground = true;
        devThread->Start();
        Console::WriteLine("Server running in a worker thread.");
    }

    void DevServer::UpdateSharedData(SharedData^ data)
    {
        Monitor::Enter(lockObject);
        try
        {
            sharedData = data;
            Console::WriteLine("Shared data updated: {0}", sharedData);
        }
        finally
        {
            Monitor::Exit(lockObject);
        }
    }

    //Listener functions
    int DevServer::GetRandomUnusedPort()
    {
        TcpListener^ listener = gcnew TcpListener(IPAddress::Loopback, 0);
        listener->Start();

        int port = ((IPEndPoint^)listener->LocalEndpoint)->Port;

        listener->Stop();
        return port;
    }

    void DevServer::WriteResponse(HttpListenerResponse^ response, String^ content, int statusCode)
    {
        array<Byte>^ buffer = System::Text::Encoding::UTF8->GetBytes(content);
        response->StatusCode = statusCode;
        response->ContentLength64 = buffer->Length;
        response->OutputStream->Write(buffer, 0, buffer->Length);
        response->OutputStream->Close();
    }
    void DevServer::Start()
    {
        try
        {
            mlistener->Start();
            Console::WriteLine("DevServer::Start()-  mlistener->Prefixes->ToString(): {0}", mlistener->Prefixes->ToString());
            Console::WriteLine("DevServer::Start()- Server started. Listening at {0}", prefix);
           // return;
            while (!stopRequested)
            {
                if (!mlistener->IsListening)
                    break;
                //IAsyncResult^ contextResult = mlistener->BeginGetContext(nullptr, nullptr);
                IAsyncResult^ contextResult = mlistener->BeginGetContext(nullptr, sharedData->mState);
                if (contextResult->AsyncWaitHandle->WaitOne(100))
                {
                    // Process the request
                    mContext = mlistener->EndGetContext(contextResult);
                    mRequest = mContext->Request;
                    mResponse = mContext->Response;

                    // Write a response
                    String^ responseString = "Hello from C++/CLI HttpListener!";
                    array<Byte>^ buffer = System::Text::Encoding::UTF8->GetBytes(responseString);
                    mResponse->ContentLength64 = buffer->Length;
                    mResponse->OutputStream->Write(buffer, 0, buffer->Length);
                    mResponse->OutputStream->Close();
                    Console::WriteLine("Request handled.");
                }
                mContext = mlistener->GetContext();
                HandleRequest(mContext);
            }
        }
        catch (Exception^ ex)
        {
            Console::WriteLine("Server Error: {0}", ex->Message);
        }
        finally
        {
            Stop();
        }
    }

    void DevServer::Stop()
    {
        if (mlistener->IsListening)
        {
            mlistener->Stop();
        }
        Console::WriteLine("Server stopped.");
    }

    void DevServer::HandleRequest(HttpListenerContext^ context)
    {
        try
        {
            String^ urlPath = context->Request->Url->AbsolutePath;


            if (urlPath->Equals("/getsku", StringComparison::OrdinalIgnoreCase))
            {
                WriteResponse(context->Response, "Mock SKU data", 200);
            }
            else if (urlPath->Equals("/getpurchases", StringComparison::OrdinalIgnoreCase))
            {
                WriteResponse(context->Response, "Mock purchase data", 200);
            }
            else if (urlPath->Equals("/", StringComparison::OrdinalIgnoreCase))
            {
                WriteResponse(context->Response, ReadFile("index.htm"), 200);
            }
            else
            {
                WriteResponse(context->Response, "Invalid endpoint.", 404);
            }
        }
        catch (Exception^ ex)
        {
            Console::WriteLine("Request Handling Error: {0}", ex->Message);
            WriteResponse(context->Response, "Internal Server Error", 500);
        }
    }

};
