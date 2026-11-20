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

    // Dedicated loopback OAuth2 redirect listener bound directly to 127.0.0.1 on an
    // OS-assigned ephemeral port (zero probe-then-rebind TOCTOU). Accepts only
    // `GET /?state=...&code=...` matching the active CSRF state, exchanges the code
    // with RFC 7636 PKCE, and stops immediately upon completion.
    public ref class HttpListenerThread {
    public:
        TcpListener^ mTcpListener;
        Object^ mLock;
        bool stopRequested;
        SharedData^ sharedData;
        String^ mPrefix;
        int mPort;

    public:
        HttpListenerThread();
        void BindLoopback();
        void StartListening();
        void Stop();

    private:
        bool IsStopRequested();
        bool HandleClient(TcpClient^ client);
        void WriteHttpResponse(
            NetworkStream^ stream,
            int statusCode,
            String^ reasonPhrase,
            String^ content,
            String^ contentType);
        static bool FixedTimeEquals(String^ left, String^ right);
        String^ PerformCodeExchange(String^ code, String^ verifier);
    };

};
#endif // !DEVSERVER_H
