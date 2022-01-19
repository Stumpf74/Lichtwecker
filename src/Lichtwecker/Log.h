#ifndef INC_LOG_H
#define INC_LOG_H
#include <stdarg.h>
#include "Config.h"
#include "Debug.h"

/**
 * @brief Logging Klasse 
 * 
 */
class Log
{
public:
   /**
       * @brief Prototype für die Sendefunktion
       * 
       */
   typedef void (*sendfuncptr)(const String &msg);

   
   static void ActivateLogging(bool act) { m_bfloggingactive = act; }
   static void RegisterSendfunction(sendfuncptr);
   static void Print(); // Only new line
   static void Print(const String &text);
   static void Print(const char *text) { Print(String(text)); }
   static void Print(const uint32_t value) { Print(String(value)); }
   static void Print(const int32_t value) { Print(String(value)); }
   static void Print(const float value);
   static void Print(const double value);
   static void PrintF(const char *format, ...);

private:
   Log(/* args */);
   ~Log();

   static sendfuncptr m_sendfunction;
   static bool m_bfloggingactive;
};

Log::sendfuncptr Log::m_sendfunction = nullptr;
bool Log::m_bfloggingactive = false;

/**
 * @brief Construct a new Log:: Log object
 * 
 */
Log::Log(/* args */)
{
}

/**
 * @brief Destroy the Log:: Log object
 * 
 */
Log::~Log()
{
}

/**
 * @brief 
 * 
 * @param function 
 */
void Log::RegisterSendfunction(sendfuncptr function)
{
   if (function != nullptr)
      m_sendfunction = function;
}

/**
 * @brief 
 * 
 * @param text 
 */
void Log::Print(const String &text)
{
   if (m_sendfunction != nullptr)
      if (m_bfloggingactive)
         (*m_sendfunction)(text);
}

/**
 * @brief 
 * 
 * @param text 
 */
void Log::Print()
{
   if (m_sendfunction != nullptr)
      if (m_bfloggingactive)
         (*m_sendfunction)("\n\r");
}

/**
 * @brief 
 * 
 * @param text 
 */
void Log::Print(const float value)
{
   Print(String(value));
}

/**
 * @brief 
 * 
 * @param text 
 */
void Log::Print(const double value)
{
   Print(String(value));
}


/**
 * @brief 
 * 
 * @param text 
 */
void Log::PrintF(const char *format, ... )
{
   va_list args;
   va_start(args, format);

   // int snprintf ( char * s, size_t n, const char * format, ... );
   char cBuffer[2048];
   int32_t byteswrite = vsnprintf ((char *)&cBuffer, 2048, format, args);
   if (byteswrite >= 0)
   {
      cBuffer[byteswrite] = 0;
      Print(cBuffer);
      //Serial.print(cBuffer);
   }

   va_end(args);
}

#endif // LOG
