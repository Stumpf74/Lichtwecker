#include "AlarmManager.h"
#include "Log.h"
#include "AppContext.h" // Für Zugriff auf appContext.ntpTime
#include <ctime>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "LightController.h"
#include "SunTimeManager.h" // Hinzufügen, um auf die Sonnenaufgangszeit zugreifen zu können
#include "AudioManager.h"

const char* ALARMS_CONFIG_FILE = "/alarms.json";


AlarmManager::AlarmManager() : m_lastDayChecked(-1),
                               m_ringingAlarmIndex(-1),
                               m_snoozedAlarmIndex(-1),
                               m_snoozeUntilEpoch(0)
{
    // Initialisiere alle Wecker als deaktiviert
    for (int i = 0; i < NUM_ALARMS; ++i)
    {
        m_alarms[i] = {false, true, false, 7, 0, 0, false, false}; // sunriseEnabled standardmäßig auf true
    }
}

AlarmManager *AlarmManager::GetInstance()
{
    static AlarmManager instance;
    return &instance;
}

void AlarmManager::begin()
{
    LOG_INFO("Init AlarmManager");
    loadAlarms(); // Gespeicherte Weckzeiten aus dem LittleFS laden
}

void AlarmManager::handleAlarms()
{
    if (m_snoozedAlarmIndex >= 0 && time(nullptr) >= m_snoozeUntilEpoch)
    {
        const int alarmIndex = m_snoozedAlarmIndex;
        m_snoozedAlarmIndex = -1;
        m_snoozeUntilEpoch = 0;
        if (alarmIndex < NUM_ALARMS && m_alarms[alarmIndex].enabled)
        {
            startRinging(alarmIndex);
        }
    }

    for (int i = 0; i < NUM_ALARMS; ++i)
    {
        checkAndTriggerAlarm(i);
    }
}

bool AlarmManager::isRinging() const
{
    return m_ringingAlarmIndex >= 0;
}

void AlarmManager::snooze()
{
    if (!isRinging())
    {
        return;
    }

    const int alarmIndex = m_ringingAlarmIndex;
    m_ringingAlarmIndex = -1;
    m_snoozedAlarmIndex = alarmIndex;
    m_snoozeUntilEpoch = time(nullptr) + 5 * 60;
    if (m_alarms[alarmIndex].radioEnabled)
    {
        AudioManager::GetInstance()->stopStream();
    }
    LOG_PRINTF(Log::LOG_LEVEL_INFO, "Wecker %d für 5 Minuten zurückgestellt.", alarmIndex + 1);
}

void AlarmManager::dismiss()
{
    if (!isRinging())
    {
        return;
    }

    const int alarmIndex = m_ringingAlarmIndex;
    m_ringingAlarmIndex = -1;
    m_snoozedAlarmIndex = -1;
    m_snoozeUntilEpoch = 0;
    if (m_alarms[alarmIndex].radioEnabled)
    {
        AudioManager::GetInstance()->stopStream();
    }
    LightController::GetInstance()->stop();
    LOG_PRINTF(Log::LOG_LEVEL_INFO, "Wecker %d für heute beendet.", alarmIndex + 1);
}

void AlarmManager::startRinging(int index)
{
    m_ringingAlarmIndex = index;
    LightController::GetInstance()->setColor(CRGB::White, 255);
    LOG_PRINTF(Log::LOG_LEVEL_INFO, "Wecker %d ausgelöst!", index + 1);

    const auto& alarm = m_alarms[index];
    if (alarm.radioEnabled)
    {
        const auto& stations = Config::GetInstance()->GetRadioStations();
        if (!stations.empty())
        {
            AudioManager::GetInstance()->playStream(stations[0].url);
        }
        else
        {
            LOG_ERROR("Radio-Wecker aktiviert, aber keine Sender konfiguriert.");
        }
    }
}

