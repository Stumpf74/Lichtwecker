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
      typedef enum SensorState
      {
         Inactive = 0,
         ShortPressed,
         LongPressed

      }SensorState_t;

   cTouchSensor(const uint32_t PinNumber, const uint32_t ThresholdPercent, const uint32_t AvgSize = 32);
   ~cTouchSensor();

   void Setup();
   void Runtime();
   bool IsPressed() {return m_bfIsPressed;};
   bool IsShortPressed();
   bool IsLongPressed();

private:
   SensorState_t CheckPinIsTouched();
   void CheckForMaxValue(uint32_t uivaluetouch);


private:
   uint32_t m_uiPinNumber;
   double m_uiTouchValueMax;
   CAverage<int32_t> m_cAvgTouch;
   CAverage<int32_t> m_cAvgTouchMax;
   bool m_bfIsPressed;
   uint32_t m_uiThresholdPercent;
   time_t m_uiStartTimePressed;
   bool m_bfIsShortPressed;
   bool m_bfIsLongPressed;
   uint32_t m_uiLastCheckTouchsensorMax;
   uint32_t m_uiDelayReadLastTouchPin;
   uint32_t m_actTime;
   SensorState_t m_actSensorState;


};


/**
 * @brief Construct a new c Touch Sensor::c Touch Sensor object
 * 
 */
cTouchSensor::cTouchSensor( const uint32_t PinNumber, const uint32_t ThresholdPercent, const uint32_t AvgSize ) : m_cAvgTouch(AvgSize,10), m_cAvgTouchMax(64,100), 
   m_uiPinNumber(PinNumber), m_uiTouchValueMax(0), m_bfIsPressed(false), m_uiThresholdPercent(ThresholdPercent), m_bfIsShortPressed(false), m_bfIsLongPressed(false),
   m_uiDelayReadLastTouchPin(0), m_actTime(0), m_actSensorState(Inactive)
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
   uint32_t uiTouchValue = touchRead(m_uiPinNumber);
   Log::PrintF("%i:          Init TouchValue: %i", m_uiPinNumber, uiTouchValue);
   m_cAvgTouch.InitBuffer(uiTouchValue);
   m_cAvgTouchMax.InitBuffer(uiTouchValue);
}

/**
 * 
*/
cTouchSensor::SensorState_t cTouchSensor::CheckPinIsTouched()
{
   // Get max Value
   m_uiTouchValueMax = m_cAvgTouchMax.Get();
   // Get act Value
   double uiAvg = m_cAvgTouch.Get();
   // calc threshold
   double threshold = m_uiTouchValueMax - (m_uiTouchValueMax / 100.0f * m_uiThresholdPercent);
   uint32_t uiPressTime = 0;

   if (uiAvg <= threshold)
   {
      if( m_bfIsPressed == false)
      {
         // First entry
         m_uiStartTimePressed = m_actTime;
         m_bfIsPressed = true;
         Log::PrintF("Pressed: %i:          threshold: %2.1f              Avg: %2.1f", m_uiPinNumber, threshold, uiAvg);
      }
   }
   else
   {
      if( m_bfIsPressed == true)
         Log::PrintF("Reset: %i:          threshold: %2.1f              Avg: %2.1f", m_uiPinNumber, threshold, uiAvg);

      m_bfIsPressed = false;
      m_actSensorState = Inactive;
      m_uiStartTimePressed = m_actTime;
   }

   uiPressTime = m_actTime - m_uiStartTimePressed; 

   switch (m_actSensorState)
   {
      case Inactive:
         if( uiPressTime > 50 )
         { // minimum time is 50ms
            if( uiPressTime < 500 )
            { // short time
               m_bfIsShortPressed = true;
               m_actSensorState = ShortPressed;
               Log::PrintF("Short: %i:          threshold: %2.1f              Avg: %2.1f", m_uiPinNumber, threshold, uiAvg);
            }
         }
         break;
   
      case ShortPressed:
         if( uiPressTime > 2500 )
         { // long time time
            m_bfIsLongPressed = true;
            m_actSensorState = LongPressed;
            Log::PrintF("Long: %i:          threshold: %2.1f              Avg: %2.1f", m_uiPinNumber, threshold, uiAvg);
         }
         break;

      case LongPressed:
         /* code */
         break;

      default:
         m_bfIsPressed = false;
         m_actSensorState = Inactive;
         break;
   }   



   return m_actSensorState;
}

/**
 * 
*/
void cTouchSensor::CheckForMaxValue(uint32_t uivaluetouch)
{
   // check only 15 seconds
   if( m_actTime > (m_uiLastCheckTouchsensorMax + 15000))
   {
      static uint32_t uiLastTouchValueMax = 0;
      m_uiLastCheckTouchsensorMax = m_actTime;
      
      if( uivaluetouch != 0 )
         m_cAvgTouchMax.Add(uivaluetouch);
   
      m_uiTouchValueMax = m_cAvgTouchMax.Get() ;
      if( uiLastTouchValueMax != m_uiTouchValueMax )
      {
         uiLastTouchValueMax = m_uiTouchValueMax;
         Log::PrintF("%i:          new Max: %f", m_uiPinNumber, m_uiTouchValueMax);
      }
   }
}

/**
 * @brief Runtime
 * 
 */
void cTouchSensor::Runtime()
{
   //static time_t lastTime = 0;
   uint32_t uivaluetouch = 0;
   m_actTime = millis();

   // we read every 50 milli sec the value of the pin
   if( m_actTime > ( m_uiDelayReadLastTouchPin + 5))
   {
      m_uiDelayReadLastTouchPin = m_actTime; 
      
      uivaluetouch = touchRead(m_uiPinNumber);
      //Log::PrintF("%i:          act: %i", m_uiPinNumber, uivaluetouch);
      m_cAvgTouch.Add(uivaluetouch); // fill pin avg
   }

   SensorState_t PinState = CheckPinIsTouched();

   if( PinState == Inactive )
   {  
      uivaluetouch = touchRead(m_uiPinNumber);
      CheckForMaxValue(uivaluetouch);
   }

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
