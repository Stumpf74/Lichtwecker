#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "AppContext.h"      // Für NTPTime
#include "SunTimeManager.h"  // Für Sonnenauf- und -untergangszeiten
#include <TFT_eSPI.h>
// Weist FastLED an, den I2S-Controller zu verwenden, um den Konflikt mit TFT_eSPI (SPI-Controller) zu lösen.
// Muss VOR dem Inkludieren von FastLED.h stehen.
#define FASTLED_ESP32_I2S true
#include <FastLED.h> // Für CRGB



class DisplayManager
{
public:
    // Definiert die verschiedenen Anzeigezustände
    enum DisplayState { STATE_CLOCK, STATE_MENU };
    /**
     * @brief Gibt die Singleton-Instanz des DisplayManagers zurück.
     * @return DisplayManager* Zeiger auf die Instanz.
     */
    static DisplayManager* GetInstance();

    /**
     * @brief Initialisiert das Display, den Touch-Controller und die UI-Elemente.
     */
    void begin();

    /**
     * @brief Verarbeitet periodische Aufgaben, wie z.B. das Abfragen von Touch-Eingaben.
     */
    void handle();

    /**
     * @brief Zeichnet die statische Benutzeroberfläche (Titel, etc.).
     */
    void drawUi();

    /**
     * @brief Aktualisiert die Zeitanzeige auf dem Display.
     * @param timeStr Der anzuzeigende Zeit-String (z.B. "HH:MM:SS").
     */
    void updateTime(const char* timeStr);

    /**
     * @brief Aktualisiert die Datumsanzeige auf dem Display.
     * @param dateStr Der anzuzeigende Datums-String (z.B. "DD.MM.YYYY").
     */
    void updateDate(const char* dateStr);

    /**
     * @brief Fragt die Touch-Koordinaten ab.
     * @param x Referenz für die X-Koordinate.
     * @param y Referenz für die Y-Koordinate.
     * @return true, wenn der Bildschirm berührt wird, sonst false.
     */
    bool getTouch(uint16_t& x, uint16_t& y);

    /**
     * @brief Testet die Kommunikation mit dem Touch-Controller durch Auslesen des Z-Wertes.
     */
    void testTouchController();

    /**
     * @brief Verarbeitet eine Touch-Eingabe basierend auf dem aktuellen Anzeigezustand.
     * @param x Die X-Koordinate der Berührung.
     * @param y Die Y-Koordinate der Berührung.
     */
    void handleTouch(uint16_t x, uint16_t y);

private:
    /**
     * @brief Privater Konstruktor für das Singleton-Muster.
     */
    DisplayManager();
    ~DisplayManager() = default;
    DisplayManager(const DisplayManager&) = delete;
    DisplayManager& operator=(const DisplayManager&) = delete;

    /**
     * @brief Startet die Touchscreen-Kalibrierungsroutine.
     * Die Kalibrierungsdaten werden auf dem seriellen Monitor ausgegeben.
     */
    void calibrateTouch();

    /**
     * @brief Initialisiert die Positionen für eine statische, zentrierte Anzeige.
     */
    void initializeStaticPositions();

    /**
     * @brief Initialisiert die Positionen und Bewegungsvektoren für eine bewegte Anzeige.
     */
    void initializeMovingPositions();

    /**
     * @brief Berechnet die neuen Positionen für die bewegte Anzeige.
     */
    void updateMovingPositions();

    TFT_eSPI m_tft;
    TFT_eSprite m_timeSprite; // Sprite für die Uhrzeit
    TFT_eSprite m_dateSprite; // Sprite für das Datum

    // Variablen für die Bewegung der Uhrzeit
    int16_t m_timeX, m_timeY;
    int16_t m_timeDX, m_timeDY;

    // Variablen für die Bewegung des Datums
    int16_t m_dateX, m_dateY;
    int16_t m_dateDX, m_dateDY;

    // Variablen für die letzte Position, um flackerfrei zu löschen
    int16_t m_lastTimeX, m_lastTimeY;
    int16_t m_lastDateX, m_lastDateY;

    // Letzte Zeit, zu der die Sprites aktualisiert wurden
    unsigned long m_lastMoveTime;

    // Letzte angezeigte Werte, um unnötiges Neuzeichnen zu vermeiden
    String m_lastTimeStr;
    String m_lastDateStr;

    // Aktuelle Werte, die angezeigt werden sollen (für häufiges Neuzeichnen während Fading)
    // Diese werden von updateTime/updateDate gesetzt und von drawCurrentSprites verwendet.
    String m_currentTimeStr;
    String m_currentDateStr;


    // Steuert, ob die Anzeige sich bewegt oder statisch ist.
    bool m_isMoving;

    /**
     * @brief Behandelt die Logik für die kontinuierlich bewegte Anzeige.
     */
    void handleMoving();

    /**
     * @brief Behandelt die Logik für die statische Anzeige mit periodischem Positionswechsel.
     */
    void handleStaticWithRelocation();

    // --- Variablen für die statische Anzeige mit Positionswechsel ---
    enum FadeState { NONE, FADING_OUT, FADING_IN };
    FadeState m_fadeState;

    unsigned long m_lastFadeRedrawTime;     // Zeitstempel der letzten Fading-Aktualisierung
    unsigned long m_lastPositionChangeTime; // Zeitstempel des letzten Positionswechsels
    unsigned long m_fadeStartTime;          // Zeitstempel für den Start der Überblendung

    // Zielpositionen für den nächsten Fade-In
    int16_t m_nextTimeX, m_nextTimeY;
    int16_t m_nextDateX, m_nextDateY;

    /**
     * @brief Berechnet eine neue zufällige, statische Position für Zeit und Datum.
     * @param outTimeX Referenz für die neue X-Position der Zeit.
     * @param outTimeY Referenz für die neue Y-Position der Zeit.
     * @param outDateX Referenz für die neue X-Position des Datums.
     * @param outDateY Referenz für die neue Y-Position des Datums.
     */
    void calculateNewRandomPosition(int16_t& outTimeX, int16_t& outTimeY, int16_t& outDateX, int16_t& outDateY);

    /**
     * @brief Zeichnet die Zeit- und Datumssprites auf das Display.
     * Diese Methode wird häufig aufgerufen, besonders während Fading-Animationen oder Bewegungen.
     */
    void drawCurrentSprites();

    /**
     * @brief Berechnet die Farbe während einer Fading-Animation.
     * @param baseColor Die Basisfarbe als CRGB-Objekt.
     * @return Die berechnete Farbe im RGB565-Format für das Display.
     */
    uint16_t calculateFadedColor(CRGB baseColor);

    /**
     * @brief Prüft, ob der Nachtmodus für das Display aktiv sein soll.
     * @return true, wenn es Nacht ist, sonst false.
     */
    bool isNightMode() const;

    /**
     * @brief Zeichnet das Hauptmenü auf dem Display.
     */
    void drawMenu();

    DisplayState m_displayState; // Aktueller Anzeigezustand
    TFT_eSPI_Button m_backButton;
    TFT_eSPI_Button m_forwardButton;
};

#endif // DISPLAY_MANAGER_H