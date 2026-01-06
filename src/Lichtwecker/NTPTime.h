#ifndef INC_NTPTIME_H
#define INC_NTPTIME_H

#include <Arduino.h>
#include <time.h> // Für time_t, tm, localtime_r, strftime

class NTPTime
{
public:
    NTPTime(); // Parameterloser Konstruktor
    ~NTPTime();

    void begin();
    // update() ist nicht mehr nötig, da configTime im Hintergrund synchronisiert
    time_t getEpochTime(); // Rückgabetyp time_t für Standard-C-Funktionen
    String getFormattedTime();
    String getFormattedTimeShort();
    String getFormattedDate();

private:
    // Muss static sein, um als C-Style-Callback verwendet zu werden.
    static void time_sync_notification_cb(struct timeval *tv);
};

#endif