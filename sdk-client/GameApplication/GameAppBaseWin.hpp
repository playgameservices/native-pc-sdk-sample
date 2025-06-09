#pragma once
#ifndef _BASEWIN_H__
#define _BASEWIN_H__
#include <windows.h>
#include "shellapi.h"
#include <string>

#include <boost\asio.hpp>
#include <boost\beast.hpp>
#include <restclient-cpp\restclient.h>
#include <nlohmann/json.hpp>
#include "GameAppGenericClasses.hpp"

typedef map<HWND, HWND> MappedTxtBox;

namespace GameApp {
    // Define the function pointer type
    using FPEvents = std::function<void(void*)>;

    template <typename FP>
    struct EPair : public std::pair<FP, FP> {
        using std::pair<FP, FP>::pair;
        FP onSelect;
        FP onClick;
        EPair(FP e1, FP e2) : onSelect(e1), onClick(e2) {}
    };

    enum ResIDs {
        // Label
        IDC_LBL_GoogleTitle = 10001,
        IDC_LBL_AccessToken,
        IDC_LBL_SelectOpr,
        IDC_LBL_HttpMethod,
        IDC_LBL_DefultUrl,
        IDC_LBL_BuiltUrl,
        IDC_LBL_Result,
        IDC_LBL_Parameter,
        // Create textboxes
        IDC_TXT_AccessToken,
        IDC_TXT_DefaulUrl,
        IDC_TXT_BuiltUrl,
        IDC_TXT_Result,
        // Create buttons
        IDC_BTN_Execute,
        IDC_BTN_RebuildUrl,
        // Create checkbox
        IDC_CHECKBTN_BearerToken,
        // Create listbox
        IDC_LSTBOX_Operations,
        // Create panel
        IDC_SCROLLPANEL,
        // Create table layout (placeholder)
        IDC_tableParameters
    };
    using namespace GameApp;

    class BaseWindow {
    protected:
        std::wstring mBuildUrl;
        // Function to retrieve the IP address as std::string
        std::string GetIPAddress()
        {
            // Open a pipe to execute the "ipconfig" command
            FILE* pipe = _popen("ipconfig", "r");
            if (!pipe)
            {
                std::cerr << "Failed to execute ipconfig command." << std::endl;
                return "";
            }

            // Read the output of the command line-by-line
            char buffer[128];
            std::string result;
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
            {
                result += buffer;  // Append each line to the result string
            }

            // Close the pipe
            _pclose(pipe);

            // Parse the output to find the IPv4 address
            std::istringstream stream(result);
            std::string line;
            while (std::getline(stream, line))
            {
                // Look for the line containing "IPv4 Address"
                size_t pos = line.find("IPv4 Address");
                if (pos != std::string::npos)
                {
                    // Extract the IP address after the colon (:)
                    size_t colonPos = line.find(':');
                    if (colonPos != std::string::npos)
                    {
                        // Trim whitespace and return the IP address
                        std::string ip = line.substr(colonPos + 1);
                        ip.erase(0, ip.find_first_not_of(" \t"));  // Remove leading spaces
                        ip.erase(ip.find_last_not_of(" \t") + 1);  // Remove trailing spaces
                        return ip;
                    }
                }
            }

            // If no IPv4 address is found, return an empty string
            return "";
        }

    public:
        std::wstring mClassName;
        std::wstring mDialogTitle;
        Manifest manifest;
        wstring mPurchaseToken = L"";
        MappedTxtBox mMappedTxtBox;

        BaseWindow(HINSTANCE hInstance) : hwnd(NULL), hInstance(hInstance) {
            GameLogging::Info("Billing process started");
            mAppConfig.populate();
            ParseXmlFile("manifest.xml",manifest);

        }

        virtual ~BaseWindow() {
            if (hwnd) DestroyWindow(hwnd);
        }

        wstring FindReplace(wstring text, const wstring& search, const wstring& replace) {
            size_t pos = 0;
            while ((pos = text.find(search, pos)) != std::wstring::npos) {
                text.replace(pos, search.length(), replace);
                pos += replace.length();
            }
            return text;
        }

        wstring GetText(HWND hwd) {
            wstring buildUrl;
            buildUrl.reserve(GetWindowTextLength(txtDefaulUrl) + 1);
            GetWindowText(txtDefaulUrl, const_cast<WCHAR*>(buildUrl.c_str()), (int)buildUrl.capacity());
            //wcout << "data" << buildUrl.data() << endl;
            return buildUrl.data();
        }

