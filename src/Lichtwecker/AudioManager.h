#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <Audio.h>  // Neue Include für ESP32-audioI2S
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

class AudioManager
{
public:
    static AudioManager *GetInstance();

    void begin();
    void handle();

    // Neue Methoden für Webstreams
    void playStream(const String& url);
    void stopStream();
    void setVolume(int volume);
    int getVolume();

private:
    enum class CommandType : uint8_t
    {
        Play,
        Stop,
        SetVolume
    };

    struct AudioCommand
    {
        CommandType type;
        uint8_t volume;
        char url[256];
    };

    AudioManager();
    ~AudioManager() = default;
    AudioManager(const AudioManager &) = delete;
    AudioManager &operator=(const AudioManager &) = delete;

    Audio m_audio;  // Instanz der Audio-Klasse
    int m_currentVolume;  // Aktuelle Lautstärke
    QueueHandle_t m_commandQueue;
};

#endif // AUDIO_MANAGER_H