#ifndef INC_CONFIG_H
#define INC_CONFIG_H

#include "include.h"

//#define MAX
//#define AMELIE
//#define TEICH
#define LICHTWECKER



/**
 * @brief mqtt server config
 * 
 */
//#define mqtt_server "nherrmann"
#define mqtt_server "haussteuerung"
//#define mqtt_user "your_username"
//#define mqtt_password "your_password"
const char *cpcSubScriberSetHomeProtokollServer = {"HomeProtokollServer/"};
const char *cpcSubScriberSet = {"/Set/+"};
const char *cpcSubScriberGet = {"/Get"};
const char *cpcSubScriberAlarmClock = {"alarm_clock_mqtt_max/+"};



/**
 * @brief pin config
 * 
 */
//const int iLdr = A0; 
#define LED_PIN   16     
#define TOUCH_PIN_1   T0  // IO 4     
#define TOUCH_PIN_2   T8 // IO 33    

/**
 * @brief Speicher verwaltung für die Configdaten
 * 
 */
class Config
{
   public:
      enum LoggingSource
      {
         Off = 0,
         Serial,
         MQTT
      };


      static Config * GetInstance();
      void Load();
      void Write();

      const char * GetWifiSsid() { return ptr_wifi_ssid;}
      const char * GetWifiSPassword() { return ptr_wifi_password;}
      const char * GetWifiHostname() { return ptr_wifi_hostname;}

      const char * GetVersionName() { return ptr_VersionName;}
      const char * GetVersionNumber() { return ptr_VersionNumber;}
      const String & GetBuildDate();
      const String & GetVersionString();

      const bool GetLoggingActive( ) {return m_tsConfig.loggingActive;}
      void  SetLoggingActive(bool set) {m_tsConfig.loggingActive = set; Write();}

   private:
      Config();
      ~Config();

      #define VERSIONNAME  "Lichtwecker"
      #define VERSIONNUMBER  "2.3"


      const char* ptr_VersionName = VERSIONNAME;
      const char* ptr_VersionNumber = VERSIONNUMBER;
      const char* ptrBuildDate = {__DATE__};
      const char* ptrBuildTime = {__TIME__};

      const char* ptr_wifi_ssid = {MeinWlanSsid};
      const char* ptr_wifi_password = {MeinWlanPW};

#ifdef MAX      
      const char* ptr_wifi_hostname = {"Lichtwecker_Max"};
#endif
#ifdef AMELIE      
      const char* ptr_wifi_hostname = {"Lichtwecker_Amelie"};
#endif
#ifdef TEICH      
      const char* ptr_wifi_hostname = {"Lichtwecker_Teich"};
#endif
#ifdef LICHTWECKER      
      const char* ptr_wifi_hostname = {"Lichtwecker"};
#endif

      typedef struct tsConfig
      {
         bool loggingActive;
      }Config_ts;

      Config_ts m_tsConfig;
      static Config * ptrInstance;
      static const uint32_t m_uiEEPROM_ADDR_CONFIG = 0;
      static const uint32_t m_uiEepromSize = 512;

};

Config * Config::ptrInstance = NULL;

/**
 * @brief Singleton Config
 * 
 * @return Config* 
 */
Config * Config::GetInstance()
{
   if( ptrInstance == NULL)
   {
      ptrInstance = new Config();
   }
   return ptrInstance;
}

/**
 * @brief Construct a new Config:: Config object
 * 
 */
Config::Config() 
{
   m_tsConfig.loggingActive = true;
   EEPROM.begin(m_uiEepromSize);
   Load();
}

/**
 * @brief Schreibt die Config Daten ins EEPROM
 * 
 * @param tsconfig 
 */
void Config::Write( )
{
   uint8_t * ptrData = (uint8_t *)&m_tsConfig;
   uint32_t uiConfigStartAddress = m_uiEEPROM_ADDR_CONFIG;

   for( uint32_t i = 0; i < sizeof(tsConfig); i++)
   {
      EEPROM.write( uiConfigStartAddress, *ptrData);
      ++ptrData;
      ++uiConfigStartAddress;
   }

   EEPROM.commit();
}


/**
 * @brief Laden der Config Parameter
 * 
 */
void Config::Load( )
{
   uint8_t * ptrData = (uint8_t *)&m_tsConfig;
   uint32_t uiConfigStartAddress = m_uiEEPROM_ADDR_CONFIG;
   
   for( uint32_t i = 0; i < sizeof(tsConfig); i++)
   {
      *ptrData = EEPROM.read( uiConfigStartAddress);
      ++ptrData;
      ++uiConfigStartAddress;
   }
}

/**
 * @brief Gibt den Versionsstring zurück
 * 
 * @return const char* 
 */
const String & Config::GetVersionString()
{
   static String str = String(ptr_VersionName) + " V" + String(ptr_VersionNumber);
   return str;
}

/**
 * @brief Gibt build Zeitpunkt zurück
 * 
 * @return const char* 
 */
const String & Config::GetBuildDate()
{
   static String str = String(ptrBuildDate) + " " + String(ptrBuildTime);
   return str;
}

#endif // INC_CONFIG_H
