#include "WebServerRoutes.h"
#include <LittleFS.h>
#include "Log.h"
#include "AppContext.h"
#include "LightController.h" // Für CRGB und LightController
#include "SunTimeManager.h"  // Für Sonnenauf- und -untergangszeiten
#include "AlarmManager.h"    // Für AlarmManager
#include <ArduinoJson.h> // Für JSON-Verarbeitung

// Replaces placeholder with LED state value
String processor(const String &var)
{
   // Temperatur::TempValue temps;
   // Temperatur::GetInstance()->GetLastTemperatures(temps);
   // //   Log::PrintF("Web: %i\t%i\t%i\t%i\r\n", temps.temp1, temps.temp2, temps.temp3, temps.temp4);
   // if (var == "TEMP1")
   // {
   //    return String(temps.temp1);
   // }
   // else if (var == "TEMP2")
   // {
   //    return String(temps.temp2);
   // }
   // else if (var == "TEMP3")
   // {
   //    return String(temps.temp3);
   // }
   // else if (var == "TEMP4")
   // {
   //    return String(temps.temp4);
   // }

   return String("NA");
}

void setupWebServerRoutes(AsyncWebServer& server, AsyncEventSource& events, AsyncWebSocket& ws)
{
   // Route for root / web page
   server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
             { request->send(LittleFS, "/index.html", String(), false, processor); });

   server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request)
             { request->send(LittleFS, "/index.html", String(), false, processor); });

   // Route für die neue Radio-Seite
   server.on("/radio.html", HTTP_GET, [](AsyncWebServerRequest *request)
             { request->send(LittleFS, "/radio.html", "text/html"); });

   server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
             { request->send(LittleFS, "/favicon.ico",  "text/css"); });

   // API-Endpunkt, um alle Wecker-Einstellungen als JSON zu senden
   server.on("/api/alarms", HTTP_GET, [](AsyncWebServerRequest *request) 
   {
      JsonDocument doc;
      JsonArray alarmsArray = doc.to<JsonArray>();
      for (int i = 0; i < AlarmManager::NUM_ALARMS; ++i) 
      {
         const auto& alarm = AlarmManager::GetInstance()->getAlarm(i);
         JsonObject alarmObj = alarmsArray.add<JsonObject>();
         alarmObj["enabled"] = alarm.enabled;
         alarmObj["sunriseEnabled"] = alarm.sunriseEnabled;
         alarmObj["hour"] = alarm.hour;
         alarmObj["minute"] = alarm.minute;
         alarmObj["days"] = alarm.days;
      }
      String response;
      serializeJson(doc, response);
      request->send(200, "application/json", response);
   });

   // API-Endpunkt, um einen Wecker zu setzen
   server.on("/api/setalarm", HTTP_POST, [](AsyncWebServerRequest *request){},
      NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) 
      {
      
      JsonDocument doc;
      if (deserializeJson(doc, data, len) == DeserializationError::Ok) 
      {
         if (!doc["index"].isNull()) 
         {
            int alarmIndex = doc["index"];
            bool enabled = doc["enabled"];
            bool sunriseEnabled = doc["sunriseEnabled"];
            uint8_t hour = doc["hour"];
            uint8_t minute = doc["minute"];
            uint8_t days = doc["days"];

            AlarmManager::GetInstance()->setAlarm(alarmIndex, enabled, sunriseEnabled, hour, minute, days);
            request->send(200, "text/plain", "OK");
            return;
         }
      }
      request->send(400, "text/plain", "Bad Request");
   });

   // API-Endpunkt, um die Radiosenderliste als JSON zu senden
   server.on("/api/radiostations", HTTP_GET, [](AsyncWebServerRequest *request)
   {
      JsonDocument doc;
      JsonArray stationsArray = doc.to<JsonArray>();
      const auto& stations = Config::GetInstance()->GetRadioStations();
      for (const auto& station : stations)
      {
         JsonObject stationObj = stationsArray.add<JsonObject>();
         stationObj["name"] = station.name;
         stationObj["url"] = station.url;
         stationObj["volume"] = station.volume;
      }
      String response;
      serializeJson(doc, response);
      request->send(200, "application/json", response);
   });

   // API-Endpunkt, um die Radiosenderliste zu aktualisieren
   server.on("/api/radiostations", HTTP_POST, {},
      NULL, 
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
         JsonDocument doc;
         if (deserializeJson(doc, data, len) == DeserializationError::Ok)
         {
            JsonArray stationsArray = doc.as<JsonArray>();
            if (!stationsArray.isNull())
            {
               std::vector<Config::RadioStation> newStations;
               for (JsonObject stationObj : stationsArray)
               {
                  newStations.push_back({stationObj["name"].as<String>(), stationObj["url"].as<String>(), stationObj["volume"].as<int>()});
               }
               Config::GetInstance()->SetRadioStations(newStations);
               request->send(200, "text/plain", "OK");
               return;
            }
         }
         request->send(400, "text/plain", "Bad Request");
      });

   // API-Endpunkt, um Statusinformationen abzurufen
   server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
      JsonDocument doc;
      doc["version"] = Config::GetInstance()->GetVersionStringAsCharPtr();
      doc["build"] = Config::GetInstance()->GetBuildDateAsCharPtr();
      doc["hostname"] = Config::GetInstance()->GetWifiHostname();
      doc["ip"] = WiFi.localIP().toString();
      doc["ssid"] = WiFi.SSID();
      doc["rssi"] = WiFi.RSSI();
      doc["time"] = appContext.ntpTime.getFormattedTime();

      // Uptime formatieren
      unsigned long seconds = millis() / 1000;
      unsigned long days = seconds / 86400;
      seconds %= 86400;
      unsigned long hours = seconds / 3600;
      seconds %= 3600;
      unsigned long minutes = seconds / 60;
      doc["uptime"] = String(days) + "d " + String(hours) + "h " + String(minutes) + "m";

      // Sonnenauf- und -untergangszeiten hinzufügen
      time_t sunrise = SunTimeManager::GetInstance()->getSunriseTime();
      time_t sunset = SunTimeManager::GetInstance()->getSunsetTime();
      time_t transit = SunTimeManager::GetInstance()->getTransitTime();
      char timeBuffer[6]; // HH:MM + Null-Terminator

      if (sunrise > 0) {
          strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", localtime(&sunrise));
          doc["sunrise"] = String(timeBuffer);
      } else {
          doc["sunrise"] = "wird berechnet...";
      }

      if (sunset > 0) {
          strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", localtime(&sunset));
          doc["sunset"] = String(timeBuffer);
      } else {
          doc["sunset"] = "wird berechnet...";
      }

      if (transit > 0) {
          strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", localtime(&transit));
          doc["transit"] = String(timeBuffer);
      } else {
          doc["transit"] = "wird berechnet...";
      }

      String response;
      serializeJson(doc, response);
      request->send(200, "application/json", response);
   });

   // API-Endpunkt, um eine statische Farbe zu setzen
   server.on("/api/setcolor", HTTP_POST, [](AsyncWebServerRequest *request){},
      NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      
      JsonDocument doc;
      if (deserializeJson(doc, data, len) == DeserializationError::Ok) {
         if (!doc["color"].isNull() && !doc["brightness"].isNull()) {
            const char* hexColor = doc["color"]; // z.B. "#RRGGBB"
            uint8_t brightness = doc["brightness"];

            // Konvertiere Hex-String zu CRGB
            // strtol ignoriert das '#' am Anfang
            long number = strtol(hexColor + 1, nullptr, 16);
            CRGB color = CRGB(number);

            LightController::GetInstance()->setColor(color, brightness);
            request->send(200, "text/plain", "OK");
            return;
         }
      }
      request->send(400, "text/plain", "Bad Request");
   });

   // API-Endpunkt, um das Licht auszuschalten
   server.on("/api/lightoff", HTTP_POST, [](AsyncWebServerRequest *request) {
      LightController::GetInstance()->stop();
      request->send(200, "text/plain", "OK");
   });


   events.onConnect([](AsyncEventSourceClient *client)
                    {
    if (client->lastId()) {
      Serial.printf("SSE Client reconnected! Last message ID that it gat is: %" PRIu32 "\n", client->lastId());
    }
    client->send("hello!", NULL, millis(), 1000); });

   ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
              {
    (void)len;
    if (type == WS_EVT_CONNECT) {
      Serial.println("ws connect");
      client->setCloseClientOnQueueFull(false);
      client->ping();
    } else if (type == WS_EVT_DISCONNECT) {
      Serial.println("ws disconnect");
    } else if (type == WS_EVT_ERROR) {
      Serial.println("ws error");
    } else if (type == WS_EVT_PONG) {
      Serial.println("ws pong");
    } else if (type == WS_EVT_DATA) {
      AwsFrameInfo* info = (AwsFrameInfo*)arg;
      String msg = "";
      if (info->final && info->index == 0 && info->len == len) {
        if (info->opcode == WS_TEXT) {
          data[len] = 0;
          Serial.printf("ws text: %s\n", (char*)data);
        }
      }
    } });
}