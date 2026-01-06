#include "OtaManager.h"
#include <ArduinoOTA.h>
#include "Log.h"
#include "Config.h"

/**
 * @brief Konstruktor für den OtaManager.
 */
OtaManager::OtaManager() : m_isOtaActive(false)
{
}

/**
 * @brief Initialisiert die Over-The-Air Update Funktionalität.
 *
 */
void OtaManager::begin()
{
   LOG_INFO("Init OTA");

   // Hostname für OTA
   ArduinoOTA.setHostname(Config::GetInstance()->GetWifiHostname());

   // Callbacks für den OTA-Prozess.
   // Wichtig: Lambdas, die auf Member-Variablen zugreifen, benötigen [this].
   ArduinoOTA.onStart([this]()
                      {
                         m_isOtaActive = true;
                         LOG_INFO("OTA Update gestartet...");
                      });

   ArduinoOTA.onEnd([]()
                    {
      LOG_INFO("\nOTA Update beendet. Starte neu...");
      delay(1000);
      ESP.restart(); });

   ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Fortschritt: %u%%\r", (progress / (total / 100)));
   });

   ArduinoOTA.onError([](ota_error_t error)
                      {
      LOG_PRINTF(Log::LOG_LEVEL_ERROR, "OTA Fehler[%u]: ", error);
      if (error == OTA_AUTH_ERROR) LOG_ERROR("Authentifizierung fehlgeschlagen");
      else if (error == OTA_BEGIN_ERROR) LOG_ERROR("Start fehlgeschlagen");
      else if (error == OTA_CONNECT_ERROR) LOG_ERROR("Verbindung fehlgeschlagen");
      else if (error == OTA_RECEIVE_ERROR) LOG_ERROR("Empfang fehlgeschlagen");
      else if (error == OTA_END_ERROR) LOG_ERROR("Ende fehlgeschlagen"); });

   ArduinoOTA.begin();
   LOG_INFO(" -> fertig");
}

/**
 * @brief Verarbeitet eingehende OTA-Anfragen. Muss in der Hauptschleife aufgerufen werden.
 */
void OtaManager::handle()
{
    ArduinoOTA.handle();
}

/**
 * @brief Prüft, ob gerade ein OTA-Update aktiv ist.
 * @return true, wenn ein Update läuft, sonst false.
 */
bool OtaManager::isOtaActive() const
{
    return m_isOtaActive;
}