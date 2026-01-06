#include <Arduino.h>
#include "Log.h"
#include "AppContext.h" // Für Zeitstempel

Log::Log() : m_sendFunction(nullptr), m_currentLogLevel(LOG_LEVEL_INFO)
{
}

Log* Log::GetInstance()
{
    static Log instance;
    return &instance;
}

void Log::registerSendFunction(sendfuncptr function)
{
    if (function != nullptr)
        m_sendFunction = function;
}

void Log::setLogLevel(LogLevel level)
{
    m_currentLogLevel = level;
}

Log::LogLevel Log::getLogLevel() const
{
    return m_currentLogLevel;
}

std::string Log::levelToString(LogLevel level)
{
    switch (level)
    {
        case LOG_LEVEL_ERROR: return "FEHLER";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_DEBUG: return "DEBUG";
        default:              return "";
    }
}

void Log::print(LogLevel level, const std::string &text)
{
    if (m_sendFunction != nullptr && level <= m_currentLogLevel && level != LOG_LEVEL_NONE)
    {
        String timestamp = appContext.ntpTime.getFormattedTime();
        String levelStr = levelToString(level).c_str();
        
        String fullMessage;
        if (!levelStr.isEmpty()) {
            fullMessage = timestamp + " [" + levelStr + "] " + text.c_str();
        } else {
            fullMessage = text.c_str();
        }
        
        (*m_sendFunction)(fullMessage.c_str());
    }
}

void Log::println(LogLevel level, const std::string &text)
{
    print(level, text + "\r\n");
}

void Log::printf(LogLevel level, const char *format, ...)
{
    if (m_sendFunction != nullptr && level <= m_currentLogLevel && level != LOG_LEVEL_NONE)
    {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        println(level, buffer);
    }
}
