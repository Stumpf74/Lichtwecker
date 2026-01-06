#ifndef SUN_TIME_MANAGER_H
#define SUN_TIME_MANAGER_H

#include <time.h>

/**
 * @class SunTimeManager
 * @brief Berechnet und verwaltet die Zeiten für Sonnenaufgang und -untergang.
 *
 * Diese Klasse ist als Singleton implementiert. Sie berechnet einmal täglich
 * die relevanten Sonnenzeiten für den konfigurierten Standort.
 */
class SunTimeManager
{
public:
    static SunTimeManager* GetInstance();

    void begin();
    void handle();
    void notifyNtpSync(); // Neue Methode, die aufgerufen wird, wenn NTP-Zeit synchronisiert ist

    time_t getSunriseTime() const { return m_sunriseTime; }
    time_t getSunsetTime() const { return m_sunsetTime; }
    time_t getTransitTime() const { return m_transitTime; }

private:
    SunTimeManager();
    ~SunTimeManager() = default;
    SunTimeManager(const SunTimeManager&) = delete;
    SunTimeManager& operator=(const SunTimeManager&) = delete;

    void calculateTimesForToday();

    time_t m_sunriseTime;
    time_t m_sunsetTime;
    time_t m_transitTime;

    int m_lastDayCalculated; // Um die Berechnung nur einmal pro Tag durchzuführen
    bool m_ntpSyncedOnce;    // Flag, um sicherzustellen, dass die erste Berechnung nach NTP-Sync einmalig erfolgt
};


#endif // SUN_TIME_MANAGER_H
