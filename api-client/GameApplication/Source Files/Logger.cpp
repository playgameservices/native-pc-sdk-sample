
#include "pch.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>

#include "Logger.h"

using namespace std;
using namespace CLogging;


Logger* Logger::m_Instance = 0;

// Log file name. File name should be change from here only
const string logFileName = "GameApplication.log";

std::string Logger::ConvertString(System::String^ managedString) {
    std::string nativeString;
    for (int i = 0; i < managedString->Length; i++) {
        nativeString += static_cast<char>(managedString[i]);
    }
    return nativeString;
}

Logger::Logger()
{
   m_File.open(logFileName.c_str(), ios::out|ios::app);
   m_LogLevel	= LOG_LEVEL_TRACE;
   m_LogType	= FILE_LOG;

   // Initialize mutex
#ifdef WIN32
   InitializeCriticalSection(&m_Mutex);
#else
   int ret=0;
   ret = pthread_mutexattr_settype(&m_Attr, PTHREAD_MUTEX_ERRORCHECK_NP);
   if(ret != 0)
   {   
      printf("Logger::Logger() -- Mutex attribute not initialize!!\n");
      exit(0);
   }   
   ret = pthread_mutex_init(&m_Mutex,&m_Attr);
   if(ret != 0)
   {   
      printf("Logger::Logger() -- Mutex not initialize!!\n");
      exit(0);
   }   
#endif
}

Logger::~Logger()
{
   m_File.close();
#ifdef WIN32
   DeleteCriticalSection(&m_Mutex);
#else
   pthread_mutexattr_destroy(&m_Attr);
   pthread_mutex_destroy(&m_Mutex);
#endif
}

Logger* Logger::getInstance() throw ()
{
   if (m_Instance == 0) 
   {
      m_Instance = new Logger();
   }
   return m_Instance;
}

void Logger::lock()
{
#ifdef WIN32
   EnterCriticalSection(&m_Mutex);
#else
   pthread_mutex_lock(&m_Mutex);
#endif
}

void Logger::unlock()
{
#ifdef WIN32
   LeaveCriticalSection(&m_Mutex);
#else
   pthread_mutex_unlock(&m_Mutex);
#endif
}

void Logger::logIntoFile(std::string& data)
{
   lock();
   m_File << getCurrentTime() << "  " << data << endl;
   unlock();
}

void Logger::logOnConsole(std::string& data)
{
   cout << getCurrentTime() << "  " << data << endl;
}

string Logger::getCurrentTime()
{
   return ConvertString(System::DateTime::Now.ToString());
}

// Interface for Error Log
void Logger::error(const char* text) throw()
{
   string data;
   data.append("[ERROR]: ");
   data.append(text);

   // ERROR must be capture
   if(m_LogType == FILE_LOG)
   {
      logIntoFile(data);
      cout << data;
   }
   else if(m_LogType == CONSOLE)
   {
      logOnConsole(data);
   }
}

void Logger::error(std::string& text) throw()
{
   error(text.data());
}

void Logger::error(std::ostringstream& stream) throw()
{
   string text = stream.str();
   error(text.data());
}

// Interface for Alarm Log 
void Logger::alarm(const char* text) throw()
{
   string data;
   data.append("[ALARM]: ");
   data.append(text);

   // ALARM must be capture
   if(m_LogType == FILE_LOG)
   {
      logIntoFile(data);
   }
   else if(m_LogType == CONSOLE)
   {
      logOnConsole(data);
   }
}

void Logger::alarm(std::string& text) throw()
{
   alarm(text.data());
}

void Logger::alarm(std::ostringstream& stream) throw()
{
   string text = stream.str();
   alarm(text.data());
}

// Interface for Always Log 
void Logger::always(const char* text) throw()
{
   string data;
   data.append("[ALWAYS]: ");
   data.append(text);

   // No check for ALWAYS logs
   if(m_LogType == FILE_LOG)
   {
      logIntoFile(data);
   }
   else if(m_LogType == CONSOLE)
   {
      logOnConsole(data);
   }
}

void Logger::always(std::string& text) throw()
{
   always(text.data());
}

void Logger::always(std::ostringstream& stream) throw()
{
   string text = stream.str();
   always(text.data());
}

// Interface for Buffer Log 
void Logger::buffer(const char* text) throw()
{
   // Buffer is the special case. So don't add log level
   // and timestamp in the buffer message. Just log the raw bytes.
   if((m_LogType == FILE_LOG) && (m_LogLevel >= LOG_LEVEL_BUFFER))
   {
      lock();
      m_File << text << endl;
      unlock();
   }
   else if((m_LogType == CONSOLE) && (m_LogLevel >= LOG_LEVEL_BUFFER))
   {
      cout << text << endl;
   }
}

void Logger::buffer(std::string& text) throw()
{
   buffer(text.data());
}

void Logger::buffer(std::ostringstream& stream) throw()
{
   string text = stream.str();
   buffer(text.data());
}

void Logger::info(String^ str) throw()
{
    string data = ConvertString(str);
    info(data.c_str());
}

// Interface for Info Log
void Logger::info(const char* text) throw()
{
   string data;
   data.append("[INFO]: ");
   data.append(text);

   if((m_LogType == FILE_LOG) && (m_LogLevel >= LOG_LEVEL_INFO))
   {
      logIntoFile(data);
      cout << data;
   }
   else if((m_LogType == CONSOLE) && (m_LogLevel >= LOG_LEVEL_INFO))
   {
      logOnConsole(data);
   }
}

void Logger::info(std::string& text) throw()
{
   info(text.data());
}

void Logger::info(std::ostringstream& stream) throw()
{
   string text = stream.str();
   info(text.data());
}

// Interface for Trace Log
void Logger::trace(const char* text) throw()
{
   string data;
   data.append("[TRACE]: ");
   data.append(text);

   if((m_LogType == FILE_LOG) && (m_LogLevel >= LOG_LEVEL_TRACE))
   {
      logIntoFile(data);
   }
   else if((m_LogType == CONSOLE) && (m_LogLevel >= LOG_LEVEL_TRACE))
   {
      logOnConsole(data);
   }
}

void Logger::trace(std::string& text) throw()
{
   trace(text.data());
}

void Logger::trace(std::ostringstream& stream) throw()
{
   string text = stream.str();
   trace(text.data());
}

// Interface for Debug Log
void Logger::debug(const char* text) throw()
{
   string data;
   data.append("[DEBUG]: ");
   data.append(text);

   if((m_LogType == FILE_LOG) && (m_LogLevel >= LOG_LEVEL_DEBUG))
   {
      logIntoFile(data);
   }
   else if((m_LogType == CONSOLE) && (m_LogLevel >= LOG_LEVEL_DEBUG))
   {
      logOnConsole(data);
   }
}

void Logger::debug(std::string& text) throw()
{
   debug(text.data());
}

void Logger::debug(std::ostringstream& stream) throw()
{
   string text = stream.str();
   debug(text.data());
}

// Interfaces to control log levels
void Logger::updateLogLevel(LogLevel logLevel)
{
   m_LogLevel = logLevel;
}

// Enable all log levels
void Logger::enaleLog()
{
   m_LogLevel = ENABLE_LOG; 
}

// Disable all log levels, except error and alarm
void Logger:: disableLog()
{
   m_LogLevel = DISABLE_LOG;
}

// Interfaces to control log Types
void Logger::updateLogType(LogType logType)
{
   m_LogType = logType;
}

void Logger::enableConsoleLogging()
{
   m_LogType = CONSOLE; 
}

void Logger::enableFileLogging()
{
   m_LogType = FILE_LOG ;
}

