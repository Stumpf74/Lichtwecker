#ifndef INC_LOG_H
#define INC_LOG_H
#include <stdarg.h>
#include <string>

class Log
{
public:
   enum LogLevel
   {
      LOG_LEVEL_NONE,
      LOG_LEVEL_ERROR,
      LOG_LEVEL_INFO,
      LOG_LEVEL_DEBUG
   };

   typedef void (*sendfuncptr)(const std::string &msg);

   static Log *GetInstance();

   void registerSendFunction(sendfuncptr function);
   void setLogLevel(LogLevel level);
   LogLevel getLogLevel() const;

   void print(LogLevel level, const std::string &text);
   void println(LogLevel level, const std::string &text);
   void printf(LogLevel level, const char *format, ...);

private:
   Log();
   ~Log() = default;
   Log(const Log &) = delete;
   Log &operator=(const Log &) = delete;

   std::string levelToString(LogLevel level);

   sendfuncptr m_sendFunction;
   LogLevel m_currentLogLevel;
};

/**
 * @brief Wrapper-Funktion für INFO-Level Logging mit Zeilenumbruch.
 */
inline void LOG_INFO(const std::string &message)
{
   Log::GetInstance()->println(Log::LOG_LEVEL_INFO, message);
}

/**
 * @brief Wrapper-Funktion für DEBUG-Level Logging mit Zeilenumbruch.
 */
inline void LOG_DEBUG(const std::string &message)
{
   Log::GetInstance()->println(Log::LOG_LEVEL_DEBUG, message);
}

/**
 * @brief Wrapper-Funktion für ERROR-Level Logging mit Zeilenumbruch.
 */
inline void LOG_ERROR(const std::string &message)
{
   Log::GetInstance()->println(Log::LOG_LEVEL_ERROR, message);
}

/**
 * @brief Wrapper-Funktion für formatiertes Logging (printf-Stil).
 * Verwendet C++11 variadic templates für Typsicherheit.
 */
template <typename... Args>
inline void LOG_PRINTF(Log::LogLevel level, const char *format, Args... args)
{
   // Wir müssen die Größe des Buffers schätzen. 256 sollte für die meisten Log-Meldungen ausreichen.
   char buffer[256];
   int result = snprintf(buffer, sizeof(buffer), format, args...);
   if (result > 0 && result < sizeof(buffer)) {
      Log::GetInstance()->println(level, buffer);
   } else {
      Log::GetInstance()->println(Log::LOG_LEVEL_ERROR, "Fehler oder Pufferüberlauf in LOG_PRINTF");
   }
}

#endif // LOG