        void UpdateUrl(map<wstring,wstring> params) 
        {
            //std::wstring buildUrl;
            mBuildUrl.reserve(GetWindowTextLength(txtDefaulUrl) + 1);
            GetWindowText(txtDefaulUrl, const_cast<WCHAR*>(mBuildUrl.c_str()), (int)mBuildUrl.capacity());
            SetWindowText(txtBuiltUrl, mBuildUrl.c_str());
            for (auto item : params) {
                string tempStr = wstringTostring(item.first.c_str());
                tempStr = "{" + tempStr + "}";
                wstring searchStr = stringToWstring(tempStr);
                mBuildUrl = FindReplace(mBuildUrl.data(), searchStr.data(), item.second);
            }
            SetWindowText(txtBuiltUrl, mBuildUrl.c_str());
        }

        // Helper function to create a TextBox-like control
        HWND CreateTextBox(HWND parent, int x, int y, int width, int height, const std::wstring& text,bool readOnly = false) {
            DWORD type = WS_CHILD | WS_VISIBLE | WS_BORDER;
            if (readOnly)
                type = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY;
            HWND textBox = CreateWindowEx(
                0, L"EDIT", text.c_str(),
                type,//WS_CHILD | WS_VISIBLE | WS_BORDER,//|  ES_READONLY,// | WS_HSCROLL,
                x, y, width, height,
                parent, NULL, NULL, NULL);
            SendMessage(textBox, LB_ADDSTRING, 0, (LPARAM)L"Billing()");
            return textBox;
        }

        void SetTableData(map<wstring, wstring> parameters) {
            static int numRows = 0;
            int rowCount = (int)parameters.size();
            int columnCount = 2;
            int fristColumnWidth = 180;  // Half of 480
            int secondColumnWidth = 400;
            int cellHeight = 20;

            numRows = rowCount;
            tableParameters = nullptr;
            scrollPanel = nullptr;
            scrollPanel = CreateWindow(L"STATIC", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL, 165, 265, 600, 120, hwnd, (HMENU)ResIDs::IDC_SCROLLPANEL, hInstance, NULL);
            tableParameters = CreateWindowEx(0, L"STATIC", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL,
                0, 0, 550, rowCount * cellHeight, scrollPanel, NULL, NULL, NULL);
            // Add TextBox-like controls for each parameter
            int rowIndex = 0;
            for (const auto& elements : parameters) {
                mListOfTxtboxs.push_back(CreateTextBox(tableParameters, 0, rowIndex * cellHeight, fristColumnWidth, cellHeight, elements.first));
                mListOfTxtboxs.push_back(CreateTextBox(tableParameters, fristColumnWidth + 3, rowIndex * cellHeight, secondColumnWidth, cellHeight, elements.second));
                ++rowIndex;
            }
        }

        void AddControls() {
            // Create labels
            lblGoogleTitle = CreateWindow(L"STATIC", L"Google Gaming Billing", WS_VISIBLE | WS_CHILD , 5, 5, 100, 50, hwnd, (HMENU)ResIDs::IDC_LBL_GoogleTitle, hInstance, NULL);
            lblAccessToken = CreateWindow(L"STATIC", L"Access Token:", WS_VISIBLE | WS_CHILD, 165, 5, 110, 18, hwnd, (HMENU)ResIDs::IDC_LBL_AccessToken, hInstance, NULL);
            lblSelectOpr = CreateWindow(L"STATIC", L"Select the operation:", WS_VISIBLE | WS_CHILD, 10, 75, 150, 18, hwnd, (HMENU)ResIDs::IDC_LBL_SelectOpr, hInstance, NULL);
            lblHttpMethod = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD, 350, 85, 200, 18, hwnd, (HMENU)ResIDs::IDC_LBL_HttpMethod, hInstance, NULL);//Http Method - 
            lblDefultUrl = CreateWindow(L"STATIC", L"Default Url:", WS_VISIBLE | WS_CHILD, 170, 85, 130, 18, hwnd, (HMENU)ResIDs::IDC_LBL_DefultUrl, hInstance, NULL);
            lblBuiltUrl = CreateWindow(L"STATIC", L"Built Url:", WS_VISIBLE | WS_CHILD, 170, 170, 75, 18, hwnd, (HMENU)ResIDs::IDC_LBL_BuiltUrl, hInstance, NULL);
            lblResult = CreateWindow(L"STATIC", L"Result:", WS_VISIBLE | WS_CHILD, 10, 390, 106, 18, hwnd, (HMENU)ResIDs::IDC_LBL_DefultUrl, hInstance, NULL);
            lblParameter = CreateWindow(L"STATIC", L"Parameter", WS_VISIBLE | WS_CHILD, 170, 250, 100, 18, hwnd, (HMENU)ResIDs::IDC_LBL_Parameter, hInstance, NULL);

