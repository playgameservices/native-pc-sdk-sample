#include "pch.h"
#include "DevServer.h"

using namespace System::Collections::Specialized;
using namespace System::Threading::Tasks;
using namespace System::Web;
using namespace System;
using namespace System::IO;
using namespace System::Security::Cryptography;
using namespace System::Net;
using namespace System::Threading;
using namespace System::Net::Sockets;

namespace GameApplication {

    HttpListenerThread::HttpListenerThread() {
        mTcpListener = nullptr;
        mLock = gcnew Object();
        stopRequested = false;
        sharedData = nullptr;
        mPrefix = "";
        mPort = 0;
    }

    void HttpListenerThread::BindLoopback() {
        Monitor::Enter(mLock);
        try {
            if (mTcpListener != nullptr) {
                try {
                    mTcpListener->Stop();
                }
                catch (Exception^) {
                }
                mTcpListener = nullptr;
            }
            stopRequested = false;
            mTcpListener = gcnew TcpListener(IPAddress::Loopback, 0);
            mTcpListener->Start();
            mPort = ((IPEndPoint^)mTcpListener->LocalEndpoint)->Port;
            mPrefix = String::Format("http://127.0.0.1:{0}/", mPort);
            Console::WriteLine("OAuth loopback listener bound at {0}", mPrefix);
        }
        finally {
            Monitor::Exit(mLock);
        }
    }

    bool HttpListenerThread::IsStopRequested() {
        Monitor::Enter(mLock);
        try {
            return stopRequested || mTcpListener == nullptr;
        }
        finally {
            Monitor::Exit(mLock);
        }
    }

    void HttpListenerThread::StartListening() {
        try {
            while (!IsStopRequested()) {
                TcpListener^ listener = nullptr;
                Monitor::Enter(mLock);
                try {
                    listener = mTcpListener;
                }
                finally {
                    Monitor::Exit(mLock);
                }
                if (listener == nullptr) {
                    break;
                }

                TcpClient^ client = nullptr;
                try {
                    client = listener->AcceptTcpClient();
                }
                catch (SocketException^) {
                    if (IsStopRequested()) {
                        break;
                    }
                    continue;
                }
                catch (ObjectDisposedException^) {
                    break;
                }

                if (client == nullptr) {
                    continue;
                }

                bool completed = false;
                try {
                    completed = HandleClient(client);
                }
                catch (Exception^ ex) {
                    Console::WriteLine("Error handling OAuth callback connection: {0}", ex->Message);
                }
                finally {
                    try {
                        client->Close();
                    }
                    catch (Exception^) {
                    }
                }

                if (completed) {
                    break;
                }
            }
        }
        finally {
            Stop();
        }
    }

    void HttpListenerThread::Stop() {
        Monitor::Enter(mLock);
        try {
            stopRequested = true;
            if (mTcpListener != nullptr) {
                try {
                    mTcpListener->Stop();
                }
                catch (Exception^) {
                }
                mTcpListener = nullptr;
            }
        }
        finally {
            Monitor::Exit(mLock);
        }
    }

    void HttpListenerThread::WriteHttpResponse(
        NetworkStream^ stream,
        int statusCode,
        String^ reasonPhrase,
        String^ content,
        String^ contentType)
    {
        array<Byte>^ bodyBytes = Encoding::UTF8->GetBytes(content);
        String^ header = String::Format(
            "HTTP/1.1 {0} {1}\r\n"
            "Content-Type: {2}\r\n"
            "Content-Length: {3}\r\n"
            "Cache-Control: no-store\r\n"
            "Connection: close\r\n\r\n",
            statusCode, reasonPhrase, contentType, bodyBytes->Length);
        array<Byte>^ headerBytes = Encoding::ASCII->GetBytes(header);
        stream->Write(headerBytes, 0, headerBytes->Length);
        stream->Write(bodyBytes, 0, bodyBytes->Length);
        stream->Flush();
    }

    bool HttpListenerThread::FixedTimeEquals(String^ left, String^ right) {
        if (left == nullptr || right == nullptr) {
            return false;
        }
        array<Byte>^ a = Encoding::UTF8->GetBytes(left);
        array<Byte>^ b = Encoding::UTF8->GetBytes(right);
        if (a->Length != b->Length || a->Length == 0) {
            return false;
        }
        int diff = 0;
        for (int i = 0; i < a->Length; ++i) {
            diff |= (a[i] ^ b[i]);
        }
        return diff == 0;
    }

