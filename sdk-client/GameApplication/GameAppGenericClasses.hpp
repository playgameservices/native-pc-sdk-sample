#pragma once
#ifndef _HELPERCLASSES_H_
#define _HELPERCLASSES_H_
#include <Windows.h>
#include <string>
#include <vector>
#include <regex>
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <mutex>
#include "nlohmann\json.hpp"
#include "tinyxml2.h"
#include <nlohmann/json_fwd.hpp>

using json = nlohmann::json;
using namespace std;

namespace GameApp {
    class GameLogging
    {
    public:
        enum LogLevel {
            GLDEBUG,
            GLINFO,
            GLWARNING,
            GLERROR
        };

        // Static logging methods
        static void Debug(const std::string& message) {
            GetInstance().Log(GLDEBUG, message);
        }

        static void Info(const std::string& message) {
            GetInstance().Log(GLINFO, message);
        }

        static void Warning(const std::string& message) {
            GetInstance().Log(GLWARNING, message);
        }

        static void Error(const std::string& message) {
            GetInstance().Log(GLERROR, message);
        }

    private:
        GameLogging(const GameLogging&) = delete;
        GameLogging& operator=(const GameLogging&) = delete;

        // Members
        std::ofstream logFile;
        bool enableConsoleOutput;
        LogLevel minLogLevel;
        std::mutex logMutex;

        // Helper function to get the current timestamp
        static std::string GetTimestamp() {
            auto now = std::time(nullptr);
            std::tm localTime;
            localtime_s(&localTime, &now);

            std::ostringstream oss;
            oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
            return oss.str();
        }

        // Helper function to convert LogLevel to string
        static std::string LogLevelToString(LogLevel level) {
            switch (level) {
            case GLDEBUG:   return "DEBUG";
            case GLINFO:    return "INFO";
            case GLWARNING: return "WARNING";
            case GLERROR:   return "ERROR";
            default:      return "UNKNOWN";
            }
        }

        // Private constructor
        GameLogging(const std::string& filePath, LogLevel minLevel, bool consoleOutput)
            : enableConsoleOutput(consoleOutput), minLogLevel(minLevel) {
            if (!filePath.empty()) {
                logFile.open(filePath, std::ios::app); // Open the log file in append mode
                if (!logFile.is_open()) {
                    std::cerr << "Failed to open log file: " << filePath << std::endl;
                }
            }
        }

        // Singleton instance getter
        static GameLogging& GetInstance() {
            static GameLogging instance("GameLog.txt", GLINFO, true); // Singleton instance
            return instance;
        }

        // Log a message
        void Log(LogLevel level, const std::string& message) {
            if (level < minLogLevel) {
                return; // Ignore messages below the minimum log level
            }

            std::lock_guard<std::mutex> lock(logMutex); // Ensure thread safety
            std::string logMessage = "[" + GetTimestamp() + "] [" + LogLevelToString(level) + "] " + message;

            // Write to the log file
            if (logFile.is_open()) {
                logFile << logMessage << std::endl;
            }

            // Write to the console if enabled
            if (enableConsoleOutput) {
                std::cout << logMessage << std::endl;
            }
        }
    };


    class GameJsonParser {
    public:
        // Parse JSON from a string
        bool ParseFromString(const std::string& jsonString) {
            try {
                jsonData = json::parse(jsonString); // Parse the JSON string
                return true;
            }
            catch (const json::parse_error& e) {
                std::cerr << "JSON parsing error: " << e.what() << std::endl;
                return false;
            }
        }

        // Parse JSON from a file
        bool ParseFromFile(const std::string& filePath) {
            std::ifstream file(filePath);
            if (!file.is_open()) {
                GameLogging::Error("Failed to open file: " + filePath);
                return false;
            }

            try {
                file >> jsonData; // Parse the JSON file
                return true;
            }
            catch (const json::parse_error& e) {
                GameLogging::Error("JSON parsing error: " + string(e.what()));
                return false;
            }
        }

        // Deserialize JSON into a class object
        template <typename T>
        bool Deserialize(T& data) {
            try {
                data.FromJson(jsonData); // Call the class's FromJson method
                return true;
            }
            catch (...) {
                std::cerr << "Failed to deserialize JSON into class." << std::endl;
                return false;
            }
        }

        // Serialize a class object into JSON
        template <typename T>
        bool Serialize(const T& data) {
            try {
                jsonData = data.ToJson(); // Call the class's ToJson method
                return true;
            }
            catch (...) {
                std::cerr << "Failed to serialize class into JSON." << std::endl;
                return false;
            }
        }

        // Get a value from the JSON object by key
        template <typename T>
        T GetValue(const std::string& key, const T& defaultValue = T()) {
            if (jsonData.contains(key)) {
                return jsonData[key].get<T>(); // Return the value if the key exists
            }
            return defaultValue; // Return default value if the key does not exist
        }

        // Check if a key exists in the JSON object
        bool ContainsKey(const std::string& key) const {
            return jsonData.contains(key);
        }

        // Print the entire JSON object
        void PrintJson() const {
            GameLogging::Info("jsonData.dump: "+ jsonData.dump(4));
        }

        std::string Json() {
            return jsonData.dump(4);
        }

    private:
        json jsonData;
    };


    enum HTTPType {
        GET,
        POST
    };

