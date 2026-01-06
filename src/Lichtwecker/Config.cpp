#include <Arduino.h>
#include "Config.h"

const char* CONFIG_FILE = "/config.json"; 

void Config::setDefaultRadioStations()
{
    m_tsConfig.radioStations = {
        {"Antenne", "http://mp3channels.webradio.antenne.de/antenne", 75},
        {"Ant_80er", "http://mp3channels.webradio.antenne.de/80er-kulthits", 75},
        {"Ant_90er", "http://mp3channels.webradio.antenne.de/90er-hits", 75},
        {"Ant_black", "http://mp3channels.webradio.antenne.de/black-beatz", 75},
        {"Ant_cillout", "http://play.antenne.de/chillout.m3u", 75},
        {"Ant_classic_rock", "http://mp3channels.webradio.antenne.de/classic-rock-live", 75},
        {"Ant_event", "http://mp3channels.webradio.antenne.de/event", 75},
        {"Ant_fresh", "http://mp3channels.webradio.antenne.de/fresh", 75},
        {"Ant_kids", "http://mp3channels.webradio.antenne.de/hits-fuer-kids", 75},
        {"Ant_love", "http://mp3channels.webradio.antenne.de/lovesongs", 75},
        {"Ant_new_hits", "http://mp3channels.webradio.antenne.de/new-hits", 75},
        {"Ant_oldies", "http://mp3channels.webradio.antenne.de/oldies-but-goldies", 75},
        {"Ant_rock", "http://play.rockantenne.de/rockantenne.m3u", 75},
        {"Ant_schlager", "http://mp3channels.webradio.antenne.de/das-schlager-karussell", 75},
        {"Ant_top40", "http://mp3channels.webradio.antenne.de/top-40", 75},
        {"Ant_workout", "http://mp3channels.webradio.antenne.de/workout-hits", 75},
        {"Bayern3", "http://streams.br.de/bayern3_2.m3u", 75},
        {"Swr3", "http://dg-swr-http-dus-dtag-cdn.cast.addradio.de/swr/swr3/live/mp3/128/stream.mp3", 75},
        {"Gong", "http://fhr-radiogongfm-live.cast.addradio.de/fhr/radiogongfm/live/mp3/128/stream.mp3", 75},
        {"Ant_coffee", "http://play.antenne.de/coffee.m3u", 75},
        {"Bob", "http://streams.radiobob.de/bob-national/mp3-192/mediaplayer", 75}
    };
}

void Config::begin()
{
   loadConfig();
}

void Config::loadConfig()
{
    if (!LittleFS.exists(CONFIG_FILE))
    {
        LOG_INFO("Keine Konfigurationsdatei gefunden. Standardwerte werden erstellt.");
        setDefaultRadioStations();
        saveConfig();
        return;
    }

    File configFile = LittleFS.open(CONFIG_FILE, "r");
    if (!configFile)
    {
        LOG_ERROR("Fehler beim Öffnen der Konfigurationsdatei zum Lesen");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error)
    {
        LOG_PRINTF(Log::LOG_LEVEL_ERROR, "Fehler beim Parsen der Konfigurationsdatei: %s", error.c_str());
        return;
    }

    // Überschreibe die Standardwerte nur, wenn die Schlüssel in der JSON-Datei vorhanden sind.
    if (!doc["loggingActive"].isNull())
    {
        m_tsConfig.loggingActive = doc["loggingActive"];
    }

    if (!doc["latitude"].isNull())
    {
        m_tsConfig.latitude = doc["latitude"];
    }
    if (!doc["longitude"].isNull())
    {
        m_tsConfig.longitude = doc["longitude"];
    }
    if (!doc["maxCountValueUnchanged"].isNull())
    {
        m_tsConfig.maxCountValueUnchanged = doc["maxCountValueUnchanged"];
    }
    if (doc.containsKey("radioStations"))
    {
        JsonArray stationsArray = doc["radioStations"].as<JsonArray>();
        m_tsConfig.radioStations.clear();
        for (JsonObject stationObj : stationsArray)
        {
            m_tsConfig.radioStations.push_back({
                stationObj["name"].as<String>(),
                stationObj["url"].as<String>(),
                stationObj["volume"].as<int>()
            });
        }
    }
    else
    {
        // Wenn keine Sender in der config sind, die Standardwerte laden
        setDefaultRadioStations();
    }

    LOG_INFO("Konfiguration erfolgreich geladen.");
}

void Config::saveConfig()
{
    JsonDocument doc;
    doc["loggingActive"] = m_tsConfig.loggingActive;
    doc["latitude"] = m_tsConfig.latitude;
    doc["longitude"] = m_tsConfig.longitude;
    doc["maxCountValueUnchanged"] = m_tsConfig.maxCountValueUnchanged;

    JsonArray stationsArray = doc["radioStations"].to<JsonArray>();
    for(const auto& station : m_tsConfig.radioStations) {
        JsonObject stationObj = stationsArray.add<JsonObject>();
        stationObj["name"] = station.name;
        stationObj["url"] = station.url;
        stationObj["volume"] = station.volume;
    }

    File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (!configFile)
    {
        LOG_ERROR("Fehler beim Öffnen der Konfigurationsdatei zum Schreiben");
        return;
    }

    if (serializeJson(doc, configFile) == 0)
    {
        LOG_ERROR("Fehler beim Schreiben in die Konfigurationsdatei");
    }
    else
    {
        LOG_INFO("Konfiguration erfolgreich gespeichert.");
    }
    configFile.close();
}

/**
 * @brief Gibt build Zeitpunkt zurück
 * 
 * @return const char* 
 */
const char* Config::GetBuildDate()
{
   static String buildInfo = String(ptrBuildDate) + " " + String(ptrBuildTime);
   return buildInfo.c_str();
}

const char* Config::GetBuildDateAsCharPtr() 
{
    static String buildInfo = String(ptrBuildDate) + " " + String(ptrBuildTime);
    return buildInfo.c_str();
}
const char* Config::GetVersionString()
{
   static String buildInfo = String(ptr_VersionName) + " V" + String(ptr_VersionNumber); 
    return buildInfo.c_str();
}


const char * Config::GetVersionStringAsCharPtr()
{
   static String buildInfo = String(String(ptr_VersionName) + " V" + String(ptr_VersionNumber)).c_str();
   return buildInfo.c_str();
}