    bool HttpListenerThread::HandleClient(TcpClient^ client) {
        client->ReceiveTimeout = 5000;
        client->SendTimeout = 5000;
        NetworkStream^ stream = client->GetStream();

        array<Byte>^ buffer = gcnew array<Byte>(8192);
        int totalRead = 0;
        String^ rawRequest = "";
        while (totalRead < buffer->Length) {
            int bytesRead = stream->Read(buffer, totalRead, buffer->Length - totalRead);
            if (bytesRead <= 0) {
                break;
            }
            totalRead += bytesRead;
            rawRequest = Encoding::ASCII->GetString(buffer, 0, totalRead);
            if (rawRequest->Contains("\r\n\r\n") || rawRequest->Contains("\n\n")) {
                break;
            }
        }

        if (String::IsNullOrWhiteSpace(rawRequest)) {
            WriteHttpResponse(stream, 400, "Bad Request", "Empty request.", "text/plain; charset=utf-8");
            return false;
        }

        int lineEnd = rawRequest->IndexOf('\n');
        String^ requestLine = (lineEnd >= 0) ? rawRequest->Substring(0, lineEnd)->Trim() : rawRequest->Trim();
        array<String^>^ parts = requestLine->Split(' ');
        if (parts->Length < 2) {
            WriteHttpResponse(stream, 400, "Bad Request", "Malformed HTTP request.", "text/plain; charset=utf-8");
            return false;
        }

        String^ method = parts[0];
        String^ target = parts[1];
        if (!method->Equals("GET", StringComparison::Ordinal)) {
            WriteHttpResponse(stream, 405, "Method Not Allowed", "Method not allowed.", "text/plain; charset=utf-8");
            return false;
        }

        String^ path = target;
        String^ query = "";
        int queryIdx = target->IndexOf('?');
        if (queryIdx >= 0) {
            path = target->Substring(0, queryIdx);
            query = target->Substring(queryIdx + 1);
        }

        if (!path->Equals("/", StringComparison::Ordinal)) {
            WriteHttpResponse(stream, 404, "Not Found", "Not found.", "text/plain; charset=utf-8");
            return false;
        }

        NameValueCollection^ queryParams = HttpUtility::ParseQueryString(query);
        String^ receivedCode = queryParams->Get("code");
        String^ receivedState = queryParams->Get("state");
        if (String::IsNullOrEmpty(receivedCode)) {
            WriteHttpResponse(stream, 404, "Not Found", "Not found.", "text/plain; charset=utf-8");
            return false;
        }

        if (sharedData == nullptr) {
            WriteHttpResponse(stream, 500, "Internal Server Error", "OAuth state unavailable.", "text/plain; charset=utf-8");
            return true;
        }

        String^ verifier = nullptr;
        bool stateMatches = false;
        Monitor::Enter(sharedData->syncRoot);
        try {
            if (!String::IsNullOrEmpty(sharedData->mState) &&
                FixedTimeEquals(receivedState, sharedData->mState)) {
                stateMatches = true;
                verifier = sharedData->codeVerifier;
                // Consume state and PKCE verifier immediately (single-use).
                sharedData->mState = "";
                sharedData->codeVerifier = "";
            }
        }
        finally {
            Monitor::Exit(sharedData->syncRoot);
        }

        if (!stateMatches) {
            WriteHttpResponse(
                stream,
                400,
                "Bad Request",
                "OAuth state validation failed.",
                "text/plain; charset=utf-8");
            // Keep listening for the legitimate browser redirect.
            return false;
        }

        if (String::IsNullOrEmpty(verifier)) {
            WriteHttpResponse(
                stream,
                400,
                "Bad Request",
                "Missing PKCE verifier.",
                "text/plain; charset=utf-8");
            return true;
        }

        PerformCodeExchange(receivedCode, verifier);
        WriteHttpResponse(
            stream,
            200,
            "OK",
            "<html><body><h3>Authentication complete. You may close this browser tab and return to the sample app.</h3></body></html>",
            "text/html; charset=utf-8");
        return true;
    }

    String^ HttpListenerThread::PerformCodeExchange(String^ code, String^ verifier)
    {
        if (String::IsNullOrEmpty(code) || String::IsNullOrEmpty(verifier) ||
            sharedData == nullptr || sharedData->sdConfig == nullptr) {
            return "";
        }

        String^ clientID = sharedData->sdConfig->client_id;
        String^ tokenEndpoint = sharedData->sdConfig->tokenEndpoint;
        if (String::IsNullOrEmpty(clientID) || String::IsNullOrEmpty(tokenEndpoint)) {
            return "";
        }

        // Load Google Desktop OAuth client_secret exclusively from environment variable
        // (never stored in tracked config files or compiled into RCDATA).
        String^ clientSecret = Environment::GetEnvironmentVariable("GOOGLE_OAUTH_CLIENT_SECRET");

        HttpClient^ client = gcnew HttpClient();
        try {
            Dictionary<String^, String^>^ formData = gcnew Dictionary<String^, String^>();
            formData->Add("code", code);
            formData->Add("redirect_uri", mPrefix);
            formData->Add("client_id", clientID);
            formData->Add("code_verifier", verifier);
            formData->Add("grant_type", "authorization_code");
            if (!String::IsNullOrEmpty(clientSecret) && clientSecret != "<your-client-secret>") {
                formData->Add("client_secret", clientSecret);
            }

            FormUrlEncodedContent^ content = gcnew FormUrlEncodedContent(formData);
            HttpResponseMessage^ response = client->PostAsync(tokenEndpoint, content)->Result;
            String^ retToken = response->Content->ReadAsStringAsync()->Result;

            Monitor::Enter(sharedData->syncRoot);
            try {
                sharedData->sdApiResponse = retToken;
            }
            finally {
                Monitor::Exit(sharedData->syncRoot);
            }
            return retToken;
        }
        finally {
            delete client;
        }
    }

};
