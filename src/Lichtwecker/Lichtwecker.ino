#include <assert.h>
#include <time.h>
//#include <TimeLib.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <string.h>
#include <PubSubClient.h>
#include <EEPROM.h>

#include "Debug.h"
#include "AlarmTime.h"
#include "Config.h"
#include "Avarage.h"
#include "Log.h"
#include "LichtweckerFunktions.h"

// Callback function header
//void callbackMqtt(char* topic, byte* payload, unsigned int payload_length);

void callbackMqtt(char *topic, byte *payload, unsigned int payload_length);

WiFiClient espClient;
PubSubClient client(mqtt_server, 1883, callbackMqtt, espClient);
CAverage<int32_t> cRssiAvg(16, -65);
bool bfSendStatus = false;
WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 3600 * 2; // UTC+2
NTPClient timeClientNTP(ntpUDP, "fritz.box", utcOffsetInSeconds, 3600);
static const char daysOfTheWeek[7][5] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};

/**
 * @brief Callback from mqtt
 * 
 * @param topic 
 * @param payload 
 * @param int 
 */
void callbackMqtt(char *topic, byte *payload, unsigned int payload_length)
{
   // Allocate the correct amount of memory for the payload copy
   byte *p = (byte *)malloc(payload_length);

   // Copy the payload to the new buffer
   memcpy(p, payload, payload_length);
   p[payload_length] = 0;

   String strMsg = String((char *)p);
   String strTopic = String(topic);

   DPRINT("Publish received - Topic: ");
   DPRINT(strTopic.c_str());
   DPRINT("\tMsg: ");
   DPRINTLN(strMsg.c_str());

   if (strstr((char *)p, "Set/ALARM"))
   {
      cLigthAlarmClock::GetInstance()->ParseAlarms( strMsg );
   }
   else if (strTopic.indexOf("Set/RESET") != std::string::npos)
   {
      DPRINTLN("externer Reset wird ausgeführt in 1,5s");
      delay(1500);
      ESP.restart();
   }
   else if (strTopic.indexOf("Set/LOGGING") != std::string::npos)
   {
      if (strMsg.indexOf("EIN") != std::string::npos)
      {
         Config::GetInstance()->SetLoggingActive(true);
         Log::ActivateLogging(true);
      }
      else if (strMsg.indexOf("AUS") != std::string::npos)
      {
         Config::GetInstance()->SetLoggingActive(false);
         Log::ActivateLogging(false);
      }
   }
   else if (strTopic.indexOf("Set/RGB") != std::string::npos)
   {
      //DPRINTLN("RGB Msg erhalten" + strMsg);
      cLigthAlarmClock::GetInstance()->SetRgb(strMsg);
   }
   else if (strTopic.indexOf("Get") != std::string::npos)
   {
      if (strMsg.indexOf("STATUS") != std::string::npos)
      {
         bfSendStatus = true;
      }
   }
   else if (strTopic.indexOf("alarm_clock_mqtt") != std::string::npos)
   {
      if( strTopic.indexOf("before_alarm") != std::string::npos )
      {  
         cLigthAlarmClock::GetInstance()->StartLigthSequenz();
         DPRINTLN("Sunrise is started");
      }   
      else if( strTopic.indexOf("start") != std::string::npos )
      {  
         //cLigthAlarmClock::GetInstance()->StartLigthSequenz();
         DPRINTLN("start");
      }   
      else if( strTopic.indexOf("stop") != std::string::npos )
      {  
         cLigthAlarmClock::GetInstance()->Stop();
         DPRINTLN("stop");
      }   
      else if( strTopic.indexOf("alarm") != std::string::npos )
      {  
         //cLigthAlarmClock::GetInstance()->StartLigthSequenz();
         DPRINTLN("alarm");
      }   
      else
      {
         DPRINT("MQTT command undefined: ");DPRINTLN(strTopic);
      }
   }
   
   
   // Free the memory
   free(p);
}

/**
 * @brief Get the Rssi object
 * 
 */
