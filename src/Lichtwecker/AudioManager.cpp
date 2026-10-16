#include "AudioManager.h"
#include "Config.h"
#include "Log.h"

AudioManager::AudioManager() : m_audio(false, 3, 0), m_currentVolume(35), m_commandQueue(nullptr)
{

    m_audio.setVolume((m_currentVolume * 21 + 50) / 100);
}

AudioManager *AudioManager::GetInstance()
{
    static AudioManager instance;
    return &instance;
}

void AudioManager::begin()
{
    LOG_INFO("Init AudioManager");
    m_commandQueue = xQueueCreate(8, sizeof(AudioCommand));
    if (m_commandQueue == nullptr)
    {
        LOG_ERROR("Audio-Befehlsqueue konnte nicht erstellt werden.");
        return;
    }

    m_currentVolume = Config::GetInstance()->GetRadioVolume();
    m_audio.setVolume((m_currentVolume * 21 + 50) / 100);
    // I2S-Pins setzen nach anderen Inits, um Konflikte zu vermeiden
    m_audio.setPinout(I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DIN_PIN);
    LOG_INFO("AudioManager initialized successfully");
}

void AudioManager::handle()
{
    if (m_commandQueue != nullptr)
    {
        AudioCommand command;
        while (xQueueReceive(m_commandQueue, &command, 0) == pdPASS)
        {
            switch (command.type)
            {
            case CommandType::Play:
                m_audio.connecttohost(command.url);
                break;
            case CommandType::Stop:
                m_audio.stopSong();
                break;
            case CommandType::SetVolume:
                m_audio.setVolume(command.volume);
                break;
            }
        }
    }

    m_audio.loop();  // Muss regelmäßig aufgerufen werden für Streaming
}

void AudioManager::playStream(const String& url)
{
    AudioCommand command{};
    command.type = CommandType::Play;
    url.toCharArray(command.url, sizeof(command.url));
    if (m_commandQueue == nullptr || xQueueSend(m_commandQueue, &command, 0) != pdPASS)
    {
        LOG_ERROR("Radio-Start konnte nicht an den Audio-Task übergeben werden.");
    }
}

void AudioManager::stopStream()
{
    AudioCommand command{};
    command.type = CommandType::Stop;
    if (m_commandQueue == nullptr || xQueueSend(m_commandQueue, &command, 0) != pdPASS)
    {
        LOG_ERROR("Radio-Stopp konnte nicht an den Audio-Task übergeben werden.");
    }
}

void AudioManager::setVolume(int volume)
{
    if (volume < 0)
        volume = 0;
    else if (volume > 100)
        volume = 100;

    if (volume == m_currentVolume)
        return;

    m_currentVolume = volume;
    Config::GetInstance()->SetRadioVolume(volume);

    AudioCommand command{};
    command.type = CommandType::SetVolume;
    command.volume = (volume * 21 + 50) / 100;
    if (m_commandQueue == nullptr || xQueueSend(m_commandQueue, &command, 0) != pdPASS)
    {
        LOG_ERROR("Lautstärke konnte nicht an den Audio-Task übergeben werden.");
    }
}

int AudioManager::getVolume()
{
    return m_currentVolume;
}