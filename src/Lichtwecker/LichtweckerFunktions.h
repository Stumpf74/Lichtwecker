#ifndef INC_LICHTWECKER_H
#define INC_LICHTWECKER_H

#include "FastLED.h"
#include "debug.h"
#include <time.h>
#include "AlarmTime.h"
#include <ArduinoJson.h>

//#define UPDATES_PER_SECOND 100

// Example json alarm timer
// {
//   "version" : 1,
//   "timer" : [
//     {
//       "number" : 1,
//       "weekday" : [1,2,3],
//       "time" : "8:11",
//       "active" : true
//     },
//     {
//       "number" : 2,
//       "weekday" : [1,2,3],
//       "time" : "8:11",
//       "active" : false
//     },
//     {
//       "number" : 3,
//       "weekday" : [1,2,3],
//       "time" : "8:11",
//       "active" : false
//     },
//     {
//       "number" : 4,
//       "weekday" : [1,2,3],
//       "time" : "8:11",
//       "active" : false
//     },    
//     {
//       "number" : 5,
//       "weekday" : [1,2,3],
//       "time" : "8:11",
//       "active" : false
//     },   
//     {
//       "number" : 6,
//       "weekday" : [1,2,3],
//       "time" : "8:11",
//       "active" : false
//     },    
//     {
//       "number" : 7,
//       "weekday" : [1,2,3],
//       "time" : "8:11",
//       "active" : false
//     }    
//  ]
//  }






DEFINE_GRADIENT_PALETTE(sunrise_gp){
//    Line number      
//    |     red      
//    |      |      green      
//    |      |       |      
//    |      |       |      blue       
//    |      |       |       |       
      0,      5,      3,     0,  //off -> black
      51,   176/4*1,    150/4*1,      0,  //dunkles rot-gelb
      102,  176/4*2,    176/4*2,     60,  //dunkles gelb
      153,  240/4*3,    240/4*3,    120, // gelb-weiß
      255,  240/4*3,    240/4*3,    255  //sehr helles Blau

      // 0,      0,      0,     0,  //off -> black
      // 128,  220,    110,     0,  //rot - gelb
      // 224,  240,    240,     0,  //gelb
      // 255,  128,    128,    240 //sehr helles Blau

};


CRGB crgb_RedYellow(0xB0,   0x8D,      0);  //rot - gelb
CRGB crgb_Yellow(    240,    240,      0);  //gelb
CRGB crgb_LigthBlue( 128,    128,    240); //sehr helles Blau



/**
 * @brief class cLigthAlarmClock 
 * 
 */
class cLigthAlarmClock
{
   public:
   

   static cLigthAlarmClock *GetInstance();
   ~cLigthAlarmClock();

   void Setup();
   void Stop();
   void Runtime(time_t actTime);
   void SetWackupTime( uint8_t ucAlarmNumber, const cAlarmTime & alarmtime);
   void SetRgb(String strMsg);
   void ParseAlarms( String strMsg );
   void StartLigthSequenz();


private:
   cLigthAlarmClock();
   void Sunrise();
   void LedsOff();
   void SetLigthStartTime();
   void CalculateDelayTime();
   void CheckTimeIsExpired(time_t Time);
   void CheckTimeStamp(time_t actTime);
   void StartAlarmSequenz();
   int dayofweek(int day,int month,int year);


private:

   CRGBPalette256 m_sunrisePal = sunrise_gp;

   static cLigthAlarmClock *m_ptrInstance;

   static const uint8_t m_ucLED_PIN = 2; // --> D2
   static const uint8_t m_ucNUM_LEDS = 120;
   //#define BRIGHTNESS 50
   CRGB m_leds[m_ucNUM_LEDS];

   uint32_t m_uiLigthStepDelay;
   static const char m_daysOfTheWeek[7][3];

   static const uint8_t m_ucMaxAlarmTimer = 10;
   cAlarmTime m_tAlarmTime[m_ucMaxAlarmTimer];
   cAlarmLigthTime m_tAlarmLigthTime[m_ucMaxAlarmTimer];
   bool m_IsSunriseStarted;
   uint32_t m_uiSunriseIndex;
};

const char cLigthAlarmClock::m_daysOfTheWeek[7][3] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};




