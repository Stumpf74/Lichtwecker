#ifndef INC_ALARMTIME_H
#define INC_ALARMTIME_H

#include <vector>
#include <array>


/**
 * @brief 
 * 
 */
class cAlarmTimeBase
{
   public:
      cAlarmTimeBase();
      ~cAlarmTimeBase();

      void SetAlarmTime( size_t const sizeArrayWeekDays, uint8_t * const arrayWeekDays, uint8_t cAlarmHour, uint8_t cAlarmMinute);
      void SetAlarmOn() {m_bfActive = true;};
      void SetAlarmOff() {m_bfActive = false;};
      std::array<int8_t,7> * GetWeekDays() {return &m_cActiveWeekDays;};
      uint8_t GetHour() { return m_cHour;};
      uint8_t GetMinute() { return m_cMinute;};
      bool IsActive() { return m_bfActive;};

   protected:
      std::array<int8_t,7> m_cActiveWeekDays;
      uint8_t m_cHour;
      uint8_t m_cMinute;
      bool m_bfActive;

   private:
};




/**
 * @brief 
 * 
 * @param sizeArrayWeekDays 
 * @param arrayWeekDays 
 * @param cAlarmHour 
 * @param cAlarmMinute 
 */
void cAlarmTimeBase::SetAlarmTime( size_t const sizeArrayWeekDays, uint8_t * const arrayWeekDays, uint8_t cAlarmHour, uint8_t cAlarmMinute)
{

   for (size_t i  = 0; i < sizeArrayWeekDays; i++)
   {
      this->m_cActiveWeekDays[i] = arrayWeekDays[i];
   }
   this->m_cHour = cAlarmHour;
   this->m_cMinute = cAlarmMinute;
}

/**
 * @brief Construct a new c Alarm Time Base::c Alarm Time Base object
 * 
 */
cAlarmTimeBase::cAlarmTimeBase() : m_cHour(-1), m_cMinute(-1), m_bfActive(false)
{
   m_cActiveWeekDays = {(int8_t)-1,(int8_t)-1,(int8_t)-1,(int8_t)-1,(int8_t)-1,(int8_t)-1,(int8_t)-1};
}


class cAlarmTime : public cAlarmTimeBase
{
   public:
      cAlarmTime();
      ~cAlarmTime();

   private:
};


cAlarmTime::cAlarmTime()
{

}

cAlarmTime::~cAlarmTime()
{
   
}

class cAlarmLigthTime : public cAlarmTimeBase
{
   public:
      cAlarmLigthTime();
      ~cAlarmLigthTime();

   private:
};


cAlarmLigthTime::cAlarmLigthTime()
{

}

cAlarmLigthTime::~cAlarmLigthTime()
{
   
}


#endif // INC_ALARMTIME_H