void GetRssi()
{
   int32_t rssi = WiFi.RSSI();
   static int32_t rssi_old = 100;

   cRssiAvg.Add(rssi);
   rssi = cRssiAvg.Get();

   if (rssi_old != rssi)
   {
      rssi_old = rssi;
      String strVal = String(rssi);
      client.publish("Lichtwecker/RSSI", strVal.c_str());
   }
}

/**
 * @brief Get the Wifi Status object
 * 
 */
void GetWifiStatus()
{
   int32_t rssi = cRssiAvg.Get();
   String strVal = String(rssi);

   client.publish("Lichtwecker/STATUS", "online");
   client.publish("Lichtwecker/VERSION", Config::GetInstance()->GetVersionString());
   client.publish("Lichtwecker/BUILD", Config::GetInstance()->GetBuildDate());

   IPAddress ip = WiFi.localIP();
   client.publish("Lichtwecker/LOCALIP", ip.toString().c_str());
   client.publish("Lichtwecker/HOSTNAME", Config::GetInstance()->GetWifiHostname());
   client.publish("Lichtwecker/SSID", Config::GetInstance()->GetWifiSsid());
   client.publish("Lichtwecker/RSSI", strVal.c_str());
}

/**
 * @brief setup_wifi
 * 
 */
bool setup_wifi()
{
   delay(100);
   // We start by connecting to a WiFi network
   DPRINTLN();
   DPRINT("Connecting to ");
   DPRINTLN(Config::GetInstance()->GetWifiSsid());

   WiFi.mode(WIFI_STA); // nur als client arbeiten
   WiFi.hostname(Config::GetInstance()->GetWifiHostname());
   WiFi.begin(Config::GetInstance()->GetWifiSsid(), Config::GetInstance()->GetWifiSPassword());
   DPRINT("Set Hostname: ");
   DPRINTLN(Config::GetInstance()->GetWifiHostname());

   uint32_t uiWaitTimeout = 20; // wenn wir nach 10s keine Verbindung haben passt was nicht
   bool bfIsConnected = true;

   while (WiFi.status() != WL_CONNECTED)
   {
      delay(500);
      DPRINT(".");
      if (--uiWaitTimeout == 0)
      {
         DPRINTLN();
         DPRINTLN("ESP ...... fehlende WLAN Verbindung. Es geht ohne WLAN weiter!");
         DPRINT("WiFi.status(): ");
         uint32_t status = WiFi.status();
         DPRINT(status);
         DPRINT(" --> ");
         switch (status)
         {
         case WL_NO_SHIELD:
            DPRINTLN("No shield");
            break;
         case WL_IDLE_STATUS:
            DPRINTLN("Idle status");
            break;
         case WL_NO_SSID_AVAIL:
            DPRINTLN("No ssid avail");
            break;
         case WL_SCAN_COMPLETED:
            DPRINTLN("Scan completed");
            break;
         case WL_CONNECTED:
            DPRINTLN("connected");
            break;
         case WL_CONNECT_FAILED:
            DPRINTLN("connect faild");
            break;
         case WL_CONNECTION_LOST:
            DPRINTLN("Connection lost");
            break;
         case WL_DISCONNECTED:
            DPRINTLN("disconnected");
            break;
         default:
            DPRINTLN("undefined value");
            break;
         }
         bfIsConnected = false;
         break;
      }
   }

   if (bfIsConnected)
   {
      DPRINTLN("");
      DPRINTLN("WiFi connected");
      DPRINT("IP address: ");
      DPRINTLN(WiFi.localIP());
   }
   return bfIsConnected;
}

/**
 * @brief setup mqtt server
 * 
 */
