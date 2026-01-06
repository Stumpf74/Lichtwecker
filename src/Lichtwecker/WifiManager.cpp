#include "WifiManager.h"
#include <WiFi.h>
#include "Config.h"
#include "Log.h"
#include "MqttClient.h"
#include "AppContext.h"

WifiManager::WifiManager() : m_wifiRetryCount(0)
{
}

/**
 * @brief Stellt die initiale WLAN-Verbindung her.
 * @return true, wenn die Verbindung erfolgreich war, sonst false.
 */
bool WifiManager::begin()
{
   LOG_INFO("Init WifiManager");
   m_wifiRetryCount = 0;

   delay(100);
   // We start by connecting to a WiFi network
   LOG_INFO("");
   LOG_PRINTF(Log::LOG_LEVEL_INFO, "Verbinde mit %s", Config::GetInstance()->GetWifiSsid());

   WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
   LOG_PRINTF(Log::LOG_LEVEL_INFO, "Setze Hostname: %s", Config::GetInstance()->GetWifiHostname());
   WiFi.setHostname(Config::GetInstance()->GetWifiHostname());
   WiFi.mode(WIFI_STA); // nur als client arbeiten
   WiFi.begin(Config::GetInstance()->GetWifiSsid(), Config::GetInstance()->GetWifiSPassword());

   uint32_t uiWaitTimeout = 20; // wenn wir nach 10s keine Verbindung haben passt was nicht
   bool bfIsConnected = true;

   while (WiFi.status() != WL_CONNECTED)
   {
      delay(500);
      Serial.print("."); // Direkte Ausgabe, da Logging hier zu viel wäre
      if (--uiWaitTimeout == 0)
      {
         LOG_ERROR("");
         LOG_ERROR("ESP ...... fehlende WLAN Verbindung. Es geht ohne WLAN weiter!");
         
         uint32_t status = WiFi.status();
         std::string statusStr;
         switch (status)
         {
         case WL_IDLE_STATUS:
            statusStr = "Leerlauf";
            break;
         case WL_NO_SSID_AVAIL:
            statusStr = "Keine SSID verfügbar";
            break;
         case WL_SCAN_COMPLETED:
            statusStr = "Scan abgeschlossen";
            break;
         case WL_CONNECTED:
            statusStr = "Verbunden";
            break;
         case WL_CONNECT_FAILED:
            statusStr = "Verbindung fehlgeschlagen";
            break;
         case WL_CONNECTION_LOST:
            statusStr = "Verbindung verloren";
            break;
         case WL_DISCONNECTED:
            statusStr = "Getrennt";
            break;
         default:
            statusStr = "Undefinierter Wert";
            break;
         }
         LOG_PRINTF(Log::LOG_LEVEL_ERROR, "WiFi-Status: %d --> %s", status, statusStr.c_str());
         bfIsConnected = false;
         break;
      }
   }

   if (bfIsConnected)
   {
      LOG_INFO("");
      LOG_INFO("WiFi verbunden");
      LOG_PRINTF(Log::LOG_LEVEL_INFO, "IP-Adresse: %s", WiFi.localIP().toString().c_str());
      int32_t rssi = WiFi.RSSI();
      appContext.cRssiAvg.InitBuffer(rssi);
   }
   return bfIsConnected;
}

/**
 * @brief Überprüft die WLAN-Verbindung und versucht bei Bedarf eine Wiederverbindung.
 * @return true, wenn eine Wiederverbindung gerade erfolgreich war, sonst false.
 */
bool WifiManager::handle()
{
   if (WiFi.status() != WL_CONNECTED)
   {
      m_wifiRetryCount++;
      LOG_ERROR("WiFi nicht verbunden. Versuche Wiederverbindung.");
      // Wir rufen hier die begin-Methode auf, da sie die Verbindungslogik enthält.
      if (this->begin())
      {
         LOG_INFO("WiFi wurde erfolgreich wiederverbunden!");
         m_wifiRetryCount = 0;
         return true; // Signalisiert eine erfolgreiche Wiederverbindung
      }
   }
   else
   {
      m_wifiRetryCount = 0;
   }

   if (m_wifiRetryCount >= MAX_WIFI_RETRY)
   {
      LOG_ERROR("Neustart wegen fehlender WLAN-Verbindung.");
      ESP.restart();
   }

   return false; // Keine Wiederverbindung in diesem Durchlauf
}