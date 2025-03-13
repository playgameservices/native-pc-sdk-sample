#pragma once
#ifndef DEVSERVER_H
#define DEVSERVER_H

#include "HelperClasses.h"
#include <string>

using namespace System;
using namespace System::IO;
using namespace System::Security::Cryptography;
using namespace System::Net;
using namespace System::Threading;
using namespace System::Net::Sockets;
namespace GameApplication {

    using namespace System;
    using namespace System::Net;

    ref class HttpListenerThread {
    public:
        HttpListener^ mListenerOnThread;
		HttpListenerContext^ mContext;
        bool stopRequested;
		SharedData^ sharedData;
    public:
		HttpListenerThread();
		String^ mPrefix;
		void StartListening();
        void Stop();
        void HandleRequest(HttpListenerContext^ context);
		int GetRandomUnusedPort();
		void WriteResponse(HttpListenerResponse^ response, String^ content, int statusCode);
		String^ PerformCodeExchange(String^ code);
		String^ HttpGetCall(String^ url, bool bearer, bool serviceToken);
		String^ GetPurchasesDetails(String^ url, bool bearer);
		String^ GetSKUsDetails(String^ url, bool bearer);
		static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output);

		// Function to perform HTTP GET using libcurl
		void PerformCurlPurchasesRequest(const std::string& url, const std::string& bearerToken);
    };


	public ref class DevServer
	{
	public:
		DevServer(SharedData^ sd);
		~DevServer();
		HttpListener^ mlistener;
		Thread^ devThread;
		HttpListenerContext^ mContext;
		HttpListenerRequest^ mRequest;
		HttpListenerResponse^ mResponse;
		String^ prefix;
		bool stopRequested;
		Object^ lockObject;
		SharedData^ sharedData;

		//Thread functions
		void DevThreadStart();
		void DevThreadStop();
		void RunServer();
		void UpdateSharedData(SharedData^ data);


		//Listener functions
		void WriteResponse(HttpListenerResponse^ response, String^ content, int statusCode);
		int GetRandomUnusedPort();
		void Start();
		void Stop();
		void HandleRequest(HttpListenerContext^ context);
	};
};
#endif // !DEVSERVER_H