bool setup_mqtt()
{
   bool bfIsMqttConnected = false;

   if (client.connected() == false)
   {
      DPRINT("Verbinde zu Broker - ");
      DPRINTLN(mqtt_server);

      client.setServer(mqtt_server, 1883);
      if (client.connect("Lichtwecker Modul", "Lichtwecker/STATUS", 0, true, "offline"))
      {
         // Set subscriber
         DPRINT("Setze Subscriber - ");
         DPRINTLN(cpcSubScriberSet);
         client.subscribe(cpcSubScriberSet);
         DPRINT("Setze Subscriber - ");
         DPRINTLN(cpcSubScriberGet);
         client.subscribe(cpcSubScriberGet);

         DPRINT("Setze Subscriber - ");
         DPRINTLN(cpcSubScriberAlarmClock);
         client.subscribe(cpcSubScriberAlarmClock);

         DPRINT("Setze Subscriber - ");
         DPRINTLN(cpcSubScriberSetHomeProtokollServer);
         client.subscribe(cpcSubScriberSetHomeProtokollServer);

         GetWifiStatus();
         bfIsMqttConnected = true;
      }
   }
   return bfIsMqttConnected;
}

/**
 * @brief 
 * 
 */
void InitArduinoOTA()
{
   // Port defaults to 8266
   // ArduinoOTA.setPort(8266);

   // Hostname defaults to esp8266-[ChipID]
   ArduinoOTA.setHostname(Config::GetInstance()->GetWifiHostname());

   // No authentication by default
   // ArduinoOTA.setPassword((const char *)"123");

   ArduinoOTA.onStart([]() 
   {
      Serial.println("Start");
      cLigthAlarmClock::GetInstance()->Stop();
   });
   
   ArduinoOTA.onEnd([]() 
   {
      Serial.println("\nEnd");
   });
   
   ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) 
   {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
   });

   ArduinoOTA.onError([](ota_error_t error) 
   {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR)
         Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR)
         Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR)
         Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR)
         Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR)
         Serial.println("End Failed");
   });
   ArduinoOTA.begin();
}


/**
 * @brief setup
 * 
 */
void setup()
{
   delay(500);
   Serial.begin(115200);
   Config::GetInstance();
   DPRINTLN("Starte Lichtwecker");

   InitIo();

   if (setup_wifi())
      if (setup_mqtt())
      {
         // hier könnte man was tun
      }  

   timeClientNTP.begin();
//   timeClientNTP.update();

   InitArduinoOTA();

   cLigthAlarmClock::GetInstance()->Setup();
}

/**
 * @brief Init IO's
 * 
 */
void InitIo()
{
  pinMode(LED_BUILTIN, OUTPUT); // LED als Output definieren
  //digitalWrite(LED_BUILTIN, HIGH); // Ausschalten   
  digitalWrite(LED_BUILTIN, LOW); // Ausschalten   
}

/**
 * @brief Sendet den Status der MQTT Verbindung in Klartext
 * 
 * @param iConnectionStatus 
 */
void WriteMqttDebugMsg(int32_t iConnectionStatus)
{

   switch (iConnectionStatus)
   {
   case -4:
      DPRINTLN("MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time");
      break;
   case -3:
      DPRINTLN("MQTT_CONNECTION_LOST - the network connection was broken");
      break;
   case -2:
      DPRINTLN("MQTT_CONNECT_FAILED - the network connection failed");
      break;
   case -1:
      DPRINTLN("MQTT_DISCONNECTED - the client is disconnected cleanly");
      break;
   case 0:
      DPRINTLN("MQTT_CONNECTED - the client is connected");
      break;
   case 1:
      DPRINTLN("MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT");
      break;
      ;
   case 2:
      DPRINTLN("MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier");
      break;
   case 3:
      DPRINTLN("MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection");
      break;
   case 4:
      DPRINTLN("MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected");
      break;
   case 5:
      DPRINTLN("MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect");
      break;
   }
}

/**
 * @brief Check mqtt client is connected
 * 
 */
