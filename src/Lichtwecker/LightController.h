#ifndef LIGHT_CONTROLLER_H
#define LIGHT_CONTROLLER_H

// Weist FastLED an, den I2S-Controller zu verwenden, um den Konflikt mit TFT_eSPI (SPI-Controller) zu lösen.
// Muss VOR dem Inkludieren von FastLED.h stehen.
#define FASTLED_ESP32_I2S true
#include <FastLED.h>

// Vorwärtsdeklaration
class LightController;

class LightController {
public:
    static LightController* GetInstance();

    void begin();
    void handle();

    // Startet die Weck-Routine über eine Dauer von 'durationMinutes'
    void startWakeUpRoutine(uint16_t durationMinutes = 30);
    
    // Startet das Herunterdimmen über 1 Minute
    void startDimDownRoutine();

    // Stoppt alle Licht-Animationen und schaltet die LEDs aus
    void stop();

    // Setzt eine statische Farbe und Helligkeit
    void setColor(CRGB color, uint8_t brightness);

    // Startet die LED-Testsequenz
    void startTestSequence();

    // Schaltet das statische Licht ein/aus
    void toggleStaticLight();

    // Prüft, ob eine Weck- oder Dimm-Animation läuft
    bool isWakeUpOrDimDownActive() const;

private:
    LightController();
    ~LightController() = default;
    LightController(const LightController&) = delete;
    LightController& operator=(const LightController&) = delete;

    void updateWakeUp();
    void updateDimDown();
    void updateTestSequence();
    void updateAutoShutdown();

    // --- LED Konfiguration ---
    static const int NUM_LEDS = 60; // Passe die Anzahl deiner LEDs an
    static const int LED_PIN = 15;  // Passe den GPIO-Pin an
    CRGB m_leds[NUM_LEDS];

    bool m_isTesting;
    unsigned long m_lastTestStepTime;
    bool m_isWakingUp;
    bool m_isWakeUpComplete; // Flag für den Zustand nach dem Wecken
    unsigned long m_autoShutdownStartTime; // Zeitstempel für den Auto-Shutdown
    bool m_isDimmingDown;
    unsigned long m_wakeUpStartTime;
    uint16_t m_wakeUpDurationSeconds;
    unsigned long m_dimDownStartTime;
    uint8_t m_dimDownStartBrightness;
    CRGB m_lastStaticColor;
    uint8_t m_lastStaticBrightness;
};

#endif // LIGHT_CONTROLLER_H