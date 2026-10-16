#include "WifiManager.h"
#include <WiFi.h>
#include "Config.h"
#include "Log.h"
#include "MqttClient.h"
#include "AppContext.h"

WifiManager::WifiManager() : m_connectionStartedAt(0),
                             m_nextRetryAt(0),
                             m_isConnecting(false),
                             m_wasConnected(false)
{
}

/**
 * @brief Stellt die initiale WLAN-Verbindung her.
 * @return true, wenn die Verbindung erfolgreich war, sonst false.
 */
bool WifiManager::begin()
{
   LOG_INFO("Init WifiManager");
   LOG_PRINTF(Log::LOG_LEVEL_INFO, "Verbinde mit %s", Config::GetInstance()->GetWifiSsid());

   WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
   LOG_PRINTF(Log::LOG_LEVEL_INFO, "Setze Hostname: %s", Config::GetInstance()->GetWifiHostname());
   WiFi.setHostname(Config::GetInstance()->GetWifiHostname());
   WiFi.mode(WIFI_STA); // nur als client arbeiten
   WiFi.setAutoReconnect(true);
   WiFi.begin(Config::GetInstance()->GetWifiSsid(), Config::GetInstance()->GetWifiSPassword());

   m_connectionStartedAt = millis();
   m_isConnecting = WiFi.status() != WL_CONNECTED;
   m_wasConnected = !m_isConnecting;
   if (!m_isConnecting)
   {
      LOG_INFO("WiFi verbunden");
      LOG_PRINTF(Log::LOG_LEVEL_INFO, "IP-Adresse: %s", WiFi.localIP().toString().c_str());
      int32_t rssi = WiFi.RSSI();
      appContext.cRssiAvg.InitBuffer(rssi);
   }
   return !m_isConnecting;
}

/**
 * @brief Überprüft die WLAN-Verbindung und versucht bei Bedarf eine Wiederverbindung.
 * @return true, wenn eine Wiederverbindung gerade erfolgreich war, sonst false.
 */
bool WifiManager::handle()
{
   if (WiFi.status() == WL_CONNECTED)
   {
      m_isConnecting = false;
      if (!m_wasConnected)
      {
         m_wasConnected = true;
         LOG_INFO("WiFi verbunden");
         LOG_PRINTF(Log::LOG_LEVEL_INFO, "IP-Adresse: %s", WiFi.localIP().toString().c_str());
         appContext.cRssiAvg.InitBuffer(WiFi.RSSI());
         return true;
      }
      return false;
   }

   m_wasConnected = false;
   if (m_isConnecting)
   {
      if (millis() - m_connectionStartedAt < 10000)
      {
         return false;
      }

      LOG_PRINTF(Log::LOG_LEVEL_ERROR, "WLAN-Verbindung fehlgeschlagen, Status=%d. Neuer Versuch folgt.", WiFi.status());
      WiFi.disconnect(false, false);
      m_isConnecting = false;
      m_nextRetryAt = millis() + 5000;
      return false;
   }

   if (static_cast<int32_t>(millis() - m_nextRetryAt) >= 0)
   {
      LOG_INFO("WLAN getrennt. Starte nicht-blockierenden Wiederverbindungsversuch.");
      WiFi.begin(Config::GetInstance()->GetWifiSsid(), Config::GetInstance()->GetWifiSPassword());
      m_connectionStartedAt = millis();
      m_isConnecting = true;
   }

   return false;
}