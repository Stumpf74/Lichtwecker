#ifndef INC_LOG_H
#define INC_LOG_H

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

      static void ActivateLogging( bool act) {m_bfloggingactive = act;}
      static void RegisterSendfunction(sendfuncptr);
      static void Print( const String & text );
      static void Print( const char * text ) {Print(String(text));}



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
   if( function != nullptr )
      m_sendfunction = function;
}

/**
 * @brief 
 * 
 * @param text 
 */
void Log::Print( const String & text )
{
   if( m_sendfunction != nullptr )
      if( m_bfloggingactive)
         (*m_sendfunction)(text);
}





#endif // LOG
