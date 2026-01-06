#include "LightController.h"
#include "Log.h"

// Eine Farbpalette für den Sonnenaufgang: von tiefrot über orange zu hellem Weiß
const TProgmemRGBPalette16 WakeUpPalette_p PROGMEM =
    {
        CRGB::DarkRed, CRGB::DarkRed, CRGB::Red, CRGB::Orange,
        CRGB::Orange, CRGB::Gold, CRGB::Gold, CRGB::Yellow,
        CRGB::Yellow, CRGB::LightGoldenrodYellow, CRGB::LightGoldenrodYellow, CRGB::Cornsilk,
        CRGB::Cornsilk, CRGB::White, CRGB::White, CRGB::White};

// Eine schönere, sanftere Farbpalette für den Sonnenaufgang
// Index, Rot, Grün, Blau
DEFINE_GRADIENT_PALETTE(sunrise_gp)
{
    0, 10, 0, 0,        // 1. Tiefes, dunkles Rot (fast schwarz)
    60, 139, 0, 0,      // 2. Sattes Dunkelrot
    120, 255, 80, 0,    // 3. Leuchtendes Orange
    180, 255, 200, 40,  // 4. Warmes, helles Gelb
    220, 255, 255, 180, // 5. Helles, warmweißes Licht
    255, 255, 255, 255  // 6. Reines, helles Weiß (Tageslicht)
};

CRGBPalette256 m_sunrisePal = sunrise_gp;

LightController::LightController() : m_isWakingUp(false),
                                     m_isTesting(false),
                                     m_isDimmingDown(false),
                                     m_isWakeUpComplete(false),
                                     m_autoShutdownStartTime(0),
                                     m_lastTestStepTime(0),
                                     m_wakeUpStartTime(0),
                                     m_wakeUpDurationSeconds(0),
                                     m_lastStaticColor(CRGB::Orange),
                                     m_lastStaticBrightness(128)
{
}

LightController *LightController::GetInstance()
{
    static LightController instance;
    return &instance;
}

void LightController::begin()
{
    LOG_INFO("Init LightController");
    // Wähle hier deinen LED-Typ und die Pin-Belegung (z.B. WS2812B, GRB-Reihenfolge)
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(m_leds, NUM_LEDS);
    FastLED.setBrightness(0); // LEDs beim Start ausschalten
    FastLED.show();
}

void LightController::handle()
{
    // Diese Log-Nachricht kann sehr gesprächig sein, daher auskommentiert.
    // Bei Bedarf einkommentieren, um zu sehen, ob die handle-Funktion überhaupt aufgerufen wird.
    // Log::PrintLN("LightController::handle()");

    if (m_isWakingUp)
    {
        updateWakeUp();
    }
    else if (m_isTesting) // Nur wenn keine Weck-Routine läuft, die Testsequenz ausführen
    {
        updateTestSequence();
    }
    else if (m_isDimmingDown)
    {
        updateDimDown();
    }
    else if (m_isWakeUpComplete)
    {
        updateAutoShutdown();
    }
}

void LightController::startWakeUpRoutine(uint16_t durationMinutes)
{
    if (m_isWakingUp)
    {
        LOG_DEBUG("Weck-Routine läuft bereits.");
        return;
    }
    LOG_PRINTF(Log::LOG_LEVEL_INFO, "Starte Weck-Routine (%d Minuten).", durationMinutes);

    m_isWakingUp = true;
    m_isTesting = false; // Testsequenz beenden, falls sie läuft
    m_isDimmingDown = false; // Auch die Dimm-Routine beenden
    m_isWakeUpComplete = false;
    m_wakeUpStartTime = millis(); // Zeitstempel für den Start der Routine
    m_wakeUpDurationSeconds = durationMinutes * 60;
}

void LightController::stop()
{
    m_isTesting = false;
    m_isWakingUp = false;
    m_isDimmingDown = false;
    m_isWakeUpComplete = false;
    FastLED.setBrightness(0);
    FastLED.show();
    LOG_INFO("Licht gestoppt.");
}

void LightController::setColor(CRGB color, uint8_t brightness)
{
    m_isTesting = false;  // Stoppt auch die Test-Routine
    m_isWakingUp = false; // Stoppt die Weck-Routine, falls sie lief
    m_isDimmingDown = false;
    m_isWakeUpComplete = false;
    FastLED.setBrightness(brightness);
    // Speichere die letzte Einstellung für die Toggle-Funktion
    m_lastStaticColor = color;
    m_lastStaticBrightness = brightness;
    fill_solid(m_leds, NUM_LEDS, color);
    FastLED.show();
}

void LightController::updateWakeUp()
{
    // Log::PrintLN("LightController::updateWakeUp()"); // Sehr gesprächig, nur bei Bedarf aktivieren

    unsigned long elapsedTime = (millis() - m_wakeUpStartTime) / 1000;

    if (elapsedTime >= m_wakeUpDurationSeconds)
    {
        // Weck-Routine beendet, Licht auf voller Helligkeit lassen
        m_isWakingUp = false;
        m_isWakeUpComplete = true; // Gehe in den Zustand "Warten auf Auto-Shutdown"
        m_autoShutdownStartTime = millis(); // Starte den 60-Minuten-Timer
        LOG_INFO("Weck-Routine beendet.");
        // Setze die finale Farbe und Helligkeit
        uint8_t finalBrightness = 255;
        CRGB finalColor = ColorFromPalette(m_sunrisePal, 255);
        FastLED.setBrightness(finalBrightness);
        fill_solid(m_leds, NUM_LEDS, finalColor);
        FastLED.show();
    }

    // Berechne den Fortschritt (0-255)
    uint8_t progress = map(elapsedTime, 0, m_wakeUpDurationSeconds, 0, 255);
    //Log::PrintF("WakeUp - Fortschritt: %d/255, Helligkeit: %d\r\n", progress, FastLED.getBrightness());

    // Setze die Helligkeit (langsam ansteigend)
    uint8_t brightness = ease8InOutQuad(progress); // Sanfter Anstieg
    FastLED.setBrightness(brightness);

    // Setze die Farbe basierend auf der Palette
    CRGB color = ColorFromPalette(m_sunrisePal, progress);
    fill_solid(m_leds, NUM_LEDS, color);

    FastLED.show();
}

