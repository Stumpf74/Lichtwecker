#ifndef INC_TOUCHSENSOR_H
#define INC_TOUCHSENSOR_H

#include "time.h"
#include "debug.h"
#include "Avarage.h"




/**
 * @brief class cTouchSensor 
 * 
 */
class cTouchSensor
{
   public:
  
   cTouchSensor(const uint32_t PinNumber, const uint32_t ThresholdPercent, const uint32_t AvgSize = 32);
   ~cTouchSensor();

   void Setup();
   void Runtime(time_t actTime);
   bool IsPressed() {return m_bfIsPressed;};
   bool IsShortPressed();
   bool IsLongPressed();

private:


private:
   uint32_t m_uiPinNumber;
   uint32_t m_uiTouchValueMax;
   CAverage<int32_t> m_cAvgTouch;
   CAverage<int32_t> m_cAvgTouchMax;
   bool m_bfIsPressed;
   uint32_t m_uiThresholdPercent;
   time_t m_uiTimePressed;
   bool m_bfIsShortPressed;
   bool m_bfIsLongPressed;
   uint32_t m_uiLastCheckTouchsensorMax;


};



/**
 * @brief Construct a new c Touch Sensor::c Touch Sensor object
 * 
 */
cTouchSensor::cTouchSensor( const uint32_t PinNumber, const uint32_t ThresholdPercent, const uint32_t AvgSize ) : m_cAvgTouch(AvgSize,100), m_cAvgTouchMax(64,100), 
   m_uiPinNumber(PinNumber), m_uiTouchValueMax(0), m_bfIsPressed(false), m_uiThresholdPercent(ThresholdPercent), m_bfIsShortPressed(false), m_bfIsLongPressed(false)
{

}

/**
 * @brief Destroy the c Touch Sensor::c Touch Sensor object
 * 
 */
cTouchSensor::~cTouchSensor()
{
}


/**
 * @brief init the class
 * 
 */
void cTouchSensor::Setup()
{
   m_cAvgTouch.InitBuffer(touchRead(m_uiPinNumber));
   m_cAvgTouchMax.InitBuffer(touchRead(m_uiPinNumber));
}


/**
 * @brief Runtime
 * 
 */
void cTouchSensor::Runtime(time_t actTime)
{
   static time_t lastTime = 0;

   uint32_t uivaluetouch = touchRead(m_uiPinNumber);

   m_cAvgTouch.Add(uivaluetouch);
   
   
   if( actTime > (m_uiLastCheckTouchsensorMax + 15000))
   {
      static uint32_t uiLastTouchValueMax = 0;
      m_uiLastCheckTouchsensorMax = actTime;
      m_cAvgTouchMax.Add(uivaluetouch);

      m_uiTouchValueMax = m_cAvgTouchMax.Get() ;
   
      if( uiLastTouchValueMax != m_uiTouchValueMax )
         Log::PrintF("%i:          new Max: %i", m_uiPinNumber, m_uiTouchValueMax);

   }


   uint32_t uiAvg = m_cAvgTouch.Get();
   double threshold = m_uiTouchValueMax - (m_uiTouchValueMax / 100.0 * m_uiThresholdPercent);

   if (uiAvg <= threshold)
   {
      if( m_bfIsPressed == false)
      {
         // First entry
         m_uiTimePressed = actTime;
      }
      m_bfIsPressed = true;
   }
   else
   {
      if( m_bfIsPressed == true)
      {
         // First entry
         uint32_t uiPressTime = actTime - m_uiTimePressed; 

         //Log::PrintF("%i:          Time: %i", m_uiPinNumber, uiPressTime);

         if( uiPressTime > 50 )
         { // minimum time is 50ms

            if( uiPressTime < 500 )
            { // short time
               m_bfIsShortPressed = true;
            }
            else if( uiPressTime < 2500 )
            { // long time time
               m_bfIsLongPressed = true;
            }
         }

     }
      m_bfIsPressed = false;
   }
   
   lastTime = actTime;
}


bool cTouchSensor::IsShortPressed() 
{ 
   if( m_bfIsShortPressed )
   {  
      m_bfIsShortPressed = false;
      return true;
   }
   else
   {
      return false;
   }
};

bool cTouchSensor::IsLongPressed() 
{ 
   if( m_bfIsLongPressed )
   {  
      m_bfIsLongPressed = false;
      return true;
   }
   else
   {
      return false;
   }
};

#endif 
