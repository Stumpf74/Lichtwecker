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

    uint32_t m_connectionStartedAt;
    uint32_t m_nextRetryAt;
    bool m_isConnecting;
    bool m_wasConnected;
};

#endif // WIFI_MANAGER_H