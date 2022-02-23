
#include <assert.h>
#include <time.h>

// #include <ESP8266mDNS.h>
// #include <ESP8266WiFi.h>

#include <ESPmDNS.h>
#include <WiFi.h>


#include <NTPClient.h>
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
#include "Avarage.h"
#include "LichtweckerFunktions.h"
#include "TouchSensorFunktions.h"

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
cTouchSensor cTouchSensor1(TOUCH_PIN_1, 25);
cTouchSensor cTouchSensor2(TOUCH_PIN_2, 25);

//#define BMP280_I2C_ADDRESS  0x76
//Adafruit_BME280 bmp; // use I2C interface
//CAverage<float> cAvgTemperature(16, 0.0);
//CAverage<uint16_t> cAvgPressure(16, 0);
//Adafruit_Sensor *bmp_temp = bmp.getTemperatureSensor();
//Adafruit_Sensor *bmp_pressure = bmp.getPressureSensor();


/**
 * @brief 
 * 
 * @param msg 
 */
void SendLogDataToMQTT( const String & msg )
{
   Publish("Log", msg);

#ifdef TEICH
   SendLogDataToSerial(msg);
#endif
}


 /* 
 * @param msg 
 */
void SendLogDataToSerial( const String & msg )
{
   Log::Print(msg);
}


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
   String strMsg;
   for (int i = 0; i < payload_length; i++)
   {
      Serial.print((char)payload[i]);
      strMsg += (char)payload[i];
   }

   String strTopic = String(topic);

   Log::Print("Publish received - Topic: ");
   Log::Print(strTopic.c_str());
   Log::Print("\tMsg: ");
   Log::Print(strMsg.c_str());

   if (strTopic.indexOf("Set/ALARM") != std::string::npos)
   {
      cLigthAlarmClock::GetInstance()->ParseAlarms( strMsg );
   }
   else if (strTopic.indexOf("Set/RESET") != std::string::npos)
   {
      Log::Print("externer Reset wird ausgeführt in 1,5s");
      delay(1500);
      //ESP.restart();
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
      //Log::Print("RGB Msg erhalten" + strMsg);
      cLigthAlarmClock::GetInstance()->SetRgb(strMsg);
   }
   else if (strTopic.indexOf("Get") != std::string::npos)
   {
      if (strMsg.indexOf("STATUS") != std::string::npos)
      {
         bfSendStatus = true;
      }
   }
   else if (strTopic.indexOf("alarm_clock_mqtt_max") != std::string::npos)
   {
      if( strTopic.indexOf("before_alarm") != std::string::npos )
      {  
         cLigthAlarmClock::GetInstance()->StartLigthSequenz();
         Log::Print("Sunrise is started");
      }   
      else if( strTopic.indexOf("start") != std::string::npos )
      {  
         //cLigthAlarmClock::GetInstance()->StartLigthSequenz();
         Log::Print("start");
         if (strMsg.indexOf("SIMULATION") != std::string::npos)
         {
            cLigthAlarmClock::GetInstance()->StartSimulation();
            Log::ActivateLogging(true);
         }
      }   
      else if( strTopic.indexOf("stop") != std::string::npos )
      {  
         cLigthAlarmClock::GetInstance()->Stop();
         Log::Print("stop");
      }   
      else if( strTopic.indexOf("alarm") != std::string::npos )
      {  
         //cLigthAlarmClock::GetInstance()->StartLigthSequenz();
         Log::Print("alarm");
      }   
      else
      {
         Log::Print("MQTT command undefined: ");Log::Print(strTopic);
      }
   }
   
}

/**
 * @brief 
 * 
 * @param Topic 
 * @param Msg 
 */
void Publish(const char * Topic, const String & Msg)
{
   String newTopic = String(Config::GetInstance()->GetWifiHostname()) + "/" + String(Topic);
   client.publish (newTopic.c_str(), Msg.c_str());
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
      Publish("RSSI", strVal);
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
   Publish("STATUS", "online");
   Publish("VERSION", Config::GetInstance()->GetVersionString());
   Publish("BUILD", Config::GetInstance()->GetBuildDate());
   IPAddress ip = WiFi.localIP();
   Publish("LOCALIP", ip.toString());
   Publish("HOSTNAME", Config::GetInstance()->GetWifiHostname());
   Publish("SSID", Config::GetInstance()->GetWifiSsid());
   Publish("RSSI", strVal);
}

/**
 * @brief setup_wifi
 * 
 */