void AlarmManager::checkAndTriggerAlarm(int index)
{
    if (index < 0 || index >= NUM_ALARMS) return;

    auto &alarm = m_alarms[index]; // Referenz, um 'triggeredToday' ändern zu können

    if (!alarm.enabled)
    {
        return;
    }

    time_t now = appContext.ntpTime.getEpochTime();
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    // Tägliches Zurücksetzen des Trigger-Flags
    if (timeinfo.tm_yday != m_lastDayChecked)
    {
        LOG_DEBUG("Neuer Tag, setze Alarm-Trigger zurück.");
        for (int i = 0; i < NUM_ALARMS; ++i) {
            m_alarms[i].triggeredToday = false;
            m_alarms[i].alarmTriggeredToday = false;
        }
        m_lastDayChecked = timeinfo.tm_yday;
    }

    // Prüfe, ob der heutige Wochentag im Bitfeld gesetzt ist
    // tm_wday: So=0, Mo=1, ..., Sa=6
    bool todayIsAlarmDay = (alarm.days & (1 << timeinfo.tm_wday));

    // Berechne die Startzeit für den Sonnenaufgang (30 Minuten vor dem Alarm)
    struct tm alarmStartTime = timeinfo; // Kopiere aktuelle Zeitstruktur
    alarmStartTime.tm_hour = alarm.hour;
    alarmStartTime.tm_min = alarm.minute;
    alarmStartTime.tm_sec = 0;

    // Konvertiere zu time_t, um sicher zu subtrahieren
    time_t alarmTimeEpoch = mktime(&alarmStartTime);
    time_t wakeUpStartTimeEpoch = alarmTimeEpoch - (30 * 60); // 30 Minuten früher

    // Prüfe, ob die aktuelle Zeit der Startzeit des Sonnenaufgangs entspricht
    // Die Bedingung ist nur in der ersten Minute nach dem Startzeitpunkt wahr, um Fehlauslösungen zu vermeiden.
    if (todayIsAlarmDay && !alarm.triggeredToday && now >= wakeUpStartTimeEpoch && now < (wakeUpStartTimeEpoch + 60))
    {
        alarm.triggeredToday = true; // Alarm für heute als ausgelöst markieren, egal ob mit oder ohne Licht

        // Prüfe, ob der Sonnenaufgang für diesen Alarm aktiviert ist
        if (alarm.sunriseEnabled)
        {
            // NEUE BEDINGUNG: Prüfe den Zeitunterschied zum echten Sonnenaufgang
            time_t actualSunriseTime = SunTimeManager::GetInstance()->getSunriseTime();
            const long minDifferenceSeconds = 1.5 * 3600; // 1.5 Stunden in Sekunden

            if (actualSunriseTime > 0 && (alarmTimeEpoch - actualSunriseTime) >= minDifferenceSeconds)
            {
                LOG_PRINTF(Log::LOG_LEVEL_INFO, "Sonnenaufgang für ALARM %d gestartet! (Geplante Weckzeit: %02d:%02d)", index + 1, alarm.hour, alarm.minute);
                // Dauer: 25 Minuten, damit er 5 Minuten vor dem Alarm fertig ist.
                LightController::GetInstance()->startWakeUpRoutine(25);
            }
            else
            {
                LOG_PRINTF(Log::LOG_LEVEL_INFO, "Sonnenaufgang für ALARM %d übersprungen. Weckzeit ist zu nah am echten Sonnenaufgang.", index + 1);
            }
        }
        // Hier könnte später ein "else" stehen, um z.B. nur einen Ton abzuspielen
    }

    if (todayIsAlarmDay && !alarm.alarmTriggeredToday && now >= alarmTimeEpoch && now < (alarmTimeEpoch + 60))
    {
        alarm.alarmTriggeredToday = true;
        if (!isRinging())
        {
            startRinging(index);
        }
    }
}