    inline std::wstring stringToWstring(const std::string& utf8Str) {
        if (utf8Str.empty()) return L"";
        int wideStrSize = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
        if (wideStrSize == 0) {
            throw std::runtime_error("Failed to convert UTF-8 to wide string.");
        }
        std::wstring wideStr(wideStrSize - 1, L'\0'); // Subtract 1 to exclude null terminator
        MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideStr[0], wideStrSize);
        return wideStr;
    }

    inline std::string wstringTostring(const std::wstring& wideStr) {
        if (wideStr.empty()) return "";
        int utf8StrSize = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (utf8StrSize == 0) {
            throw std::runtime_error("Failed to convert wide string to UTF-8.");
        }
        std::string utf8Str(utf8StrSize - 1, '\0'); // Subtract 1 to exclude null terminator
        WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8Str[0], utf8StrSize, nullptr, nullptr);
        return utf8Str;
    }

    // Function to extract blocks enclosed in curly braces
    inline std::map<wstring,wstring> ExtractCurlyBracesBlocks(const std::wstring& data) {
        std::wregex pattern(L"\\{(.*?)\\}");
        map<wstring, wstring> param;
        //std::vector<std::wstring> blocks;
        auto begin = std::wsregex_iterator(data.begin(), data.end(), pattern);
        auto end = std::wsregex_iterator();
        for (std::wsregex_iterator it = begin; it != end; ++it) {
            std::wsmatch match = *it;
            //blocks.push_back(match[1].str());
            param[match[1].str()] = L"";
        }
        return param;
    }

 	struct AppConfig {
        std::string client_id;
        std::string client_secret;
        std::string redirect_uri;
        std::string authorization_Endpoint;
        std::string authorization_code;
        std::string Scopes;
        std::string tokenEndpoint;

        void populate() {
            
            GameJsonParser jsonParser;
            if (!jsonParser.ParseFromFile("AppConfig.json")) {
                GameLogging::Info("AppConfig.json file not found! exiting application");
                return;
            }
            if (!jsonParser.Deserialize(*this)) {
                GameLogging::Error("Failed to deserialize JSON into AppConfig.");
            }
        }

        // Deserialize JSON into the OAuthConfig object
        void FromJson(const nlohmann::json& jsonData) {
            client_id = jsonData.value("client_id", "");
            client_secret = jsonData.value("client_secret", "");
            redirect_uri = jsonData.value("redirect_uri", "");
            authorization_Endpoint = jsonData.value("authorization_Endpoint", "");
            authorization_code = jsonData.value("authorization_code", "");
            Scopes = jsonData.value("Scopes", "");
            tokenEndpoint = jsonData.value("tokenEndpoint", "");
        }
        // Serialize the OAuthConfig object into JSON
        nlohmann::json ToJson() const {
            nlohmann::json jsonData;
            jsonData["client_id"] = client_id;
            jsonData["client_secret"] = client_secret;
            jsonData["redirect_uri"] = redirect_uri;
            jsonData["authorization_Endpoint"] = authorization_Endpoint;
            jsonData["authorization_code"] = authorization_code;
            jsonData["Scopes"] = Scopes;
            jsonData["tokenEndpoint"] = tokenEndpoint;
            return jsonData;
        }
	};

    struct BillingApis {
        int id;
        string methodType;
        string methodName;
        string url;
    };

    inline list<BillingApis*>* PopulateBillingApiList(string fileName) {
        list<BillingApis*>* listApis = new list<BillingApis*>(0);
        std::ifstream file(fileName);
        if (!file.is_open()) {
            GameLogging::Error("Failed to open file: " + fileName);
            return nullptr;
        }
        try {
            std::ostringstream buffer;
            string jsonData;
            buffer << file.rdbuf(); 
            std::string jsonStr = buffer.str();
            file.close(); // Close the file
            nlohmann::json json_data;
            try {
                json_data = nlohmann::json::parse(jsonStr);
                for (const auto& item : json_data) {
                    BillingApis* api = new BillingApis();
                    api->id = item["ID"];
                    api->methodType = item["Method"];
                    api->methodName = item["MethodName"];
                    api->url = item["URL"];
                    listApis->push_back(api);
                }
            }
            catch (const nlohmann::json::parse_error& e) {
                GameLogging::Error("Error parsing JSON: " + string(e.what()));
            }
            return listApis;
        }
        catch (const json::parse_error& e) {
            GameLogging::Error("JSON parsing error: " + string(e.what()));
            return nullptr;
        }
        return nullptr;
    }

    struct Manifest {
        struct App {
            std::string PackageName;
            bool isDevelopment;
            std::string sku;
        } App;
    };

    inline bool ParseXmlFile(const std::string& filePath, Manifest& manifest) {
        bool ret = false;
        tinyxml2::XMLDocument doc;

        // Load the XML file
        if (doc.LoadFile(filePath.c_str()) != tinyxml2::XML_SUCCESS) {
            GameLogging::Error("Failed to load XML file: " + filePath);
            return ret;
        }

        // Get the root element
        tinyxml2::XMLElement* root = doc.FirstChildElement("Manifest");
        if (!root) {
            GameLogging::Error("Root element 'Manifest' not found.");
            return ret;
        }
        tinyxml2::XMLElement* applicationElement = root->FirstChildElement("Application");
        if (applicationElement) {
            tinyxml2::XMLElement* appElement = applicationElement->FirstChildElement("PackageName");
            if (appElement) {
                manifest.App.PackageName = appElement->GetText();
            }
            appElement = applicationElement->FirstChildElement("IsDeveloperMode");
            if (appElement) {
                manifest.App.isDevelopment = appElement->BoolText();
            }
            appElement = applicationElement->FirstChildElement("SKU");
            if (appElement) {
                manifest.App.sku = appElement->GetText();
            }
            ret = true;
        }
        return ret;
    }
};
#endif // !_HELPERCLASSES_H_