bool setup_wifi()
{
   delay(100);
   // We start by connecting to a WiFi network
   Log::Print();
   Log::Print("Connecting to ");
   Log::Print(Config::GetInstance()->GetWifiSsid());

   WiFi.mode(WIFI_STA); // nur als client arbeiten
   WiFi.hostname(Config::GetInstance()->GetWifiHostname());
   WiFi.begin(Config::GetInstance()->GetWifiSsid(), Config::GetInstance()->GetWifiSPassword());
   Log::Print("Set Hostname: ");
   Log::Print(Config::GetInstance()->GetWifiHostname());

   uint32_t uiWaitTimeout = 20; // wenn wir nach 10s keine Verbindung haben passt was nicht
   bool bfIsConnected = true;

   while (WiFi.status() != WL_CONNECTED)
   {
      delay(500);
      Log::Print(".");
      if (--uiWaitTimeout == 0)
      {
         Log::Print();
         Log::Print("ESP ...... fehlende WLAN Verbindung. Es geht ohne WLAN weiter!");
         Log::Print("WiFi.status(): ");
         uint32_t status = WiFi.status();
         Log::Print(status);
         Log::Print(" --> ");
         switch (status)
         {
         case WL_NO_SHIELD:
            Log::Print("No shield");
            break;
         case WL_IDLE_STATUS:
            Log::Print("Idle status");
            break;
         case WL_NO_SSID_AVAIL:
            Log::Print("No ssid avail");
            break;
         case WL_SCAN_COMPLETED:
            Log::Print("Scan completed");
            break;
         case WL_CONNECTED:
            Log::Print("connected");
            break;
         case WL_CONNECT_FAILED:
            Log::Print("connect faild");
            break;
         case WL_CONNECTION_LOST:
            Log::Print("Connection lost");
            break;
         case WL_DISCONNECTED:
            Log::Print("disconnected");
            break;
         default:
            Log::Print("undefined value");
            break;
         }
         bfIsConnected = false;
         break;
      }
   }

   if (bfIsConnected)
   {
      Log::Print("");
      Log::Print("WiFi connected");
      Log::Print("IP address: ");
      Log::Print(WiFi.localIP());
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
      Log::Print("Verbinde zu Broker - ");
      Log::Print(mqtt_server);

      client.setServer(mqtt_server, 1883);
      if (client.connect(Config::GetInstance()->GetWifiHostname(), String( String(Config::GetInstance()->GetWifiHostname()) + String("/STATUS")).c_str(), 0, true, "offline"))
      {
         // Set subscriber
         Log::Print("Setze Subscriber - ");
         String temp = String(Config::GetInstance()->GetWifiHostname()) + String(cpcSubScriberSet);
         Log::Print(temp);
         client.subscribe(temp.c_str());
         
         Log::Print("Setze Subscriber - ");
         temp = String(Config::GetInstance()->GetWifiHostname()) + String(cpcSubScriberGet);
         Log::Print(temp);
         client.subscribe(temp.c_str());

         Log::Print("Setze Subscriber - ");
         Log::Print(cpcSubScriberAlarmClock);
         client.subscribe(cpcSubScriberAlarmClock);

         Log::Print("Setze Subscriber - ");
         temp = String(cpcSubScriberSetHomeProtokollServer) + String(Config::GetInstance()->GetWifiHostname());
         Log::Print(temp);
         client.subscribe(temp.c_str());

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


String hexStr(unsigned char *data, int len)
{
   constexpr char hexmap[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                              '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

   
   // Log::Print("hexStr: ");
   // Log::Print(data[0]);
   // Log::Print(data[1]);
   // Log::Print(data[2]);

   String s(len * 2, ' ');
   for (int i = 0; i < len; ++i) 
   {
      s[2 * i]     = hexmap[(data[i] & 0xF0) >> 4];
      s[2 * i + 1] = hexmap[data[i] & 0x0F];
   }
   // Log::Print("Str: ");
   // Log::Print(s);
   return s;
}


void PublishRgbValue( CRGB rgbcolor )
{
   String strRgbHex = hexStr((unsigned char *)rgbcolor.raw, 3);
   Publish("rgb", strRgbHex);
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
   Log::RegisterSendfunction(SendLogDataToMQTT);
   Log::ActivateLogging(true);
   Log::Print ("Starte Lichtwecker");

   InitIo();

   //Log::RegisterSendfunction(SendLogDataToSerial);


   if (setup_wifi())
      if (setup_mqtt())
      {
         // hier könnte man was tun
      }  

   timeClientNTP.begin();
//   timeClientNTP.update();

   InitArduinoOTA();

   cLigthAlarmClock::GetInstance()->Setup();
   cLigthAlarmClock::GetInstance()->RegisterRgbChangedCalback(&PublishRgbValue);

   cTouchSensor1.Setup();
   cTouchSensor2.Setup();


   //bmp.begin(BMP280_I2C_ADDRESS);
   // while (!bmp.begin(0x76)) {
   //    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
   //    delay(500);
   // }

//   ReadBME280Data(true);
}

/**
 * @brief Init IO's
 * 
 */
void InitIo()
{
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
      Log::Print("MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time");
      break;
   case -3:
      Log::Print("MQTT_CONNECTION_LOST - the network connection was broken");
      break;
   case -2:
      Log::Print("MQTT_CONNECT_FAILED - the network connection failed");
      break;
   case -1:
      Log::Print("MQTT_DISCONNECTED - the client is disconnected cleanly");
      break;
   case 0:
      Log::Print("MQTT_CONNECTED - the client is connected");
      break;
   case 1:
      Log::Print("MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT");
      break;
      ;
   case 2:
      Log::Print("MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier");
      break;
   case 3:
      Log::Print("MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection");
      break;
   case 4:
      Log::Print("MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected");
      break;
   case 5:
      Log::Print("MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect");
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
   //Log::PrintF("Check MQTT Client is Connected --> %d\r\n", iConnectionStatus);

   //   if (iOldConnectionStatus != iConnectionStatus)
   if (iConnectionStatus != MQTT_CONNECT_BAD_PROTOCOL) // Das ist richtig da wir eine andere Version als der Mosquittp Broker ausgeben
   {
      Log::Print("no connection to MQTT broker...");
      WriteMqttDebugMsg(iConnectionStatus);
      Log::Print("Try MQTT reconnect...");

      ++imqtt_retry;

      // Attempt to connect
      if (setup_mqtt())
      {
         if (imqtt_retry != 0)
         {
            Log::Print("WiFi was successfully reconnected!");
         }

         imqtt_retry = 0;
      }

      if (imqtt_retry >= uiMaxRetry)
      {
         Log::Print("Reboot because no MQTT connection");
         //ESP.restart();
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
      Log::Print("WiFi not connected. Try to reconnect");
      bfIsWifiConnected = setup_wifi();

      if (bfIsWifiConnected)
         setup_mqtt();
   }
   else
   {
      if (wifi_retry != 0)
      {
         Log::Print("WiFi was successfully reconnected!");
  //       SetLedGruen();
      }

      wifi_retry = 0;
      bfIsWifiConnected = true;
   }

   if (wifi_retry >= uiMaxRetry)
   {
      Log::Print("Reboot because no WLAN connection");
      //ESP.restart();
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
      Log::Print("ESP cyclic restart");
      //ESP.restart();
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

   Publish("TIME", timestamp);
}


// void ReadBME280Data( bool bfInit )
// {
//    static float old_temp = 0;
//    static uint16_t old_press = 0;

//    float temp = bmp.readTemperature();
//    temp = round(temp*10) / 10;
   
//    float pressure = (bmp.readPressure()/100); // /100 for hPA
//    //float altitude = bmp.readAltitude(1013.25);
//    uint16_t pressure_round = static_cast<uint16_t>(pressure+0.5);

//    if (bfInit)
//    {
//       cAvgTemperature.InitBuffer(temp);
//       cAvgPressure.InitBuffer(pressure_round);
//    }
//    else
//    {
//       cAvgTemperature.Add(temp);   
//       cAvgPressure.Add(pressure_round);
//    }

//    if( old_temp != cAvgTemperature.Get())
//    {
//       old_temp = cAvgTemperature.Get();
//       Publish("Lichtwecker_Max/TEMPERATUR", String(old_temp).c_str());
//    }

//    if( old_press != cAvgPressure.Get())
//    {
//       old_press = cAvgPressure.Get();
//       Publish("Lichtwecker_Max/LUFTDRUCK", String(old_press).c_str());
//    }
// }
 
 
 

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
   static uint32_t next5Min = currentMillis + 5 * 60 * 1000; // alle 5 Min
   static uint32_t next12HMin = currentMillis + 720 * 60 * 1000; // alle 12 Stunden
   

   // 10ms Schleife
   if (currentMillis - previousMillis10ms >= 10)
   {
      previousMillis10ms = currentMillis;

      if( cTouchSensor1.IsShortPressed())
      {  
         Log::Print("DimColor");
         cLigthAlarmClock::GetInstance()->DimColor();
      }

      if( cTouchSensor1.IsLongPressed())
      {  
         Log::Print("SetBrigthness");
         cLigthAlarmClock::GetInstance()->SetBrigthness(50);
      }
      if( cTouchSensor2.IsShortPressed())
      {
         Log::Print("SetNextColor");
         cLigthAlarmClock::GetInstance()->SetNextColor();
      }  

      if( cTouchSensor2.IsLongPressed())
      {
         Log::Print("Stop");
         cLigthAlarmClock::GetInstance()->Stop();
      }  
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

      if (CheckWifiIsConnected())
      {
         CheckMqttClientIsConnected();
      }
   }

   // 5s Schleife
   if (currentMillis >= next5Second)
   {
      next5Second = currentMillis + 5000;
    //  ReadBME280Data(false);
   }

   // 5Min Schleife
   if (currentMillis >= next5Min)
   {
      next5Min = currentMillis + 5 * 60 * 1000; // alle 5 Min;
      GetRssi();

      bfSendStatus = true;
   }

   // 12hMin Schleife
   if (currentMillis >= next12HMin)
   {
      next12HMin = currentMillis + 720 * 60 * 1000; // alle 15 Min;
      timeClientNTP.update();
      bfSendStatus = true;
   }


   client.loop();
   ArduinoOTA.handle();
   timeClientNTP.update();
   cLigthAlarmClock::GetInstance()->Runtime(timeClientNTP.getEpochTime());
   cTouchSensor1.Runtime(currentMillis);
   cTouchSensor2.Runtime(currentMillis);

}
