#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include "WifiManager.h"
#include "MqttClient.h"
#include "OtaManager.h"
#include "NTPTime.h"
#include "Avarage.h"

struct AppContext
{
    AppContext();
    WifiManager wifiManager;
    MqttClient mqttClient;
    OtaManager otaManager;
    NTPTime ntpTime;
    CAverage<int32_t> cRssiAvg;
    bool bfSendStatus;
};

extern AppContext appContext;

#endif // APP_CONTEXT_H