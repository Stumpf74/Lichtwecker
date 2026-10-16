#include "WebServerRoutes.h"
#include <LittleFS.h>
#include "Log.h"
#include "AppContext.h"
#include "LightController.h" // Für CRGB und LightController
#include "SunTimeManager.h"  // Für Sonnenauf- und -untergangszeiten
#include "AlarmManager.h"    // Für AlarmManager
#include <ArduinoJson.h> // Für JSON-Verarbeitung
#include <WiFi.h>

void logHttpRequest(const char* route, AsyncWebServerRequest* request)
{
   LOG_PRINTF(Log::LOG_LEVEL_INFO, "HTTP %s %s WiFi=%d Heap=%u", route,
              request->url().c_str(), WiFi.status(), ESP.getFreeHeap());
}

void sendJsonResponse(AsyncWebServerRequest* request, JsonDocument& document)
{
   AsyncResponseStream* response = request->beginResponseStream("application/json", 256);
   if (response == nullptr)
   {
      request->send(503, "text/plain", "Insufficient memory");
      return;
   }

   serializeJson(document, *response);
   request->send(response);
}

void sendIndexPage(AsyncWebServerRequest* request)
{
   AsyncWebServerResponse* response = request->beginResponse(LittleFS, "/index.html", "text/html");
   if (response == nullptr)
   {
      LOG_PRINTF(Log::LOG_LEVEL_ERROR, "HTTP index response fehlgeschlagen. Heap=%u, Datei vorhanden=%d",
                 ESP.getFreeHeap(), LittleFS.exists("/index.html"));
      request->send(503, "text/plain", "Index temporarily unavailable");
      return;
   }
   request->send(response);
}

void setupWebServerRoutes(AsyncWebServer& server, AsyncEventSource& events, AsyncWebSocket& ws)
{
   // Route for root / web page
   server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
             {
                logHttpRequest("index", request);
                sendIndexPage(request);
             });

   server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request)
             {
                logHttpRequest("index", request);
                sendIndexPage(request);
             });

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
         alarmObj["radioEnabled"] = alarm.radioEnabled;
         alarmObj["hour"] = alarm.hour;
         alarmObj["minute"] = alarm.minute;
         alarmObj["days"] = alarm.days;
      }
      sendJsonResponse(request, doc);
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
            bool radioEnabled = doc["radioEnabled"] | false;
            uint8_t hour = doc["hour"];
            uint8_t minute = doc["minute"];
            uint8_t days = doc["days"];

            AlarmManager::GetInstance()->setAlarm(alarmIndex, enabled, sunriseEnabled, radioEnabled, hour, minute, days);
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
      sendJsonResponse(request, doc);
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
      char uptimeBuffer[32];
      snprintf(uptimeBuffer, sizeof(uptimeBuffer), "%lud %luh %lum", days, hours, minutes);
      doc["uptime"] = uptimeBuffer;

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

      sendJsonResponse(request, doc);
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