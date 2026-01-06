#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <WiFi.h>
#include <PubSubClient.h>

class MqttClient
{
public:
    MqttClient();
    ~MqttClient() = default;

    void begin();

    /**
     * @brief Verarbeitet die MQTT-Schleife und prüft die Verbindung.
     */
    void handle();

    /**
     * @brief Sendet Log-Daten an den MQTT-Broker.
     * @param msg Die zu sendende Nachricht.
     */
    void sendLog(const String &msg);

    /**
     * @brief Veröffentlicht eine Nachricht zu einem bestimmten Unterthema.
     */
    void publish(const String& subTopic, const String& message, bool retained = false);

private:
    MqttClient(const MqttClient&) = delete;
    MqttClient& operator=(const MqttClient&) = delete;

    void reconnect();
    static void callback(char* topic, byte* payload, unsigned int length); // Muss statisch bleiben

    WiFiClient m_espClient;
    PubSubClient m_client;
    unsigned long m_nextReconnectAttempt; // Zeitstempel für den nächsten Wiederverbindungsversuch
};

#endif // MQTT_CLIENT_H