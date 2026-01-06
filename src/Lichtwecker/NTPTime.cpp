#include "NTPTime.h"
#include "Config.h"
#include "Log.h"
#include <esp_sntp.h> // Für sntp_get_sync_status()
#include "SunTimeManager.h"

void NTPTime::time_sync_notification_cb(struct timeval *tv)
{
   time_t now = time(nullptr);
   struct tm timeinfo;
   localtime_r(&now, &timeinfo);
   char timeStr[20];
   strftime(timeStr, sizeof(timeStr), "%d.%m.%Y %H:%M:%S", &timeinfo);
   LOG_PRINTF(Log::LOG_LEVEL_INFO, "NTP: Zeit wurde synchronisiert: %s", timeStr);
   SunTimeManager::GetInstance()->notifyNtpSync(); // Benachrichtige SunTimeManager
}

NTPTime::NTPTime() {}

NTPTime::~NTPTime()
{
   // Keine dynamisch allozierten Ressourcen mehr hier
}

void NTPTime::begin()
{
   LOG_INFO("Init NTP");
   // Setze die Zeitzone für automatische Sommer-/Winterzeitumstellung (Berlin)
   // CET-1CEST,M3.5.0/2,M10.5.0/3

   // Starte NTP-Synchronisierung. Offsets sind 0, da die Zeitzone über "TZ" gehandhabt wird.
   // 1. configTime() aufrufen, um den SNTP-Dienst zu starten.
   configTime(0, 0, Config::GetInstance()->GetNTPHostname());

   // 2. Die Zeitzonen-Variable setzen.
   setenv("TZ", Config::GetInstance()->GetTimezoneString(), 1);

   // 3. Die Zeitzonen-Einstellung anwenden.
   tzset();

   // Debug: Print the TZ environment variable to confirm it's set
   const char *tz_env = getenv("TZ");
   // if (tz_env) Log::PrintLN("Debug: TZ environment variable set to: " + String(tz_env));
   // else Log::PrintLN("Debug: TZ environment variable not found!");

   // Registriere den Callback, um über erfolgreiche Synchronisierungen informiert zu werden.
   sntp_set_time_sync_notification_cb(NTPTime::time_sync_notification_cb);
   // Optional: Setze das Synchronisationsintervall (Standard ist 1 Stunde)
   sntp_set_sync_interval(3600000); // 1 Stunde in Millisekunden

   LOG_INFO("Warte auf initiale NTP-Zeitsynchronisation..."); // print ohne LN hat keinen Wrapper
   struct tm timeinfo;
   // getLocalTime wartet, bis die Zeit synchronisiert ist (mit einem internen Timeout von ca. 15s).
   if (!getLocalTime(&timeinfo))
   {
      LOG_ERROR("Fehler beim Abrufen der Zeit");
      // Fallback: If getLocalTime fails, try to get time anyway, might be UTC or uninitialized
      LOG_PRINTF(Log::LOG_LEVEL_INFO, "Aktuelle Zeit (könnte UTC oder uninitialisiert sein): %s", getFormattedTime().c_str());
      return;
   }
   LOG_PRINTF(Log::LOG_LEVEL_INFO, " fertig. Initial synchronisierte Zeit: %s", getFormattedTime().c_str());

   // Small delay to ensure TZ settings are fully propagated before final log
   delay(100);
   LOG_PRINTF(Log::LOG_LEVEL_INFO, "NTP-Synchronisation erfolgreich. Finale lokale Zeit: %s", getFormattedTime().c_str());
}

time_t NTPTime::getEpochTime()
{
   return time(nullptr); // Standard-C-Funktion für Epoch-Zeit
}

String NTPTime::getFormattedTime()
{
   time_t now = time(nullptr);
   struct tm timeinfo;
   // Konvertiert die UTC-Epoch-Zeit in die lokale Zeit unter Berücksichtigung der TZ-Einstellung.
   // Dies ist die kritische Funktion, die die TZ-Variable nutzen sollte.
   localtime_r(&now, &timeinfo);
   char timeStr[20];
   // Erweitertes Format, um Zeitzonen-Informationen auszugeben
   strftime(timeStr, sizeof(timeStr), "%d.%m.%Y %H:%M:%S", &timeinfo); // Formatiert die Zeit
   return String(timeStr);
}

String NTPTime::getFormattedTimeShort()
{
   time_t now = time(nullptr);
   struct tm timeinfo;
   // Konvertiert die UTC-Epoch-Zeit in die lokale Zeit unter Berücksichtigung der TZ-Einstellung.
   // Dies ist die kritische Funktion, die die TZ-Variable nutzen sollte.
   localtime_r(&now, &timeinfo);
   char timeStr[20];
   // Erweitertes Format, um Zeitzonen-Informationen auszugeben
   strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo); // Formatiert die Zeit
   return String(timeStr);
}

String NTPTime::getFormattedDate()
{
   time_t now = time(nullptr);
   struct tm timeinfo;
   localtime_r(&now, &timeinfo);
   char dateStr[11]; // DD.MM.YYYY + Null-Terminator
   strftime(dateStr, sizeof(dateStr), "%d.%m.%Y", &timeinfo);
   return String(dateStr);
}
