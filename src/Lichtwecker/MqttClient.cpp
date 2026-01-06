#include "MqttClient.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include "AppContext.h"
#include "Config.h"
#include "Log.h"
#include "AlarmManager.h"
#include "LightController.h"

MqttClient::MqttClient() : m_client(m_espClient),
                           m_nextReconnectAttempt(0)
{
}

/**
 * @brief Callback from mqtt
 * Muss statisch sein, da C-Style-Callbacks keine this-Zeiger akzeptieren.
 */
void MqttClient::callback(char* topic, byte* payload, unsigned int length)
{
   // Allocate the correct amount of memory for the payload copy
   byte *p = (byte *)malloc(length + 1);

   // Copy the payload to the new buffer
   memcpy(p, payload, length);
   p[length] = 0;

   String strMsg = String((char *)p);
   String strTopic = String(topic);

   if (strTopic.indexOf("Set/RESET") != -1)
   {
      LOG_INFO("externer Reset wird ausgeführt in 1,5s");
      delay(1500);
      ESP.restart();
   }
   else if (strTopic.indexOf("Set/LOGLEVEL") != -1)
   {
      if (strMsg.equalsIgnoreCase("NONE")) {
         Log::GetInstance()->setLogLevel(Log::LOG_LEVEL_NONE); // Setup-Call bleibt
      } else if (strMsg.equalsIgnoreCase("ERROR")) {
         Log::GetInstance()->setLogLevel(Log::LOG_LEVEL_ERROR); // Setup-Call bleibt
      } else if (strMsg.equalsIgnoreCase("INFO")) {
         Log::GetInstance()->setLogLevel(Log::LOG_LEVEL_INFO); // Setup-Call bleibt
      } else if (strMsg.equalsIgnoreCase("DEBUG")) {
         Log::GetInstance()->setLogLevel(Log::LOG_LEVEL_DEBUG); // Setup-Call bleibt
      }
      LOG_PRINTF(Log::LOG_LEVEL_INFO, "Loglevel auf '%s' gesetzt.", strMsg.c_str());
   }
   else if (strTopic.indexOf("Get") != -1)
   {
      if (strMsg.indexOf("STATUS") != -1)
      {
         appContext.bfSendStatus = true;
      }
   }
   else if (strTopic.indexOf("Set/ALARM") != -1)
   {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, (char *)p);

      if (error) 
      {
         LOG_PRINTF(Log::LOG_LEVEL_ERROR, "deserializeJson() failed: %s", error.c_str());
         free(p);
         return;
      }

      if (!doc["index"].isNull())
      {
         int index = doc["index"]; // 0-3
         bool enabled = doc["enabled"];
         bool sunriseEnabled = doc["sunriseEnabled"] | true; // Default auf true, falls Feld fehlt
         uint8_t hour = doc["hour"];
         uint8_t minute = doc["minute"];
         uint8_t days = doc["days"]; // Bitmask

         AlarmManager::GetInstance()->setAlarm(index, enabled, sunriseEnabled, hour, minute, days);
      }
   }
   else if (strTopic.indexOf("Set/RGB") != -1)
   {
      // Konvertiere Hex-String zu CRGB
      // strtol ignoriert das '#' am Anfang, falls vorhanden
      long number = strtol(strMsg.c_str(), nullptr, 16);
      CRGB color = CRGB(number);
      // Helligkeit ist hier nicht Teil der Nachricht, Standardwert verwenden
      LightController::GetInstance()->setColor(color, 128);
   }
   else if (strTopic.indexOf("Set/LIGHT") != -1 && strMsg.equalsIgnoreCase("OFF"))
   {
      LightController::GetInstance()->stop();
   }

   // Free the memory
   free(p);
}

/**
 * @brief
 *
 * @param msg
 */
void MqttClient::sendLog(const String &msg)
{
   // Sende nur, wenn der MQTT-Client verbunden ist.
   if (m_client.connected())
   {
      publish("Log", msg); // publish ist eine Instanzmethode, das funktioniert hier nicht direkt.
   }
}

/**
 * @brief setup mqtt server
 */
void MqttClient::begin()
{
   LOG_INFO("Init MqttClient");
   m_client.setServer(Config::GetInstance()->GetMQTTHostname(), 1883);
   m_client.setCallback(MqttClient::callback);
}

void MqttClient::handle()
{
   // Wenn nicht verbunden, versuche periodisch einen Reconnect.
   if (!m_client.connected() && millis() > m_nextReconnectAttempt)
   {
      reconnect();
   }
   // m_client.loop() muss immer aufgerufen werden, um eingehende Nachrichten
   // und die Keep-Alive-Pakete zu verarbeiten.
   m_client.loop();
}

void MqttClient::reconnect()
{
   // Setze den nächsten Versuch in 5 Sekunden, egal ob dieser Versuch klappt oder nicht.
   // Das verhindert, dass diese Funktion in einer schnellen Schleife aufgerufen wird.
   m_nextReconnectAttempt = millis() + 5000;

   LOG_INFO("Versuche MQTT-Verbindung...");
   String lwtTopic = String(Config::GetInstance()->GetMqttBaseTopic()) + "/STATUS";
   if (m_client.connect(Config::GetInstance()->GetMqttBaseTopic(), lwtTopic.c_str(), 0, true, "offline"))
   {
      LOG_INFO(" verbunden.");
      // Publish initial status
      publish("STATUS", "online", true);

      // Resubscribe
      m_client.subscribe(Config::GetInstance()->GetSubscriberToSet());
      LOG_PRINTF(Log::LOG_LEVEL_INFO, "Subscribed to: %s", Config::GetInstance()->GetSubscriberToSet());

      m_client.subscribe(Config::GetInstance()->GetSubscriberToGet());
      LOG_PRINTF(Log::LOG_LEVEL_INFO, "Subscribed to: %s", Config::GetInstance()->GetSubscriberToGet());
   }
   else
   {
      LOG_PRINTF(
          Log::LOG_LEVEL_ERROR, 
          "Verbindung fehlgeschlagen, rc=%d. Nächster Versuch in 5 Sekunden.", 
          m_client.state()
      );
   }
}

void MqttClient::publish(const String& subTopic, const String& message, bool retained)
{
   if (m_client.connected())
   {
      String topic = String(Config::GetInstance()->GetMqttBaseTopic()) + "/" + subTopic;
      m_client.publish(topic.c_str(), message.c_str(), retained);
   }
}