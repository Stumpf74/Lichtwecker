#include <Arduino.h>
#include <assert.h>
#include <string>
#include <LittleFS.h>
#include <WiFi.h>
#include "Log.h"
#include "Config.h"
#include "Avarage.h"
#include "WebServerManager.h"
#include "WebServerRoutes.h"
#include "OtaManager.h"
#include "AppContext.h"
#include "AlarmManager.h"
#include "LightController.h"
#include "DisplayManager.h"
#include "SunTimeManager.h"
#include "InputManager.h"

// Definition und Initialisierung der globalen AppContext-Instanz
AppContext appContext;

// --- Struktur für die Zeitsteuerung ---
struct TimedTask
{
   unsigned long interval;
   unsigned long lastRun;
   void (*task)();
};

/**
 * @brief Konstruktor für den globalen Anwendungskontext.
 */
AppContext::AppContext() : cRssiAvg(16, 0),
                           bfSendStatus(false)
{
}

/**
 * @brief Leitet eine Log-Nachricht an die serielle Schnittstelle weiter.
 * @details Diese Funktion dient als Callback für das Logging-System und wird
 *          verwendet, um Log-Ausgaben direkt auf dem seriellen Monitor anzuzeigen.
 * @param msg Die zu sendende Nachricht.
 */
void SendLogDataToSerial(const std::string &msg)
{
   Serial.print(msg.c_str());
}
/**
 * @brief Wrapper-Funktion, um Log-Nachrichten an den MQTT-Client zu senden.
 * @details Diese Funktion dient als Callback für das Logging-System. Sie konvertiert
 *          den `std::string` in einen Arduino-`String` und übergibt ihn an den MQTT-Client.
 * @param msg Die zu sendende Nachricht als `std::string`.
 */
void SendLogDataToMqttWrapper(const std::string &msg)
{
   appContext.mqttClient.sendLog(String(msg.c_str()));
}

/**
 * @brief Veröffentlicht den aktuellen Systemstatus über MQTT.
 */
void publishStatusToMqtt()
{
   // Sende Status über den neuen MqttClient
   appContext.mqttClient.publish("STATUS", "online", true);
   appContext.mqttClient.publish("VERSION", Config::GetInstance()->GetVersionStringAsCharPtr());
   appContext.mqttClient.publish("BUILD", Config::GetInstance()->GetBuildDateAsCharPtr());
   appContext.mqttClient.publish("LOCALIP", WiFi.localIP().toString());
   appContext.mqttClient.publish("RSSI", String(WiFi.RSSI()));
}

// --- Periodische Aufgaben ---

/**
 * @brief Wird jede Sekunde aufgerufen, um zeitkritische Aufgaben zu erledigen.
 * @details Überprüft die WLAN-Verbindung, verarbeitet Alarme und sendet bei Bedarf
 *          den Systemstatus via MQTT.
 */
void handleEverySecond()
{
   // Stellt die WLAN-Verbindung bei Bedarf wieder her und startet MQTT neu.
   if (appContext.wifiManager.handle())
   {
      // wifiManager.handle() gibt true zurück, wenn eine Wiederverbindung erfolgreich war.
      appContext.mqttClient.begin();
   }

   AlarmManager::GetInstance()->handleAlarms(); // Prüft jede Sekunde, ob ein Alarm ausgelöst werden soll
   DisplayManager::GetInstance()->updateTime(appContext.ntpTime.getFormattedTimeShort().c_str());
   DisplayManager::GetInstance()->updateDate(appContext.ntpTime.getFormattedDate().c_str());

   if (appContext.bfSendStatus)
   {
      appContext.bfSendStatus = false;
      publishStatusToMqtt();
   }
}

/**
 * @brief Wird alle 5 Sekunden aufgerufen.
 * @details Startet einmalig nach dem ersten erfolgreichen WLAN-Connect eine
 *          LED-Testsequenz, um die Funktionsfähigkeit der LEDs zu signalisieren.
 */
void handleEvery5Seconds()
{
   // Starte die LED-Testsequenz einmalig nach dem ersten erfolgreichen WLAN-Connect
   static bool testSequenceStarted = false;
   if (!testSequenceStarted && WiFi.status() == WL_CONNECTED)
   {
      LightController::GetInstance()->startTestSequence();
      testSequenceStarted = true;
   }
}

/**
 * @brief Wird jede Minute aufgerufen.
 * @details Sendet die aktuelle WLAN-Signalstärke (RSSI) über MQTT.
 */
void handleEveryMinute()
{
   int32_t rssi = WiFi.RSSI();
   appContext.mqttClient.publish("RSSI", String(rssi));
}