void CheckMqttClientIsConnected()
{
   static int iConnectionStatus = 0;
   static int imqtt_retry = 0;
   static const uint32_t uiMaxRetry = 15;

   iConnectionStatus = client.connected();
   //DPRINTF("Check MQTT Client is Connected --> %d\r\n", iConnectionStatus);

   //   if (iOldConnectionStatus != iConnectionStatus)
   if (iConnectionStatus != MQTT_CONNECT_BAD_PROTOCOL) // Das ist richtig da wir eine andere Version als der Mosquittp Broker ausgeben
   {
      DPRINTLN("no connection to MQTT broker...");
      WriteMqttDebugMsg(iConnectionStatus);
      DPRINTLN("Try MQTT reconnect...");

      ++imqtt_retry;

      // Attempt to connect
      if (setup_mqtt())
      {
         if (imqtt_retry != 0)
         {
            DPRINTLN("WiFi was successfully reconnected!");
         }

         imqtt_retry = 0;
      }

      if (imqtt_retry >= uiMaxRetry)
      {
         DPRINTLN("Reboot because no MQTT connection");
         ESP.restart();
      }
   }
}

/**
 * @brief Überprüfung ob Wifi
 * 
 */
bool CheckWifiIsConnected()
{
   static int wifi_retry = 0;
   static const uint32_t uiMaxRetry = 15;
   bool bfIsWifiConnected = false;

   if (WiFi.status() != WL_CONNECTED && wifi_retry < uiMaxRetry)
   {
      wifi_retry++;
//      SetLedRot();
      WiFi.disconnect();
      DPRINTLN("WiFi not connected. Try to reconnect");
      bfIsWifiConnected = setup_wifi();

      if (bfIsWifiConnected)
         setup_mqtt();
   }
   else
   {
      if (wifi_retry != 0)
      {
         DPRINTLN("WiFi was successfully reconnected!");
  //       SetLedGruen();
      }

      wifi_retry = 0;
      bfIsWifiConnected = true;
   }

   if (wifi_retry >= uiMaxRetry)
   {
      DPRINTLN("Reboot because no WLAN connection");
      ESP.restart();
   }

   return bfIsWifiConnected;
}


/**
 * @brief CyclicRestart 
 * 
 */
void CyclicRestart()
{
   static const long RestartTime = 60 * 60 * 24; // ein Tag
   static long Counter = RestartTime;

   if (--Counter <= 0)
   {
      Counter = RestartTime;
      Serial.println("ESP cyclic restart");
      ESP.restart();
   }
}

/**
 * @brief 
 * 
 */
void SendTimeStatus()
{
   String timestamp(daysOfTheWeek[timeClientNTP.getDay()]);
   timestamp += " " + timeClientNTP.getFormattedTime();

   client.publish("Lichtwecker/TIME", timestamp.c_str());
}

/**
 * @brief main loop
 * 
 */
void loop()
{
   uint32_t currentMillis = millis();
   static uint32_t previousMillis10ms = 0;
   static uint32_t previousMillis1000ms = 0;
   static uint32_t next5Second = currentMillis + 2500;
   static uint32_t next15Min = currentMillis + 15 * 60 * 1000; // alle 15 Min

   // 10ms Schleife
   if (currentMillis - previousMillis10ms >= 10)
   {
      previousMillis10ms = currentMillis;

      // Ab hier user code!
   }

   // 1000ms Schleife
   if (currentMillis - previousMillis1000ms >= 1000)
   {
      previousMillis1000ms = currentMillis;

      if (bfSendStatus)
      {
         bfSendStatus = false;
         GetWifiStatus();
         SendTimeStatus();
      }
   }

   // 5s Schleife
   if (currentMillis >= next5Second)
   {
      next5Second = currentMillis + 5000;

      if (CheckWifiIsConnected())
      {
         CheckMqttClientIsConnected();
      }

      GetRssi();
   }

   // 15Min Schleife
   if (currentMillis >= next15Min)
   {
      next15Min = currentMillis + 15 * 60 * 1000; // alle 15 Min;
      bfSendStatus = true;
      timeClientNTP.update();
   }

   client.loop();
   ArduinoOTA.handle();
   timeClientNTP.update();
   cLigthAlarmClock::GetInstance()->Runtime(timeClientNTP.getEpochTime());
}