void LightController::updateAutoShutdown()
{
    const unsigned long autoShutdownMinutes = 60;
    const unsigned long autoShutdownMillis = autoShutdownMinutes * 60 * 1000;

    if (millis() - m_autoShutdownStartTime >= autoShutdownMillis)
    {
        LOG_PRINTF(Log::LOG_LEVEL_INFO, "Auto-Shutdown nach %d Minuten. Schalte Licht aus.", autoShutdownMinutes);
        stop(); // Ruft stop() auf, was alle Flags zurücksetzt und das Licht ausschaltet.
    }
    // Ansonsten tue nichts und lasse das Licht an, bis der Timer abläuft
    // oder eine andere Aktion (stop, setColor, etc.) den Zustand ändert.
}

void LightController::startTestSequence()
{
    LOG_INFO("Starte LED-Testsequenz.");
    // Setze alle anderen Zustände zurück, um einen sauberen Start zu gewährleisten.
    m_isWakingUp = false;
    m_isDimmingDown = false;
    m_isWakeUpComplete = false;
    m_isTesting = true;
    m_lastTestStepTime = millis();

    // Start mit Rot
    FastLED.setBrightness(100); // Eine moderate Helligkeit für den Test
    fill_solid(m_leds, NUM_LEDS, CRGB::Red);
    FastLED.show();
}

void LightController::updateTestSequence()
{
    // Log::PrintLN("LightController::updateTestSequence()"); // Sehr gesprächig, nur bei Bedarf aktivieren

    static int testColorIndex = 0; // 0=Rot, 1=Grün, 2=Blau
    const CRGB testColors[] = {CRGB::Red, CRGB::Green, CRGB::Blue};

    if (millis() - m_lastTestStepTime >= 1000)
    { // Jede Sekunde umschalten
        m_lastTestStepTime = millis();
        testColorIndex = (testColorIndex + 1) % 3;

        LOG_PRINTF(Log::LOG_LEVEL_DEBUG, "Testsequenz - Nächste Farbe: %d", testColorIndex);
        fill_solid(m_leds, NUM_LEDS, testColors[testColorIndex]);
        FastLED.show();

        // Die Testsequenz nach einer vollen Runde (R,G,B) beenden und LEDs ausschalten.
        if (testColorIndex == 0)
        { // Nach Blau wieder bei Rot angekommen -> Stopp
            stop();
            LOG_INFO("LED-Testsequenz beendet.");
        }
    }
}

void LightController::startDimDownRoutine()
{
    // Startet nur, wenn eine Weck-Routine aktiv ist oder das Licht manuell an ist.
    if (!m_isWakingUp && FastLED.getBrightness() == 0)
    {
        return;
    }

    LOG_INFO("Starte Herunterdimmen...");
    m_isWakingUp = false;
    m_isTesting = false;
    m_isWakeUpComplete = false;
    m_isDimmingDown = true;
    m_dimDownStartTime = millis();
    m_dimDownStartBrightness = FastLED.getBrightness(); // Speichere aktuelle Helligkeit
}

void LightController::updateDimDown()
{
    // Log::PrintLN("LightController::updateDimDown()"); // Sehr gesprächig, nur bei Bedarf aktivieren

    const uint16_t durationSeconds = 60; // 1 Minute
    unsigned long elapsedTime = (millis() - m_dimDownStartTime) / 1000;

    if (elapsedTime >= durationSeconds)
    {
        // Herunterdimmen beendet
        LOG_INFO("Herunterdimmen beendet.");
        stop(); // Schaltet das Licht komplett aus
        return;
    }

    // Berechne den Fortschritt (von 255 nach 0)
    uint8_t progress = map(elapsedTime, 0, durationSeconds, 255, 0);
    // Skaliere die Helligkeit von der Starthelligkeit auf 0
    uint8_t newBrightness = scale8(m_dimDownStartBrightness, ease8InOutQuad(progress));
    //Log::PrintF("DimDown - Fortschritt: %d/255, Helligkeit: %d\r\n", progress, newBrightness);

    FastLED.setBrightness(newBrightness);
    FastLED.show();
}

void LightController::toggleStaticLight()
{
    // Diese Funktion soll keine laufenden Animationen unterbrechen
    if (m_isWakingUp || m_isDimmingDown || m_isTesting || m_isWakeUpComplete)
    {
        return;
    }

    // Wenn das Licht an ist, schalte es aus.
    if (FastLED.getBrightness() > 0)
    {
        stop();
    }
    // Wenn das Licht aus ist, schalte es mit den letzten Werten wieder ein.
    else
    {
        setColor(m_lastStaticColor, m_lastStaticBrightness);
    }
}

bool LightController::isWakeUpOrDimDownActive() const
{
    return m_isWakingUp || m_isDimmingDown;
}