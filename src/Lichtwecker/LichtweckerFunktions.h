#ifndef INC_LICHTWECKER_H
#define INC_LICHTWECKER_H

#include <FastLED.h>
#include "debug.h"

//#define UPDATES_PER_SECOND 100

DEFINE_GRADIENT_PALETTE(sunrise_gp){
      0, 0, 0, 0,        //schwarz
      128, 100, 220, 0,  //rot - gelb
      224, 240, 240, 0,  //gelb
      255, 128, 128, 240 //sehr helles Blau
};



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
   void SetWackupTime(const uint32_t ui);

private:
   cLigthAlarmClock();
   void Sunrise();
   void LedsOff();
   void SetLigthStartTime();
   void CalculateDelayTime();
   void CheckTimeIsExpired(time_t Time);

private:

   CRGBPalette256 m_sunrisePal = sunrise_gp;

   static cLigthAlarmClock *m_ptrInstance;

   static const uint8_t m_ucLED_PIN = 23;
   static const uint8_t m_ucNUM_LEDS = 120;
   //#define BRIGHTNESS 50
   CRGB m_leds[m_ucNUM_LEDS];

   uint32_t m_uiLigthStepDelay;
   static const char m_daysOfTheWeek[7][3];
};

const char cLigthAlarmClock::m_daysOfTheWeek[7][3] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};

/**
 * @brief 
 * 
 * @param ui 
 */
void cLigthAlarmClock::SetWackupTime(const uint32_t ui)
{

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
cLigthAlarmClock::cLigthAlarmClock() : m_uiLigthStepDelay(0)
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
   FastLED.addLeds<WS2812B, m_ucLED_PIN, RGB>(m_leds, m_ucNUM_LEDS).setCorrection(TypicalLEDStrip);
   LedsOff();
}

/**
 * @brief stops the alarm clock
 * 
 */
void cLigthAlarmClock::Stop()
{
   LedsOff();
}

/**
 * @brief 
 * 
 * @param actTime 
 */
void cLigthAlarmClock::CheckTimeIsExpired(time_t actTime)
{
   DPRINT(m_daysOfTheWeek[dayOfWeek(actTime)]);DPRINT(", ");DPRINT(day(actTime));DPRINT(".");DPRINT(month(actTime));DPRINT(".");DPRINTLN(year(actTime));
   DPRINT("Time: "); DPRINT(hour(actTime)); DPRINT(":");DPRINT(minute(actTime));DPRINT(":");DPRINTLN(second(actTime));
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
      //Sunrise();
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
}

/**
 * @brief Sunrises
 * 
 */
void cLigthAlarmClock::Sunrise()
{
   static uint16_t heatIndex = 0; // start out at 0
   CRGB color = ColorFromPalette(m_sunrisePal, heatIndex);

   // fill the entire strip with the current color
   fill_solid(m_leds, m_ucNUM_LEDS, color);

   FastLED.show();
   //        FastLED.delay(250);

   heatIndex++;
   if (heatIndex >= 255)
   {
      heatIndex = 0;
   }
}

#endif //INC_LICHTWECKER_H