void cLigthAlarmClock::ParseAlarms( String strMsg )
{
   // Size of document with 7 timers
   // 7*JSON_ARRAY_SIZE(3) + JSON_ARRAY_SIZE(7) + JSON_OBJECT_SIZE(2) + 7*JSON_OBJECT_SIZE(4)
   // Additional bytes for strings duplication
   // 76

   // Platform	Size
   // AVR
   // 464+76 = 540
   // ESP32
   // ESP8266
   // 928+76 = 1004     
   DPRINTLN( strMsg );
   StaticJsonDocument<2000> doc;      
   deserializeJson(doc, strMsg.c_str());
   
   JsonArray timer = doc["timer"];
   JsonObject timer_0 = timer[0];
   int timer_0_number = timer_0["number"]; // 1
   JsonArray timer_0_weekday = timer_0["weekday"];
   int timer_0_weekday_0 = timer_0_weekday[0]; // 1
   int timer_0_weekday_1 = timer_0_weekday[1]; // 2
   int timer_0_weekday_2 = timer_0_weekday[2]; // 3
   const char* timer_0_time = timer_0["time"]; // "8:11"
   bool timer_0_active = timer_0["active"]; // true

}

/**
 * @brief 
 * 
 * @param ucAlarmNumber 
 * @param alarmtime 
 */
void cLigthAlarmClock::SetWackupTime( uint8_t ucAlarmNumber, const cAlarmTime & alarmtime)
{
   assert(ucAlarmNumber >= m_ucMaxAlarmTimer);



}

/**
 * @brief 
 * 
 */
void cLigthAlarmClock::SetLigthStartTime()
{

}

/**
 * @brief 
 * 
 */
void cLigthAlarmClock::CalculateDelayTime()
{

}

/**
 * @brief Singleton of the AlarmClock 
 * 
 */
cLigthAlarmClock *cLigthAlarmClock::m_ptrInstance = nullptr;

/**
 * @brief Construct a new c Ligth Alarm Clock::c Ligth Alarm Clock object
 * 
 */
cLigthAlarmClock::cLigthAlarmClock() : m_uiLigthStepDelay(0), m_IsSunriseStarted(false), m_uiSunriseIndex(0)
{
}

/**
 * @brief Destroy the c Ligth Alarm Clock::c Ligth Alarm Clock object
 * 
 */
cLigthAlarmClock::~cLigthAlarmClock()
{
}

/**
 * @brief Get Instance
 * 
 * @return cLigthAlarmClock* 
 */
cLigthAlarmClock *cLigthAlarmClock::GetInstance()
{
   if (m_ptrInstance == nullptr)
      m_ptrInstance = new cLigthAlarmClock();

   return m_ptrInstance;
}

/**
 * @brief init the class
 * 
 */
void cLigthAlarmClock::Setup()
{
   FastLED.addLeds<WS2812B, m_ucLED_PIN, GRB>(m_leds, m_ucNUM_LEDS).setCorrection(TypicalLEDStrip);
   LedsOff();

   
   m_tAlarmLigthTime[0];   

}

/**
 * @brief stops the alarm clock
 * 
 */
void cLigthAlarmClock::Stop()
{
   LedsOff();
   m_IsSunriseStarted = false;
}

void cLigthAlarmClock::StartLigthSequenz()
{
   m_IsSunriseStarted = true;
}

void cLigthAlarmClock::StartAlarmSequenz()
{

}


/**
 * @brief 
 * 
 * @param actTime 
 */