/**
 * @brief Wird alle 15 Minuten aufgerufen.
 * @details Löst die Neuberechnung der Sonnenauf- und -untergangszeiten aus und setzt ein Flag,
 *          um den vollständigen Systemstatus beim nächsten 1-Sekunden-Tick zu senden.
 */
void handleEvery15Minutes()
{
   appContext.bfSendStatus = true;
   SunTimeManager::GetInstance()->handle(); // Berechnet Sonnenzeiten
}

// Array mit allen periodischen Aufgaben
TimedTask tasks[] = {
    {1000, 0, handleEverySecond},
    {5000, 0, handleEvery5Seconds},
    {60 * 1000, 0, handleEveryMinute},
    {15 * 60 * 1000, 0, handleEvery15Minutes}};
const int numTasks = sizeof(tasks) / sizeof(TimedTask);

/**
 * @brief Die Haupt-Setup-Funktion. Wird einmal beim Start des ESP32 ausgeführt.
 * Initialisiert alle Komponenten wie Logging, WLAN, MQTT, Display, Controller und den Webserver.
 */
void setup()
{
#ifdef DEBUG
   Log::GetInstance()->registerSendFunction(SendLogDataToSerial);
#else
   Log::GetInstance()->registerSendFunction(SendLogDataToMqttWrapper); // MQTT-Logging (noch nicht implementiert)
#endif
   Log::GetInstance()->setLogLevel(Log::LOG_LEVEL_DEBUG); // Setze initialen Loglevel

   Serial.begin(115200);
   delay(100);
   LOG_INFO("\r\n\r\n==================================================");
   LOG_PRINTF(Log::LOG_LEVEL_INFO, "Starte: %s %s", Config::GetInstance()->GetVersionStringAsCharPtr(), Config::GetInstance()->GetBuildDateAsCharPtr());
   delay(100);

   // // --- Display-Pin-Konfiguration und Reset ---
   // // Konfiguriere alle notwendigen Pins als OUTPUT.
   // pinMode(TFT_RST, OUTPUT);
   // // // Manuelle Reset-Sequenz für das Display
   // digitalWrite(TFT_RST, LOW);
   // delay(20);
   // digitalWrite(TFT_RST, HIGH);
   // delay(150);

   // Dateisystem initialisieren, bevor darauf zugegriffen wird
   if (!LittleFS.begin(true))
   {
      LOG_ERROR("Fehler beim Mounten von LittleFS!");
   }

   // Konfiguration laden, nachdem das Dateisystem initialisiert wurde
   Config::GetInstance()->begin();

   // Netzwerk- und Zeit-Komponenten
   if (appContext.wifiManager.begin())
   {
      appContext.mqttClient.begin();
   }

   appContext.ntpTime.begin();
   // Initialisiere die Controller. Es hat sich als stabiler erwiesen,
   DisplayManager::GetInstance()->begin();
   LightController::GetInstance()->begin();
   // Führe einen Test durch, um die Touch-Controller-Kommunikation zu überprüfen
   DisplayManager::GetInstance()->testTouchController();

   AlarmManager::GetInstance()->begin();
   SunTimeManager::GetInstance()->begin();
   InputManager::GetInstance()->begin();

   appContext.otaManager.begin(); // OTA nach den hardware-nahen Bibliotheken initialisieren.

   WebServerManager::GetInstance()->begin();

   // Erst mal alle Daten neu hoch
   LOG_INFO("Initialisierung abgeschlossen.");
   appContext.bfSendStatus = true;
}

/**
 * @brief Die Hauptschleife des Programms.
 * Verarbeitet periodische Aufgaben, Touch-Eingaben und die Handles der verschiedenen Manager.
 */
void loop()
{
   // OTA hat Vorrang. Wenn ein Update läuft, wird der Rest pausiert.
   if (appContext.otaManager.isOtaActive())
   {
      appContext.otaManager.handle();
      return; // Keine weiteren Aktionen während OTA
   }

   // Verarbeite die periodischen Aufgaben
   unsigned long currentMillis = millis();
   for (int i = 0; i < numTasks; i++)
   {
      if (currentMillis - tasks[i].lastRun >= tasks[i].interval)
      {
         tasks[i].lastRun = currentMillis;
         tasks[i].task();
      }
   }

   // Diese Handles müssen in jedem Schleifendurchlauf aufgerufen werden
   LightController::GetInstance()->handle(); // Aktualisiert die LED-Animationen
   DisplayManager::GetInstance()->handle(); // Verarbeitet Touch-Eingaben
   InputManager::GetInstance()->handle(); // Verarbeitet Benutzereingaben (z.B. Touch)
   appContext.mqttClient.handle();        // Verarbeitet MQTT-Nachrichten
   appContext.otaManager.handle();        // Verarbeitet ota-Nachrichten
}