            // Create textboxes
            txtAccessToken = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | WS_VSCROLL, 165, 25, 600, 50, hwnd, (HMENU)ResIDs::IDC_TXT_AccessToken, hInstance, NULL);
            txtDefaulUrl = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | WS_VSCROLL, 165, 105, 600, 60, hwnd, (HMENU)ResIDs::IDC_TXT_DefaulUrl, hInstance, NULL);
            txtBuiltUrl = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | WS_VSCROLL, 165, 185, 600, 60, hwnd, (HMENU)ResIDs::IDC_TXT_BuiltUrl, hInstance, NULL);
            txtResult = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | WS_VSCROLL, 10, 415, 760, 135, hwnd, (HMENU)ResIDs::IDC_TXT_Result, hInstance, NULL);

            // Create buttons
            btnExecute = CreateWindow(L"BUTTON", L"Execute", WS_VISIBLE | WS_CHILD, 650, 390, 100, 20, hwnd, (HMENU)ResIDs::IDC_BTN_Execute, hInstance, NULL);
            btnRebuildUrl = CreateWindow(L"BUTTON", L"Rebuild Url", WS_VISIBLE | WS_CHILD, 165, 390, 130, 20, hwnd, (HMENU)ResIDs::IDC_BTN_RebuildUrl, hInstance, NULL);

            // Create listbox
            lstBoxOperations = CreateWindowEx(
                WS_EX_LEFT, // Extended style for left alignment
                L"LISTBOX", // Class name
                L"",       // Window text (empty for ListBox)
                WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_BORDER | LBS_NOTIFY, // Styles
                10,        // X position
                95,        // Y position
                150,       // Width
                250,       // Height
                hwnd,      // Parent window handle
                (HMENU)ResIDs::IDC_LSTBOX_Operations, // Control ID
                hInstance, // Instance handle
                NULL       // Additional data (not used for ListBox)
            );
            // Create panel
            scrollPanel = CreateWindow(L"STATIC", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL, 165, 265, 600, 120, hwnd, (HMENU)ResIDs::IDC_SCROLLPANEL, hInstance, NULL);

            // Create table layout (placeholder)
            tableParameters = CreateWindow(L"STATIC", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 0, 0, 0, 15, scrollPanel, (HMENU)ResIDs::IDC_tableParameters, hInstance, NULL);
            HFONT hFont = CreateFont(
                16,                  // Height
                0,                   // Width
                0,                   // Escapement
                0,                   // Orientation
                FW_NORMAL,           // Weight
                FALSE,               // Italic
                FALSE,               // Underline
                FALSE,               // Strikeout
                DEFAULT_CHARSET,     // Character set
                OUT_DEFAULT_PRECIS,  // Output precision
                CLIP_DEFAULT_PRECIS, // Clipping precision
                DEFAULT_QUALITY,     // Quality
                DEFAULT_PITCH | FF_SWISS, // Pitch and family
                L"Arial"             // Font name
            );

            SendMessage(lstBoxOperations, WM_SETFONT, (WPARAM)hFont, TRUE);
        }
        

        // Show the window
        virtual void Show() {
            WNDCLASS wc = {};
            wc.lpfnWndProc = WndProc;
            wc.hInstance = hInstance;
            wc.lpszClassName = mClassName.c_str();
            RegisterClass(&wc);

            hwnd = CreateWindowEx(
                0, mClassName.c_str(), mDialogTitle.c_str(),
                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
                NULL, NULL, hInstance, this);
            AddControls();
            ShowWindow(hwnd, SW_SHOW);
        }

        void ClearTxtBoxs() {
            SetWindowText(txtBuiltUrl,L"");
            SetWindowText(txtDefaulUrl, L"");
            SetWindowText(txtResult, L"");
        }

    public:
        HWND hwnd; 
        HINSTANCE hInstance;
        GameJsonParser mJsonParser;
        AppConfig mAppConfig;
        list<HWND> mListOfTxtboxs;
        list<HWND>::iterator mlistTxtIterator;
        MappedTxtBox mMapOfTxtBoxs;

        // Controls
        HWND lblGoogleTitle = nullptr;
        HWND lblAccessToken = nullptr;
        HWND txtAccessToken = nullptr;
        HWND lblSelectOpr = nullptr;
        HWND lstBoxOperations = nullptr;
        HWND lblHttpMethod = nullptr;
        HWND lblDefultUrl = nullptr;
        HWND txtDefaulUrl = nullptr;
        HWND txtBuiltUrl = nullptr;
        HWND lblBuiltUrl = nullptr;
        HWND lblResult = nullptr;
        HWND btnExecute = nullptr;
        HWND txtResult = nullptr;
        HWND lblParameter = nullptr;
        HWND btnRebuildUrl = nullptr;
        HWND checkBtnBearerToken = nullptr;
        HWND scrollPanel = nullptr;
        HWND tableParameters = nullptr;

        // Helper methods
        virtual void CreateControls() = 0; 
        virtual void OnButtonClick(int buttonID) = 0; 
        virtual void OnListBoxSelectionChange() = 0; 
        virtual void OnRebuildUrlButtonClick() = 0; 
        virtual void OnLoadWindow() = 0;

        bool ExtractParameters(const std::wstring& data) {
            bool ret = false;
            std::wregex pattern(L"\\{(.*?)\\}");
            MappedTxtBox param;
            auto begin = std::wsregex_iterator(data.begin(), data.end(), pattern);
            auto end = std::wsregex_iterator();
            for (std::wsregex_iterator it = begin; it != end; ++it) {
                std::wsmatch match = *it;
            }
            return ret;;
        }

    private:
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
            BaseWindow* pThis = nullptr;
            if (msg == WM_CREATE) {
                CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
                pThis = reinterpret_cast<BaseWindow*>(pCreate->lpCreateParams);
                SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
                pThis->CreateControls();
                pThis->OnLoadWindow(); // Call OnLoadWindow after creating controls
            }
            else {
                pThis = reinterpret_cast<BaseWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            }

            if (pThis) {
                switch (msg) {
                case WM_COMMAND:
                    if (LOWORD(wParam) == IDC_BTN_Execute) { 
                        pThis->OnButtonClick(IDC_BTN_Execute);
                    }
                    else if (LOWORD(wParam) == IDC_BTN_RebuildUrl) { 
                        pThis->OnRebuildUrlButtonClick();
                    }
                    else if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == IDC_LSTBOX_Operations) {
                        pThis->OnListBoxSelectionChange();
                    }
                    break;
                case WM_CTLCOLORSTATIC: {
                        HDC hdc = (HDC)wParam;
                        SetBkMode(hdc, TRANSPARENT); // Make the background transparent
                        return (LRESULT)GetStockObject(NULL_BRUSH); // No background brush
                    }
                    break;

                case WM_CLOSE:
                    if (MessageBox(hwnd, L"Are you sure you want to close this window?", L"Confirm Close", MB_YESNO) == IDYES) {
                        DestroyWindow(hwnd); // Close the window
                    }
                    break;

                case WM_DESTROY:
                    PostQuitMessage(0); // Exit the message loop
                    break;
                default:
                    return DefWindowProc(hwnd, msg, wParam, lParam);
                }
            }
            else {
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            return 0;
        }

        public:
        //OAuth members
        std::string mBrowserCmd = "", mUserToken = "", mUserAccessToken = "";

        //OAuth functions
        // Step 1: Generate the OAuth2 authorization URL
        std::string GenerateAuthorizationUrl(AppConfig config) {
            std::string authUrl = config.authorization_Endpoint +
                "?response_type=code&client_id=" + config.client_id + "&redirect_uri="
                + config.redirect_uri + "&scope=" + config.Scopes + "+&code_challenge=" + "" + "&code_challenge_method=S256";
            GameLogging::Info("Go to the following URL to authenticate: " + authUrl);
            return authUrl;
        }
        
        void onClickGLogin(void* ptr){
            mBrowserCmd = GenerateAuthorizationUrl(mAppConfig);
            std::thread listenerThread(&BaseWindow::StartListener, this);
            listenerThread.detach();
            ShellExecuteA(NULL, "open", mBrowserCmd.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }

        // Callback handler after successful authentication
        void HandleCallback(const std::string& code) {
            std::string accessToken = ExchangeCodeForAccessToken(code);
            if (accessToken == "") {
                return;
            }
            if (!accessToken.empty()) {
                mUserToken = accessToken.c_str();
                json jAccessToken = json::parse(mUserToken);
                mUserAccessToken = jAccessToken["access_token"];
                GameLogging::Info("User Token: " + mUserToken);
                GameLogging::Info("Access Token: " + mUserAccessToken);
                SetWindowText(txtResult, stringToWstring(mUserToken).c_str());
                SetWindowText(txtAccessToken, stringToWstring(mUserAccessToken).c_str());
                SetForegroundWindow(hwnd);
                //GetUserDetails(mUserAccessToken);
            }
        }

        std::map<std::string, std::string> parseQueryParams(const std::string& url) {
            std::map<std::string, std::string> params;
            size_t questionMarkPos = url.find("?");
            if (questionMarkPos == std::string::npos) {
                return params; // No query parameters found
            }
            std::string query = url.substr(questionMarkPos + 1); // Extract query string
            size_t pos = 0;
            while (pos < query.size()) {
                size_t ampPos = query.find("&", pos);
                std::string param = query.substr(pos, ampPos - pos);
                size_t equalPos = param.find("=");
                if (equalPos != std::string::npos) {
                    std::string key = param.substr(0, equalPos);
                    std::string value = param.substr(equalPos + 1);
                    params[key] = value;
                }
                if (ampPos == std::string::npos) break;
                pos = ampPos + 1;
            }
            return params;
        }

        // Step 2: Start a listener to wait for the callback with the authorization code
        void StartListener() {
            try {
                boost::asio::io_context io_context;

                // Listener endpoint
                boost::asio::ip::tcp::acceptor acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 8080));
                boost::asio::ip::tcp::socket socket(io_context);
                acceptor.accept(socket);
                GameLogging::Info("Client connected, waiting for callback...");
                // Receive HTTP request
                boost::beast::flat_buffer buffer;
                boost::beast::http::request<boost::beast::http::string_body> req;
                boost::beast::http::read(socket, buffer, req);
                GameLogging::Info("HTTP Request received: ");
                string reqString = req.target();
                GameLogging::Info("reqString: " + reqString);
                std::map<std::string, std::string> params = parseQueryParams(reqString);
                string code = params["code"].c_str();
                GameLogging::Info("Received code: " + code);
                // Send HTTP response
                boost::beast::http::response<boost::beast::http::string_body> res{
                    boost::beast::http::status::ok, req.version() };
                res.set(boost::beast::http::field::content_type, "text/plain");
                res.body() = "Authentication successful! You can now close this window.";
                res.prepare_payload();
                boost::beast::http::write(socket, res);

                // Handle the callback by exchanging the code for the token
                HandleCallback(code);

            }
            catch (const std::exception& e) {
                GameLogging::Error("Error in StartListener: " + string(e.what()));
            }
        }

        std::string ExchangeCodeForAccessToken(const std::string& code) {
            string ret = "Error: Unknown error";
            AppConfig config;
            config.populate();
            try {
                std::string payload =
                    "code=" + code
                    + "&client_id=" + config.client_id +
                    "&client_secret=" + config.client_secret +
                    "&redirect_uri=" + config.redirect_uri +
                    "&grant_type=authorization_code";

                GameLogging::Info("Json data:\n" + payload);
                std::string url = "https://oauth2.googleapis.com/token";
                std::string content_type = "application/x-www-form-urlencoded";
                RestClient::Response response = RestClient::post(url, content_type, payload);
                std::stringstream mess;
                mess << "url" << url << endl;
                mess << "content_type" << content_type << endl;
                mess << "RestClient::post(url, content_type, payload);" << endl;
                if (response.code == 200) {
                    mess << "POST request successful" << std::endl;
                    mess << "Response body: " << response.body << std::endl;
                    GameLogging::Info(mess.str());
                    return response.body.c_str();
                }
                else {
                    mess << "POST request failed" << std::endl;
                    mess << "Status code: " << response.code << std::endl;
                    mess << "Error: " << response.body << std::endl;
                    GameLogging::Error(mess.str());
                }
            }
            catch (exception e) {
                stringstream exc;
                exc << "exception on POST call get token: " << e.what();
                GameLogging::Error(exc.str());
            }
            return "Error";
        }
    };
};

#endif // !_BASEWIN_H__