void cLigthAlarmClock::CheckTimeStamp(time_t actTime)
{
   tm *ts_time = localtime(&actTime);
      // DPRINT(m_daysOfTheWeek[ts_time->tm_wday]);DPRINT(", ");DPRINT(ts_time->tm_mday);DPRINT(".\t\t");
      // DPRINT(ts_time->tm_hour);DPRINT(":");DPRINT(ts_time->tm_min);DPRINT(":");DPRINTLN(ts_time->tm_sec);
   
   for( int i = 0; i < (sizeof(m_tAlarmLigthTime)/ sizeof(cAlarmLigthTime)); i++)
   {
      if( m_tAlarmLigthTime[i].IsActive())
      {
         DPRINT(m_daysOfTheWeek[ts_time->tm_wday]);DPRINT(", ");DPRINT(ts_time->tm_mday);DPRINT(".\t\t");
         DPRINT(ts_time->tm_hour);DPRINT(":");DPRINT(ts_time->tm_min);DPRINT(":");DPRINTLN(ts_time->tm_sec);
         DPRINT("Check Alarm Ligth Time: ");DPRINTLN(i);

            // this is the rigth day
         for ( auto weekday = m_tAlarmLigthTime[i].GetWeekDays()->begin(); weekday != m_tAlarmLigthTime[i].GetWeekDays()->end(); ++weekday )
         {
            if( ts_time->tm_wday == *weekday)
               // this is the rigth hour
               if( ts_time->tm_hour == m_tAlarmLigthTime[i].GetHour())
               // this is the rigth hour
                  if( ts_time->tm_min == m_tAlarmLigthTime[i].GetMinute())
                     if(ts_time->tm_sec == 0)
                     {
                        DPRINTLN("The Ligth goes on!");            
                       // DPRINT(m_daysOfTheWeek[*ts_time->tm_wday]);DPRINT(", ");DPRINT(day(actTime));DPRINT(".");DPRINT(month(actTime));DPRINT(".");DPRINTLN(year(actTime));
                       // DPRINT("Time: "); DPRINT(hour(actTime)); DPRINT(":");DPRINT(minute(actTime));DPRINT(":");DPRINTLN(second(actTime));
                        StartLigthSequenz();
                     }
         }
      }
   }

   for( int i = 0; i < sizeof(m_tAlarmLigthTime); i++)
   {
      if( m_tAlarmLigthTime[i].IsActive())
      {
            // this is the rigth day
         for ( auto weekday = m_tAlarmLigthTime[i].GetWeekDays()->begin(); weekday != m_tAlarmLigthTime[i].GetWeekDays()->end(); ++weekday )
         {
            if( ts_time->tm_wday == *weekday)
               // this is the rigth hour
               if( ts_time->tm_hour == m_tAlarmLigthTime[i].GetHour())
               // this is the rigth hour
                  if( ts_time->tm_min == m_tAlarmLigthTime[i].GetMinute())
                     if(ts_time->tm_sec == 0)
                     {
                        DPRINTLN("Time to wake up!");            
                        //DPRINT(m_daysOfTheWeek[ts_time->tm_wday]);DPRINT(", ");DPRINT(day(actTime));DPRINT(".");DPRINT(month(actTime));DPRINT(".");DPRINTLN(year(actTime));
                        //DPRINT("Time: "); DPRINT(hour(actTime)); DPRINT(":");DPRINT(minute(actTime));DPRINT(":");DPRINTLN(second(actTime));
                        StartAlarmSequenz();
                     }
         }
      }
   }

}

/**
 * @brief 
 * 
 * @param actTime 
 */
void cLigthAlarmClock::CheckTimeIsExpired(time_t actTime)
{
   CheckTimeStamp(actTime);

   if( m_IsSunriseStarted )
      Sunrise();

}

/**
 * @brief Runtime
 * 
 */
void cLigthAlarmClock::Runtime(time_t actTime)
{
   uint32_t currentMillis = millis();
   static uint32_t previousMillis1000ms = 1000;
   static time_t lastTime = 0;

   if(lastTime < actTime)
   {
      lastTime = actTime;
      CheckTimeIsExpired(actTime);
   }

   // 1000ms Schleife
   if (currentMillis - previousMillis1000ms >= 1000)
   {
      previousMillis1000ms = currentMillis;
      //DPRINT("actTime: "); DPRINTLN(String(actTime));
   }
}

/**
 * @brief Switch Leds off
 * 
 */
void cLigthAlarmClock::LedsOff()
{
   fill_solid(m_leds, m_ucNUM_LEDS, CRGB::Black);
   FastLED.show();
   m_uiSunriseIndex = 0;
}

/**
 * @brief Sunrises
 * 
 */
void cLigthAlarmClock::Sunrise()
{
   if (m_uiSunriseIndex <= 255)
   {
      CRGB color = ColorFromPalette(m_sunrisePal, m_uiSunriseIndex);
      DPRINT("Count: "); DPRINT(m_uiSunriseIndex);
      DPRINT(" Red: "); DPRINT(color.red);
      DPRINT(" Green: "); DPRINT(color.green);
      DPRINT(" Blue: "); DPRINTLN(color.blue);
      // fill the entire strip with the current color
      fill_solid(m_leds, m_ucNUM_LEDS, color);
      FastLED.show();
      ++m_uiSunriseIndex;
   }
}

void cLigthAlarmClock::SetRgb(String strMsg)
{
   uint32_t raw_color = std::strtoul(strMsg.c_str(), 0, 16);
   DPRINTLN( "Raw: " + String(raw_color));
   CRGB color(raw_color);   
   fill_solid(m_leds, m_ucNUM_LEDS, color);
   FastLED.show();
}


#endif //INC_LICHTWECKER_H
