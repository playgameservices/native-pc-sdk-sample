#pragma once
#ifndef GAME_APP_GENERIC_CLASSES_HPP_
#define GAME_APP_GENERIC_CLASSES_HPP_

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <bcrypt.h>

#include <cstdint>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include "tinyxml2.h"

#pragma comment(lib, "bcrypt.lib")

namespace GameApp {

class GameLogging {
 public:
  enum LogLevel { GLDEBUG, GLINFO, GLWARNING, GLERROR };

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

  std::ofstream logFile;
  bool enableConsoleOutput;
  LogLevel minLogLevel;
  std::mutex logMutex;

  static std::string GetTimestamp() {
    auto now = std::time(nullptr);
    std::tm localTime{};
    localtime_s(&localTime, &now);

    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return oss.str();
  }

  static std::string LogLevelToString(LogLevel level) {
    switch (level) {
      case GLDEBUG:
        return "DEBUG";
      case GLINFO:
        return "INFO";
      case GLWARNING:
        return "WARNING";
      case GLERROR:
        return "ERROR";
      default:
        return "UNKNOWN";
    }
  }

  GameLogging(const std::string& filePath, LogLevel minLevel,
              bool consoleOutput)
      : enableConsoleOutput(consoleOutput), minLogLevel(minLevel) {
    if (!filePath.empty()) {
      logFile.open(filePath, std::ios::app);
      if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: " << filePath << std::endl;
      }
    }
  }

  static GameLogging& GetInstance() {
    static GameLogging instance("GameLog.txt", GLINFO, true);
    return instance;
  }

  void Log(LogLevel level, const std::string& message) {
    if (level < minLogLevel) {
      return;
    }

    std::lock_guard<std::mutex> lock(logMutex);
    std::string logMessage =
        "[" + GetTimestamp() + "] [" + LogLevelToString(level) + "] " + message;

    if (logFile.is_open()) {
      logFile << logMessage << std::endl;
    }
    if (enableConsoleOutput) {
      std::cout << logMessage << std::endl;
    }
  }
};

inline std::wstring stringToWstring(const std::string& utf8Str) {
  if (utf8Str.empty()) return L"";
  int wideStrSize =
      MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
  if (wideStrSize <= 0) {
    return L"";
  }
  std::wstring wideStr(static_cast<size_t>(wideStrSize) - 1, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, wideStr.data(),
                      wideStrSize);
  return wideStr;
}

inline std::string wstringTostring(const std::wstring& wideStr) {
  if (wideStr.empty()) return "";
  int utf8StrSize = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1,
                                        nullptr, 0, nullptr, nullptr);
  if (utf8StrSize <= 0) {
    return "";
  }
  std::string utf8Str(static_cast<size_t>(utf8StrSize) - 1, '\0');
  WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, utf8Str.data(),
                      utf8StrSize, nullptr, nullptr);
  return utf8Str;
}

inline std::string BytesToHex(const unsigned char* data, size_t len) {
  static constexpr char kHexChars[] = "0123456789abcdef";
  std::string out;
  out.reserve(len * 2);
  for (size_t i = 0; i < len; ++i) {
    out.push_back(kHexChars[(data[i] >> 4) & 0x0F]);
    out.push_back(kHexChars[data[i] & 0x0F]);
  }
  return out;
}

// Computes a cryptographic SHA-256 hex digest using Windows CNG (BCrypt).
inline std::string ComputeSha256Hex(const std::string& payload) {
  BCRYPT_ALG_HANDLE hAlg = nullptr;
  NTSTATUS status = BCryptOpenAlgorithmProvider(
      &hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
  if (status < 0 || hAlg == nullptr) {
    throw std::runtime_error("BCryptOpenAlgorithmProvider(SHA256) failed");
  }

  unsigned char digest[32] = {};
  status = BCryptHash(
      hAlg, nullptr, 0,
      reinterpret_cast<PUCHAR>(const_cast<char*>(payload.data())),
      static_cast<ULONG>(payload.size()), digest, sizeof(digest));
  BCryptCloseAlgorithmProvider(hAlg, 0);

  if (status < 0) {
    throw std::runtime_error("BCryptHash(SHA256) failed");
  }
  return BytesToHex(digest, sizeof(digest));
}

// Generates a cryptographic Play Integrity request_hash binding the package,
// action, timestamp, and a CSPRNG nonce.
inline std::string GenerateIntegrityRequestHash(const std::string& packageName,
                                                const std::string& action) {
  unsigned char nonce[16] = {};
  NTSTATUS status = BCryptGenRandom(nullptr, nonce, sizeof(nonce),
                                    BCRYPT_USE_SYSTEM_PREFERRED_RNG);
  if (status < 0) {
    throw std::runtime_error("BCryptGenRandom failed");
  }
  std::ostringstream canonicalPayload;
  canonicalPayload << packageName << "|" << action << "|"
                   << std::time(nullptr) << "|"
                   << BytesToHex(nonce, sizeof(nonce));
  return ComputeSha256Hex(canonicalPayload.str());
}

struct Manifest {
  struct AppConfig {
    std::string PackageName;
    bool isDevelopment = true;
    std::string sku;
    std::vector<std::string> skuList;
    int64_t cloudProjectNumber = 0;
    std::string backendBaseUrl = "https://your-project.web.app";
    std::string apiAuthToken;
  } App;
};

inline std::string RedactTokenForLog(const std::string& token) {
  if (token.empty()) {
    return "(empty)";
  }
  return "<redacted:" + std::to_string(token.size()) + " chars>";
}

inline std::string GetEnvVarString(const char* name) {
  if (name == nullptr) {
    return "";
  }
  DWORD required = GetEnvironmentVariableA(name, nullptr, 0);
  if (required <= 1) {
    return "";
  }
  std::string value(required - 1, '\0');
  DWORD written = GetEnvironmentVariableA(name, value.data(), required);
  if (written == 0) {
    return "";
  }
  value.resize(written);
  return value;
}

inline std::string TrimAsciiWhitespace(const std::string& s) {
  const size_t start = s.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }
  const size_t end = s.find_last_not_of(" \t\r\n");
  return s.substr(start, end - start + 1);
}

