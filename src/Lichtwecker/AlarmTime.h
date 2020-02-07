#ifndef INC_ALARMTIME_H
#define INC_ALARMTIME_H


/**
 * @brief 
 * 
 */
class cAlarmTimeBase
{
   public:
      cAlarmTimeBase();
      ~cAlarmTimeBase();

      void SetAlarmTime( size_t const sizeArrayWeekDays, int8_t * const arrayWeekDays, int8_t cAlarmHour, int8_t cAlarmMinute);
      void SetAlarmOn() {m_bfActive = true;};
      void SetAlarmOff() {m_bfActive = false;};

   protected:
      int8_t m_cActiveWeekDays[7];
      int8_t m_cHour;
      int8_t m_cMinute;
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
void cAlarmTimeBase::SetAlarmTime( size_t const sizeArrayWeekDays, int8_t * const arrayWeekDays, int8_t cAlarmHour, int8_t cAlarmMinute)
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
   memset( m_cActiveWeekDays, -1, 7 );
}


class cAlarmTime : cAlarmTimeBase
{
   public:
      cAlarmTime();
      ~cAlarmTime();

   private:
};

class cAlarmLigthTime : cAlarmTimeBase
{
   public:
      cAlarmLigthTime();
      ~cAlarmLigthTime();

   private:
};


#endif // INC_ALARMTIME_H