#include "SunTimeManager.h"
#include "Log.h"
#include "Config.h"
#include "AppContext.h"
// Wir verwenden den relativen Pfad, um sicherzustellen, dass der Compiler
// die richtige Header-Datei aus der jpb10-Bibliothek findet und nicht
// eine andere, gleichnamige Datei (z.B. aus dem TFT_eSPI Beispiel).
#include "SolarCalculator.h"

SunTimeManager::SunTimeManager() : 
    m_sunriseTime(0), 
    m_sunsetTime(0), 
    m_transitTime(0),
    m_lastDayCalculated(-1),
    m_ntpSyncedOnce(false) // Initialisiere das neue Flag
{
}

SunTimeManager* SunTimeManager::GetInstance()
{
    static SunTimeManager instance;
    return &instance;
}

void SunTimeManager::begin()
{
    LOG_INFO("Init SunTimeManager");
    // Die erste Berechnung wird in notifyNtpSync() ausgelöst, sobald die NTP-Zeit synchronisiert ist.
    // handle() wird dann für tägliche Updates verwendet.
}

/**
 * @brief Wird von NTPTime aufgerufen, sobald die Zeit erfolgreich synchronisiert wurde.
 * Löst die erste Berechnung der Sonnenzeiten aus.
 */
void SunTimeManager::notifyNtpSync()
{
    if (!m_ntpSyncedOnce) {
        LOG_INFO("NTP-Zeit synchronisiert, führe erste Sonnenzeiten-Berechnung durch.");
        calculateTimesForToday();
        m_ntpSyncedOnce = true;
    }
}

void SunTimeManager::handle()
{
    // Nur fortfahren, wenn die NTP-Zeit mindestens einmal synchronisiert wurde
    if (!m_ntpSyncedOnce) {
        return;
    }

    time_t now = appContext.ntpTime.getEpochTime();
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    // Prüfe, ob ein neuer Tag begonnen hat und die Berechnung fällig ist
    if (timeinfo.tm_yday != m_lastDayCalculated)
    {
        calculateTimesForToday();
        m_lastDayCalculated = timeinfo.tm_yday;
    }
}

void SunTimeManager::calculateTimesForToday()
{
    time_t now = appContext.ntpTime.getEpochTime();
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    double lat = Config::GetInstance()->GetLatitude();
    double lon = Config::GetInstance()->GetLongitude();

    // Berechne Sonnenauf- und -untergang. Die Bibliothek erwartet keinen Zeitzonen-Offset.
    // Die Ergebnisse sind Stunden in UTC.
    double sunriseUTC, sunsetUTC, transitUTC; // transit wird nicht verwendet, muss aber übergeben werden.
    calcSunriseSunset(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, lat, lon, transitUTC, sunriseUTC, sunsetUTC);

    // Konvertiere die UTC-Stunden in eine lokale time_t
    // 1. Erstelle eine tm-Struktur für den heutigen Tag um 00:00:00 UTC
    struct tm tm_utc_base = {0};
    tm_utc_base.tm_year = timeinfo.tm_year;
    tm_utc_base.tm_mon = timeinfo.tm_mon;
    tm_utc_base.tm_mday = timeinfo.tm_mday;

    // timegm() ist nicht portabel. Wir verwenden einen Trick, um das gleiche Ergebnis zu erzielen:
    // 1. Speichere die aktuelle Zeitzone.
    char* original_tz = getenv("TZ");
    // 2. Setze die Zeitzone temporär auf UTC.
    setenv("TZ", "UTC", 1);
    tzset();
    // 3. mktime() interpretiert tm_utc_base jetzt als UTC und gibt den korrekten UTC-Epochenwert zurück.
    time_t today_utc_epoch = mktime(&tm_utc_base);
    // 4. Setze die ursprüngliche Zeitzone wieder zurück.
    setenv("TZ", original_tz, 1);
    tzset();

    // 2. Addiere die Stunden (in Sekunden) zum UTC-Start des Tages.
    // Das Ergebnis ist die korrekte Epochenzeit in UTC.
    // Die localtime()-Funktion wird diese dann korrekt in die lokale Zeitzone umwandeln.
    m_sunriseTime = today_utc_epoch + static_cast<time_t>(sunriseUTC * 3600.0);
    m_transitTime = today_utc_epoch + static_cast<time_t>(transitUTC * 3600.0);
    m_sunsetTime = today_utc_epoch + static_cast<time_t>(sunsetUTC * 3600.0);

    char buffer[30];
    // localtime() konvertiert die UTC-Epochenzeit in die lokale Zeit deines Geräts.
    strftime(buffer, sizeof(buffer), "%H:%M:%S", localtime(&m_sunriseTime));
    LOG_PRINTF(Log::LOG_LEVEL_INFO, "Sonnenaufgang heute: %s", buffer);
    strftime(buffer, sizeof(buffer), "%H:%M:%S", localtime(&m_transitTime));
    LOG_PRINTF(Log::LOG_LEVEL_INFO, "Sonnenhöchststand heute: %s", buffer);
    strftime(buffer, sizeof(buffer), "%H:%M:%S", localtime(&m_sunsetTime));
    LOG_PRINTF(Log::LOG_LEVEL_INFO, "Sonnenuntergang heute: %s", buffer);
}