inline bool FileExistsOnDisk(const std::string& path) {
  DWORD attr = GetFileAttributesA(path.c_str());
  return (attr != INVALID_FILE_ATTRIBUTES) &&
         ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

inline std::string GetExecutableDirectory() {
  char buf[MAX_PATH] = {};
  DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) {
    return "";
  }
  std::string fullPath(buf, len);
  const size_t slashPos = fullPath.find_last_of("\\/");
  return (slashPos != std::string::npos) ? fullPath.substr(0, slashPos) : "";
}

// Resolves manifest.local.xml (if present for local developer overrides) before
// falling back to <exe_dir>\manifest.xml and manifest.xml.
inline std::string ResolveManifestPath(
    const std::string& defaultName = "manifest.xml") {
  const std::string exeDir = GetExecutableDirectory();
  std::vector<std::string> candidates;
  candidates.push_back("manifest.local.xml");
  if (!exeDir.empty()) {
    candidates.push_back(exeDir + "\\manifest.local.xml");
    candidates.push_back(exeDir + "\\" + defaultName);
  }
  candidates.push_back(defaultName);
  for (const auto& candidate : candidates) {
    if (FileExistsOnDisk(candidate)) {
      return candidate;
    }
  }
  return defaultName;
}

inline std::string GetChildElementText(const tinyxml2::XMLElement* parent,
                                       const char* tagName,
                                       const std::string& fallback = "") {
  if (parent == nullptr) {
    return fallback;
  }
  const tinyxml2::XMLElement* elem = parent->FirstChildElement(tagName);
  if (elem == nullptr) {
    return fallback;
  }
  const char* text = elem->GetText();
  return (text != nullptr) ? std::string(text) : fallback;
}

inline bool ParseXmlFile(const std::string& filePath, Manifest& manifest) {
  const std::string resolvedPath = ResolveManifestPath(filePath);
  tinyxml2::XMLDocument doc;
  if (doc.LoadFile(resolvedPath.c_str()) != tinyxml2::XML_SUCCESS) {
    GameLogging::Error("Failed to load XML file: " + resolvedPath);
    return false;
  }

  const tinyxml2::XMLElement* root = doc.FirstChildElement("Manifest");
  if (root == nullptr) {
    GameLogging::Error("Root element 'Manifest' not found in " + resolvedPath);
    return false;
  }

  const tinyxml2::XMLElement* appElem = root->FirstChildElement("Application");
  if (appElem == nullptr) {
    GameLogging::Error("Child element 'Application' not found in " +
                       resolvedPath);
    return false;
  }

  manifest.App.PackageName =
      TrimAsciiWhitespace(GetChildElementText(appElem, "PackageName"));
  const std::string rawSku = GetChildElementText(appElem, "SKU");
  manifest.App.skuList.clear();
  std::stringstream skuStream(rawSku);
  std::string item;
  while (std::getline(skuStream, item, ',')) {
    std::string trimmed = TrimAsciiWhitespace(item);
    if (!trimmed.empty()) {
      manifest.App.skuList.push_back(trimmed);
    }
  }
  manifest.App.sku =
      manifest.App.skuList.empty() ? "" : manifest.App.skuList.front();

  if (const tinyxml2::XMLElement* devElem =
          appElem->FirstChildElement("IsDeveloperMode")) {
    manifest.App.isDevelopment = devElem->BoolText(true);
  }

  if (const tinyxml2::XMLElement* projElem =
          appElem->FirstChildElement("CloudProjectNumber")) {
    manifest.App.cloudProjectNumber = projElem->Int64Text(0);
  }

  std::string baseUrl = GetChildElementText(
      appElem, "BackendBaseUrl", "https://your-project.web.app");
  while (!baseUrl.empty() && baseUrl.back() == '/') {
    baseUrl.pop_back();
  }
  if (!baseUrl.empty()) {
    manifest.App.backendBaseUrl = baseUrl;
  }

  manifest.App.apiAuthToken =
      TrimAsciiWhitespace(GetChildElementText(appElem, "ApiAuthToken"));
  std::string envAuthToken = TrimAsciiWhitespace(GetEnvVarString("API_AUTH_TOKEN"));
  if (!envAuthToken.empty()) {
    manifest.App.apiAuthToken = envAuthToken;
  }

  return true;
}

}  // namespace GameApp

#endif  // GAME_APP_GENERIC_CLASSES_HPP_
