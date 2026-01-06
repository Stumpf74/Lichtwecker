#ifndef ALARM_MANAGER_H
#define ALARM_MANAGER_H

#include <cstdint>
#include "Config.h"

class AlarmManager
{
public:
    static const int NUM_ALARMS = 4;

    // Bitmaske für die Wochentage
    // tm_wday aus time.h: So=0, Mo=1, Di=2, Mi=3, Do=4, Fr=5, Sa=6
    enum DayOfWeek
    {
        Sunday = 1 << 0,
        Monday = 1 << 1,
        Tuesday = 1 << 2,
        Wednesday = 1 << 3,
        Thursday = 1 << 4,
        Friday = 1 << 5,
        Saturday = 1 << 6,
        Weekdays = Monday | Tuesday | Wednesday | Thursday | Friday,
        Weekend = Sunday | Saturday,
        AllDays = Sunday | Monday | Tuesday | Wednesday | Thursday | Friday | Saturday
    };

    struct AlarmSetting
    {
        bool enabled;
        bool sunriseEnabled; // NEU: Schalter für den Sonnenaufgang
        uint8_t hour;
        uint8_t minute;
        uint8_t days; // Bitmaske mit den `DayOfWeek`-Werten
        bool triggeredToday; // Flag, um zu verhindern, dass der Alarm mehrmals am Tag ausgelöst wird
    };

    static AlarmManager *GetInstance();

    void begin();
    void handleAlarms();

    // Methoden zum Konfigurieren der Wecker
    void setAlarm(int index, bool enabled, bool sunriseEnabled, uint8_t hour, uint8_t minute, uint8_t days);
    const AlarmSetting &getAlarm(int index) const;

private:
    AlarmManager();
    ~AlarmManager() = default;
    AlarmManager(const AlarmManager &) = delete;
    AlarmManager &operator=(const AlarmManager &) = delete;

    void checkAndTriggerAlarm(int index);
    void saveAlarms();
    void loadAlarms();

    int m_lastDayChecked; // Um den täglichen Reset der Trigger-Flags zu steuern
    AlarmSetting m_alarms[NUM_ALARMS];
};

#endif // ALARM_MANAGER_H