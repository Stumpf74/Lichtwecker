#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h> // Fügt Standard-Arduino-Typen wie uint32_t hinzu

class WifiManager
{
public:
    WifiManager();
    ~WifiManager() = default;

    bool begin();
    bool handle();

private:
    WifiManager(const WifiManager&) = delete;
    WifiManager& operator=(const WifiManager&) = delete;

    uint32_t m_wifiRetryCount;
    static const uint32_t MAX_WIFI_RETRY = 15;
};

#endif // WIFI_MANAGER_H