void AlarmManager::setAlarm(int index, bool enabled, bool sunriseEnabled, bool radioEnabled, uint8_t hour, uint8_t minute, uint8_t days)
{
    if (index >= 0 && index < NUM_ALARMS)
    {
        m_alarms[index] = {enabled, sunriseEnabled, radioEnabled, hour, minute, days, false, false}; // Trigger beim Setzen zurücksetzen

        // Erstelle einen lesbaren String für die Wochentage
        String daysStr;
        const char* dayNames[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
        bool firstDay = true;
        for (int i = 0; i < 7; ++i) 
        {
            if (days & (1 << i)) 
            {
                if (!firstDay) 
                {
                    daysStr += ",";
                }
                daysStr += dayNames[i];
                firstDay = false;
            }
        }
        if (daysStr.isEmpty()) 
        {
            daysStr = "Nie";
        }

        LOG_PRINTF(Log::LOG_LEVEL_INFO, "Wecker %d gesetzt: Aktiv=%d, Sonnenaufgang=%d, Radio=%d, Zeit=%02d:%02d, Tage=%s", 
                index + 1, enabled, sunriseEnabled, radioEnabled, hour, minute, daysStr.c_str());
        saveAlarms(); // Einstellungen im LittleFS speichern
    }
}

const AlarmManager::AlarmSetting &AlarmManager::getAlarm(int index) const
{
    return m_alarms[index];
}

void AlarmManager::saveAlarms()
{
    JsonDocument doc;
    JsonArray alarmsArray = doc.to<JsonArray>();

    for (int i = 0; i < NUM_ALARMS; ++i)
    {
        JsonObject alarmObj = alarmsArray.add<JsonObject>();
        alarmObj["enabled"] = m_alarms[i].enabled;
        alarmObj["sunriseEnabled"] = m_alarms[i].sunriseEnabled;
        alarmObj["radioEnabled"] = m_alarms[i].radioEnabled;
        alarmObj["hour"] = m_alarms[i].hour;
        alarmObj["minute"] = m_alarms[i].minute;
        alarmObj["days"] = m_alarms[i].days;
        // 'triggeredToday' wird nicht gespeichert, da es ein Laufzeit-Status ist
    }

    File configFile = LittleFS.open(ALARMS_CONFIG_FILE, "w");
    if (!configFile)
    {
        LOG_ERROR("Fehler beim Öffnen der Wecker-Konfigurationsdatei zum Schreiben");
        return;
    }

    if (serializeJson(doc, configFile) == 0)
    {
        LOG_ERROR("Fehler beim Schreiben in die Wecker-Konfigurationsdatei");
    }
    else
    {
        LOG_INFO("Wecker-Konfiguration erfolgreich gespeichert.");
    }
    configFile.close();
}

void AlarmManager::loadAlarms()
{
    if (!LittleFS.exists(ALARMS_CONFIG_FILE))
    {
        LOG_INFO("Keine Wecker-Konfigurationsdatei gefunden. Standardwerte werden verwendet.");
        saveAlarms();
        return;
    }

    File configFile = LittleFS.open(ALARMS_CONFIG_FILE, "r");
    if (!configFile)
    {
        LOG_ERROR("Fehler beim Öffnen der Wecker-Konfigurationsdatei zum Lesen");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error)
    {
        LOG_PRINTF(Log::LOG_LEVEL_ERROR, "Fehler beim Parsen der Wecker-Konfigurationsdatei: %s", error.c_str());
        return;
    }

    JsonArray alarmsArray = doc.as<JsonArray>();
    int i = 0;
    for (JsonObject alarmObj : alarmsArray)
    {
        if (i < NUM_ALARMS)
        {
            m_alarms[i].enabled = alarmObj["enabled"];
            m_alarms[i].sunriseEnabled = alarmObj["sunriseEnabled"] | true; // Default auf true, falls Feld fehlt
            m_alarms[i].radioEnabled = alarmObj["radioEnabled"] | false;
            m_alarms[i].hour = alarmObj["hour"];
            m_alarms[i].minute = alarmObj["minute"];
            m_alarms[i].days = alarmObj["days"];
            m_alarms[i].triggeredToday = false; // Beim Laden immer zurücksetzen
            m_alarms[i].alarmTriggeredToday = false;
            i++;
        }
    }
    LOG_INFO("Wecker-Konfiguration erfolgreich geladen.");
}