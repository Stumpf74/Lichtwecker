#ifndef INC_CONFIG_H
#define INC_CONFIG_H
#include <Arduino.h>
#include <vector>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "Log.h"
#include "Miscellaneous.h"
#include "Secrets.h"

// Template:
// #define mysecret_ssid "MyWifiSSID"
// #define mysecret_password "ICHBINSOGEHEIM"
// #define mysecret_mqtt_server "192.168.1.1"
// #define mysecret_NTP_server "192.168.1.1"   


/**
 * @brief Speicher verwaltung für die Configdaten
 * 
 */


class Config
{
   private:                        
      Config() = default;

      void loadConfig();
      void saveConfig();
      void setDefaultRadioStations();
   
   public:
      static Config* GetInstance()
      {
         static Config instance;
         return &instance;
      }

      ~Config() = default;
      void begin();

      struct RadioStation {
         String name;
         String url;
         int volume;
      };


      enum LoggingSource
      {
         Off = 0,
         Serial,
         MQTT
      };

      const char * GetWifiSsid() { return ptr_wifi_ssid;}
      const char * GetWifiSPassword() { return ptr_wifi_password;}
      const char * GetWifiHostname() { return ptr_wifi_hostname;}

      const char * GetMQTTHostname() { return ptr_mqtt_server;}
      const char * GetMqttBaseTopic() { return ptr_mqtt_base_topic; }
      const char * GetNTPHostname() { return ptr_NTP_server;}
      const char * GetTimezoneString() { return ptr_timezone_string; }

      // NEU: Geografische Koordinaten für Sonnenstands-Berechnung
      double GetLatitude() { return m_tsConfig.latitude; }
      void SetLatitude(double latitude) { m_tsConfig.latitude = latitude; saveConfig(); }

      double GetLongitude() { return m_tsConfig.longitude; }
      void SetLongitude(double longitude) { m_tsConfig.longitude = longitude; saveConfig(); }

      const uint32_t GetMaxCounteValueUnchanged() { return m_tsConfig.maxCountValueUnchanged; }
      void SetMaxCounteValueUnchanged(uint32_t value) { m_tsConfig.maxCountValueUnchanged = value; saveConfig(); }

      const char * GetVersionName() { return ptr_VersionName;}
      
      const char * GetVersionNumber() { return ptr_VersionNumber;}
      const char * GetBuildDate();
      const char * GetBuildDateAsCharPtr();
      const char * GetVersionString();
      const char * GetVersionStringAsCharPtr();

      const uint32_t GetVersionNumberConfig() { return uiVersionConfig;}

      const bool GetLoggingActive( ) {return m_tsConfig.loggingActive;}
      void SetLoggingActive(bool set) 
      {
         m_tsConfig.loggingActive = set; 
         saveConfig();
      }

      const char * GetSubscriberToSet() { return m_subscriberSet.c_str(); }
      const char * GetSubscriberToGet() { return m_subscriberGet.c_str(); }

      const std::vector<RadioStation>& GetRadioStations() const { return m_tsConfig.radioStations; }
      void SetRadioStations(const std::vector<RadioStation>& stations) { m_tsConfig.radioStations = stations; saveConfig(); }

   private:
      const uint32_t uiVersionConfig{2};

      // Zentrale Definition des Namens
#ifdef DEBUG
      const char* ptr_base_name = "Lichtwecker_DEBUG";
#else
      const char* ptr_base_name = "Lichtwecker";
#endif


      const char* ptr_VersionName = ptr_base_name;
      const char* ptr_wifi_hostname = ptr_base_name;
      const char* ptr_mqtt_base_topic = ptr_base_name;

      // Member-Variablen für die dynamisch erstellten Subscriber-Strings
      // Direkt initialisieren statt im Konstruktor
      String m_subscriberSet = String(ptr_base_name) + "/Set/+";
      String m_subscriberGet = String(ptr_base_name) + "/Get";

      /**
       * @brief Version number 
       * V0.90 erste gute Version die einen Namen verdient ;-)
       * 
       * 
       * 
       */
      
      const char* ptr_VersionNumber{"0.90"};
      const char* ptrBuildDate = {__DATE__};
      const char* ptrBuildTime = {__TIME__};

      const char* ptr_wifi_ssid = mysecret_ssid;
      const char* ptr_wifi_password = mysecret_password;

      const char* ptr_mqtt_server = mysecret_mqtt_server;
      const char* ptr_NTP_server = mysecret_NTP_server;   
      const char* ptr_timezone_string = { "CET-1CEST,M3.5.0/2,M10.5.0/3" };


      typedef struct tsConfig
      {
         bool loggingActive;
         double latitude;
         double longitude;
         uint32_t maxCountValueUnchanged;
         std::vector<RadioStation> radioStations;
      }Config_ts;

      Config_ts m_tsConfig = { 
         .loggingActive = true,
         .latitude = 49.0360,
         .longitude = 12.1117,
         .maxCountValueUnchanged = 450
      };
      String m_versionstring;
      String m_strbuilddate;


};


#endif // INC_CONFIG_H
