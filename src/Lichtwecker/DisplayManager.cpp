#include <algorithm> // Für std::min
#include <WiFi.h>
#include "Config.h"
#include "AudioManager.h"
#include "AlarmManager.h"
#include "LightController.h"
#include "DisplayManager.h"
#include "Log.h"

/**
 * @brief Konstruktor für den DisplayManager.
 * Initialisiert die TFT_eSPI-Instanz und das Sprite für die Uhrzeit.
 */
DisplayManager::DisplayManager() : m_tft(TFT_eSPI(TFT_WIDTH, TFT_HEIGHT)), // Explizit Dimensionen übergeben, um korrekte Initialisierung zu erzwingen
                                   m_timeSprite(&m_tft),
                                   m_dateSprite(&m_tft), // m_dateSprite muss hier initialisiert werden
                                   m_timeX(0), m_timeY(0), m_timeDX(1), m_timeDY(1),
                                   m_lastTimeX(0), m_lastTimeY(0),
                                   m_dateX(0), m_dateY(0), m_dateDX(-1), m_dateDY(-1),
                                   m_lastDateX(0), m_lastDateY(0),
                                   m_lastMoveTime(0),
                                   m_lastSpriteDrawTime(0),
                                   m_currentTimeStr(""),
                                   m_currentDateStr(""),
                                   m_displayState(STATE_CLOCK),    // Starte im Uhr-Zustand
                                   m_currentMenuPage(PAGE_MAIN),   // Starte auf der Haupt-Menü-Seite
                                   m_alarmScreenVisible(false),
                                   m_alarmDisplayedTime(""),
                                   m_currentRadioStationIndex(0),  // Starte mit dem ersten Sender
                                   m_radioIsPlaying(false),
                                   m_lightBrightness(128),
                                   m_selectedLightColor(CRGB(255, 140, 40))
{
    // Initialisiere Zufallsgenerator für Startpositionen
    randomSeed(analogRead(32));
}

/**
 * @brief Gibt die Singleton-Instanz des DisplayManagers zurück.
 * @return DisplayManager* Zeiger auf die Instanz.
 */
DisplayManager *DisplayManager::GetInstance()
{
    static DisplayManager instance;
    return &instance;
}

/**
 * @brief Initialisiert das Display, liest die Controller-ID aus, setzt die Rotation und zeichnet die UI.
 */
void DisplayManager::begin()
{
    LOG_INFO("Init DisplayManager");

    constexpr uint8_t backlightPwmChannel = 0;
    constexpr uint32_t backlightPwmFrequency = 5000;
    constexpr uint8_t backlightPwmResolution = 8;
    constexpr uint8_t backlightBrightness = 64;
    ledcSetup(backlightPwmChannel, backlightPwmFrequency, backlightPwmResolution);
    ledcAttachPin(TFT_LED, backlightPwmChannel);
    ledcWrite(backlightPwmChannel, backlightBrightness);
    digitalWrite(TFT_MISO, HIGH);

    // Eine kleine Verzögerung vor der Initialisierung kann bei manchen Displays helfen.
    delay(50);

    // Der Aufruf von readcommand8 funktioniert erst nach tft.init(), daher lesen wir die ID später.
    m_tft.init();

    pinMode(TFT_MISO, INPUT_PULLUP);

// uint32_t chip_id = m_tft.readcommand32(0xD3); // 0xD3 ist "Read ID4"
// LOG_PRINTF(Log::LOG_LEVEL_DEBUG, "Display-Controller-ID: 0x%X", chip_id);
// chip_id = m_tft.readcommand32(0xD3); // Erneut lesen, falls erster Versuch fehlschlägt
// LOG_PRINTF(Log::LOG_LEVEL_DEBUG, "Display-Controller-ID (2. Versuch): 0x%X", chip_id);

// --- Debug-Code zum Auslesen der rohen Touch-Werte ---
// m_tft.fillScreen(TFT_BLACK);
// m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
// m_tft.setCursor(20, 100);
// m_tft.setTextFont(2);
// m_tft.println("Bitte Bildschirm beruehren, um Raw-Daten zu lesen...");
// LOG_INFO("Warte auf Touch, um rohe Daten auszulesen...");

// // Warte auf eine stabile Berührung (Z-Wert > 60)
// uint16_t raw_x, raw_y;
// while (m_tft.getTouch(&raw_x, &raw_y) == false) { delay(10); }

// m_tft.getTouchRaw(&raw_x, &raw_y); // Lese die rohen X/Y-Werte
// LOG_PRINTF(Log::LOG_LEVEL_INFO, "Rohe Touch-Daten: X_RAW=%d, Y_RAW=%d, Z_RAW=%d", raw_x, raw_y, m_tft.getTouchRawZ());
// delay(2000); // Kurze Pause, damit man die Ausgabe lesen kann, bevor die Kalibrierung startet

// Hier die Kalibrierungsdaten von der Konsole einfügen:

// verbautes Display im Wecker
#ifndef DEBUG
    uint16_t calData[5] = {580, 2882, 215, 3627, 4};
#else
    // Daten vom losen Display
    uint16_t calData[5] = {261, 3623, 256, 3619, 1};
#endif

    m_tft.setRotation(3); // Querformat, passe dies bei Bedarf an (0-3)
    // calibrateTouch();
    m_tft.setTouch(calData);

    // --- Sprite-Größen dynamisch berechnen ---
    // Berechne die Größe für das Uhrzeit-Sprite (Font 8)
    m_tft.setTextFont(8);
    int16_t timeWidth = m_tft.textWidth("23:59"); // "HH:MM" hat immer 5 Zeichen, dies ist die max. Breite
    int16_t timeHeight = m_tft.fontHeight(8);

    // Berechne die Größe für das Datums-Sprite (Font 4)
    m_tft.setTextFont(4);
    int16_t dateWidth = m_tft.textWidth("31.12.2025"); // "DD.MM.YYYY" hat immer 10 Zeichen
    int16_t dateHeight = m_tft.fontHeight(4);

    // The clock uses only a few colors, so 8-bit storage preserves appearance and halves sprite RAM.
    m_timeSprite.setColorDepth(8);
    // Erstelle die Sprites mit den exakten Abmessungen
    m_timeSprite.createSprite(timeWidth, timeHeight);
    m_timeSprite.setTextDatum(MC_DATUM); // Text-Ankerpunkt in der Mitte

    m_dateSprite.createSprite(dateWidth, dateHeight);
    m_dateSprite.setTextDatum(MC_DATUM);

    if (m_isMoving)
    {
        initializeMovingPositions();
    }
    else
    {
        calculateNewRandomPosition(m_timeX, m_timeY, m_dateX, m_dateY);
        m_lastPositionChangeTime = millis();
        m_fadeState = FadeState::NONE;
    }
    // Initialisiere die "letzte" Position mit der Startposition, um beim ersten Frame kein Artefakt zu erzeugen
    m_lastTimeX = m_timeX;
    m_lastTimeY = m_timeY;
    m_lastDateX = m_dateX;
    m_lastDateY = m_dateY;

    drawUi(); // Zeichnet den initialen schwarzen Bildschirm (muss wieder aktiviert werden)

    static char alarmLabel[] = "Wecker";
    static char radioLabel[] = "Radio";
    static char lightLabel[] = "Licht";
    static char infoLabel[] = "Info";
    const int buttonWidth = (m_tft.width() - 36) / 2;
    const int buttonHeight = 78;
    const int leftCenter = 12 + buttonWidth / 2;
    const int rightCenter = m_tft.width() - leftCenter;
    m_alarmButton.initButton(&m_tft, leftCenter, 103, buttonWidth, buttonHeight, TFT_WHITE, TFT_RED, TFT_WHITE, alarmLabel, 2);
    m_radioButton.initButton(&m_tft, rightCenter, 103, buttonWidth, buttonHeight, TFT_WHITE, TFT_BLUE, TFT_WHITE, radioLabel, 2);
    m_lightButton.initButton(&m_tft, leftCenter, 197, buttonWidth, buttonHeight, TFT_BLACK, TFT_YELLOW, TFT_BLACK, lightLabel, 2);
    m_infoButton.initButton(&m_tft, rightCenter, 197, buttonWidth, buttonHeight, TFT_WHITE, TFT_DARKGREEN, TFT_WHITE, infoLabel, 2);
}

/**
 * @brief Verarbeitet periodische Aufgaben, wie das Abfragen von Touch-Eingaben.
 */
void DisplayManager::handle()
{
    if (AlarmManager::GetInstance()->isRinging())
    {
        const String currentTime = appContext.ntpTime.getFormattedTimeShort();
        if (!m_alarmScreenVisible || !m_alarmDisplayedTime.equals(currentTime))
        {
            m_alarmScreenVisible = true;
            drawAlarmScreen();
        }
        return;
    }

    if (m_alarmScreenVisible)
    {
        m_alarmScreenVisible = false;
        m_alarmDisplayedTime = "";
        m_lastTimeStr = "";
        m_lastDateStr = "";
        if (m_displayState == STATE_MENU)
        {
            drawMenu();
        }
        else
        {
            m_tft.fillScreen(TFT_BLACK);
            drawCurrentSprites();
        }
    }

    switch (m_displayState)
    {
    case STATE_CLOCK:
        if (m_isMoving)
        {
            handleMoving();
        }
        else
        {
            handleStaticWithRelocation();
        }
        drawCurrentSprites();
        break;
    case STATE_MENU:
        // Im Menü-Zustand passiert in der handle-Schleife nichts,
        // da das Menü statisch ist und nur auf Touch-Ereignisse reagiert.
        break;
    }
}

/**
 * @brief Behandelt die Logik für die statische Anzeige mit periodischem Positionswechsel.
 */
void DisplayManager::handleStaticWithRelocation()
{
    const unsigned long relocationInterval = 60 * 1000; // 60 Sekunden
    const unsigned long fadeDuration = 5 * 1000;        // 5 Sekunden für Fade-Out, 5 für Fade-In

    unsigned long now = millis();

    if (m_fadeState == FadeState::NONE && now - m_lastPositionChangeTime >= relocationInterval)
    {
        m_fadeState = FadeState::FADING_OUT;
        m_fadeStartTime = now;
        calculateNewRandomPosition(m_nextTimeX, m_nextTimeY, m_nextDateX, m_nextDateY);
    }

    if (m_fadeState == FadeState::FADING_OUT)
    {
        if (now - m_fadeStartTime >= fadeDuration)
        {
            // Fade-Out beendet: Lösche die alten Sprites und wechsle zum Fade-In
            // Das explizite Löschen hier ist nicht mehr nötig, da drawCurrentSprites das übernimmt,
            // wenn die Position sich ändert. Wir müssen nur die Positionen aktualisieren.
            m_lastTimeX = m_timeX;
            m_lastDateY = m_dateY;

            m_timeX = m_nextTimeX;
            m_timeY = m_nextTimeY;
            m_dateX = m_nextDateX;
            m_dateY = m_nextDateY;

            m_fadeState = FadeState::FADING_IN;
            m_fadeStartTime = now; // Setze Startzeit für Fade-In
        }
    }
    else if (m_fadeState == FadeState::FADING_IN)
    {
        if (now - m_fadeStartTime >= fadeDuration)
        {
            // Fade-In beendet
            m_fadeState = FadeState::NONE;
            m_lastPositionChangeTime = now;
        }
    }
}

/**
 * @brief Behandelt die Logik für die kontinuierlich bewegte Anzeige.
 */
void DisplayManager::handleMoving()
{
    // Nur aktualisieren und zeichnen, wenn genug Zeit für den nächsten Frame vergangen ist (~60 FPS)
    if (millis() - m_lastMoveTime < 33)
    {
        return;
    }
    m_lastMoveTime = millis();

    // Speichere die aktuellen Positionen als "letzte" für den nächsten Frame,
    // damit drawCurrentSprites() die alte Position löschen kann.
    m_lastTimeX = m_timeX;
    m_lastTimeY = m_timeY;
    m_lastDateX = m_dateX;
    m_lastDateY = m_dateY;

    // Berechne neue Positionen
    updateMovingPositions();
    // Das Zeichnen erfolgt über den Aufruf von drawCurrentSprites() in handle().
}

/**
 * @brief Zeichnet die statische Benutzeroberfläche (Hintergrund, Titel).
 */
void DisplayManager::drawUi()
{
    // Der gesamte Bildschirm wird schwarz, da die Sprites sich frei bewegen.
    m_tft.fillScreen(TFT_BLACK);
}

/**
 * @brief Aktualisiert die Zeitanzeige mithilfe eines Sprites, um Flackern zu vermeiden.
 * @param timeStr Der anzuzeigende Zeit-String.
 */
void DisplayManager::updateTime(const char *timeStr)
{
    // Speichere den neuen Zeit-String. Das eigentliche Zeichnen erfolgt in drawCurrentSprites().
    m_currentTimeStr = timeStr;
}

/**
 * @brief Aktualisiert die Datumsanzeige.
 * @param dateStr Der anzuzeigende Datums-String.
 */
void DisplayManager::updateDate(const char *dateStr)
{
    // Speichere den neuen Datums-String. Das eigentliche Zeichnen erfolgt in drawCurrentSprites().
    m_currentDateStr = dateStr;
}

/**
 * @brief Initialisiert die Positionen für eine statische, zentrierte Anzeige.
 */
void DisplayManager::initializeStaticPositions()
{
    m_timeX = (m_tft.width() - m_timeSprite.width()) / 2;
    m_timeY = (m_tft.height() / 2) - m_timeSprite.height(); // Leicht über der Mitte
    m_dateX = (m_tft.width() - m_dateSprite.width()) / 2;
    m_dateY = (m_tft.height() / 2) + 10; // Leicht unter der Mitte
}

/**
 * @brief Initialisiert die Positionen und Bewegungsvektoren für eine bewegte Anzeige.
 */
void DisplayManager::initializeMovingPositions()
{
    int timeZoneHeight = m_tft.height() * 2 / 3;
    m_timeX = random(0, m_tft.width() - m_timeSprite.width());
    m_timeY = random(0, timeZoneHeight - m_timeSprite.height());

    int dateZoneYStart = timeZoneHeight;
    int dateZoneHeight = m_tft.height() / 3;
    m_dateX = random(0, m_tft.width() - m_dateSprite.width());
    m_dateY = random(dateZoneYStart, dateZoneYStart + dateZoneHeight - m_dateSprite.height());
}

/**
 * @brief Berechnet die neuen Positionen für die bewegte Anzeige.
 */
void DisplayManager::updateMovingPositions()
{
    m_timeX += m_timeDX;
    m_timeY += m_timeDY;

    int timeZoneHeight = m_tft.height() * 2 / 3;
    if (m_timeX <= 0 || (m_timeX + m_timeSprite.width()) >= m_tft.width())
    {
        m_timeDX = -m_timeDX;
    }
    if (m_timeY <= 0 || (m_timeY + m_timeSprite.height()) >= timeZoneHeight)
    {
        m_timeDY = -m_timeDY;
    }

    m_dateX += m_dateDX;
    m_dateY += m_dateDY;

    int dateZoneYStart = timeZoneHeight;
    if (m_dateX <= 0 || (m_dateX + m_dateSprite.width()) >= m_tft.width())
    {
        m_dateDX = -m_dateDX;
    }
    if (m_dateY <= dateZoneYStart || (m_dateY + m_dateSprite.height()) >= m_tft.height())
    {
        m_dateDY = -m_dateDY;
    }
}

/**
 * @brief Berechnet eine neue zufällige, statische Position für Zeit und Datum.
 */
void DisplayManager::calculateNewRandomPosition(int16_t &outTimeX, int16_t &outTimeY, int16_t &outDateX, int16_t &outDateY)
{
    // Zone für die Uhrzeit (obere 2/3)
    int timeZoneHeight = m_tft.height() * 2 / 3;
    outTimeX = random(0, m_tft.width() - m_timeSprite.width());
    outTimeY = random(0, timeZoneHeight - m_timeSprite.height());

    // Zone für das Datum (unteres 1/3)
    int dateZoneYStart = timeZoneHeight;
    int dateZoneHeight = m_tft.height() / 3;
    outDateX = random(0, m_tft.width() - m_dateSprite.width());
    outDateY = random(dateZoneYStart, dateZoneYStart + dateZoneHeight - m_dateSprite.height());
}

/**
 * @brief Fragt die Touch-Koordinaten ab.
 * @param x Referenz für die X-Koordinate.
 * @param y Referenz für die Y-Koordinate.
 * @return true, wenn der Bildschirm berührt wird, sonst false.
 */
bool DisplayManager::getTouch(uint16_t &x, uint16_t &y)
{
    bool touched = m_tft.getTouch(&x, &y);
    // if (touched) {
    //     y = m_tft.height() - y;  // Y-Achse invertieren
    // }
    return touched;
}

/**
 * @brief Verarbeitet eine Touch-Eingabe basierend auf dem aktuellen Anzeigezustand.
 * @param x Die X-Koordinate der Berührung.
 * @param y Die Y-Koordinate der Berührung.
 */
void DisplayManager::handleTouch(uint16_t x, uint16_t y)
{
    if (AlarmManager::GetInstance()->isRinging())
    {
        if (m_snoozeButton.contains(x, y))
        {
            AlarmManager::GetInstance()->snooze();
            return;
        }
        if (m_dismissAlarmButton.contains(x, y))
        {
            AlarmManager::GetInstance()->dismiss();
            return;
        }
        const int sliderLeft = 30;
        const int sliderRight = m_tft.width() - 30;
        const int sliderY = m_tft.height() * 2 / 3;
        if (x >= sliderLeft && x <= sliderRight && abs(static_cast<int>(y) - sliderY) <= 25)
        {
            AudioManager::GetInstance()->setVolume(map(x, sliderLeft, sliderRight, 0, 100));
            drawAlarmScreen();
        }
        return;
    }

    if (m_displayState == STATE_CLOCK)
    {
        m_displayState = STATE_MENU;
        m_currentMenuPage = PAGE_MAIN;
        drawMenu();
        return;
    }

    if (m_backButton.contains(x, y))
    {
        if (m_currentMenuPage == PAGE_MAIN)
        {
            m_displayState = STATE_CLOCK;
            drawUi();
        }
        else
        {
            m_currentMenuPage = PAGE_MAIN;
            drawMenu();
        }
        return;
    }
    if (m_currentMenuPage != PAGE_MAIN && m_forwardButton.contains(x, y))
    {
        m_displayState = STATE_CLOCK;
        drawUi();
        return;
    }

    if (m_currentMenuPage == PAGE_MAIN)
    {
        if (m_alarmButton.contains(x, y)) m_currentMenuPage = PAGE_ALARM;
        else if (m_radioButton.contains(x, y)) m_currentMenuPage = PAGE_RADIO;
        else if (m_lightButton.contains(x, y)) m_currentMenuPage = PAGE_LIGHT;
        else if (m_infoButton.contains(x, y)) m_currentMenuPage = PAGE_INFO;
        else return;
        drawMenu();
        return;
    }

    if (m_currentMenuPage == PAGE_RADIO)
    {
        const auto& stations = Config::GetInstance()->GetRadioStations();
        if (stations.empty()) return;
        if (m_radioPreviousButton.contains(x, y))
        {
            m_currentRadioStationIndex = (m_currentRadioStationIndex + stations.size() - 1) % stations.size();
            if (m_radioIsPlaying) AudioManager::GetInstance()->playStream(stations[m_currentRadioStationIndex].url);
            drawMenu();
        }
        else if (m_radioNextButton.contains(x, y))
        {
            m_currentRadioStationIndex = (m_currentRadioStationIndex + 1) % stations.size();
            if (m_radioIsPlaying) AudioManager::GetInstance()->playStream(stations[m_currentRadioStationIndex].url);
            drawMenu();
        }
        else if (m_playStopButton.contains(x, y))
        {
            if (m_radioIsPlaying)
            {
                AudioManager::GetInstance()->stopStream();
                m_radioIsPlaying = false;
            }
            else
            {
                AudioManager::GetInstance()->playStream(stations[m_currentRadioStationIndex].url);
                m_radioIsPlaying = true;
            }
            drawMenu();
        }
        else if (x >= 30 && x <= m_tft.width() - 30 && abs(static_cast<int>(y) - 230) <= 22)
        {
            AudioManager::GetInstance()->setVolume(map(x, 30, m_tft.width() - 30, 0, 100));
            drawMenu();
        }
        return;
    }

    if (m_currentMenuPage == PAGE_LIGHT)
    {
        if (m_lightWarmButton.contains(x, y)) m_selectedLightColor = CRGB(255, 175, 75);
        else if (m_lightAmberButton.contains(x, y)) m_selectedLightColor = CRGB(255, 90, 10);
        else if (m_lightWhiteButton.contains(x, y)) m_selectedLightColor = CRGB(255, 235, 190);
        else if (m_lightOffButton.contains(x, y))
        {
            LightController::GetInstance()->stop();
            drawMenu();
            return;
        }
        else if (x >= 30 && x <= m_tft.width() - 30 && abs(static_cast<int>(y) - 190) <= 22)
        {
            m_lightBrightness = map(x, 30, m_tft.width() - 30, 0, 255);
        }
        else return;

        LightController::GetInstance()->setColor(m_selectedLightColor, m_lightBrightness);
        drawMenu();
    }
}

/**
 * @brief Führt die Touchscreen-Kalibrierungsroutine durch und gibt die Kalibrierungsdaten auf dem seriellen Monitor aus.
 * Diese Funktion sollte nur bei der Ersteinrichtung aufgerufen werden.
 */
void DisplayManager::calibrateTouch()
{
    uint16_t calData[5];
    uint8_t calDataOK = 0;

    m_tft.fillScreen(TFT_BLACK);
    m_tft.setCursor(20, 0);
    m_tft.setTextFont(2);
    m_tft.setTextSize(1);
    m_tft.setTextColor(TFT_WHITE, TFT_BLACK);

    LOG_INFO("Touch-Kalibrierung");
    LOG_INFO("Bitte die Kreuze beruehren");

    m_tft.calibrateTouch(calData, TFT_YELLOW, TFT_BLACK, 15);

    LOG_INFO("Kalibrierung abgeschlossen!");
    LOG_PRINTF(Log::LOG_LEVEL_INFO, "const uint16_t calData[5] = { %d, %d, %d, %d, %d };",
               calData[0], calData[1], calData[2], calData[3], calData[4]);
}

/**
 * @brief Berechnet die Farbe während einer Fading-Animation.
 * @param baseColor Die Basisfarbe als CRGB-Objekt.
 * @return Die berechnete Farbe im RGB565-Format für das Display.
 */
uint16_t DisplayManager::calculateFadedColor(CRGB baseColor)
{
    const unsigned long fadeDuration = 5 * 1000;
    unsigned long elapsedTime = millis() - m_fadeStartTime;
    uint8_t brightness;

    if (m_fadeState == FadeState::FADING_OUT)
    {
        // Helligkeit von 255 auf 0
        brightness = 255 - std::min((unsigned long)255, (elapsedTime * 255) / fadeDuration);
    }
    else // FADING_IN
    {
        // Helligkeit von 0 auf 255
        brightness = std::min((unsigned long)255, (elapsedTime * 255) / fadeDuration);
    }

    baseColor.nscale8_video(brightness);
    return m_tft.color565(baseColor.r, baseColor.g, baseColor.b);
}

/**
 * @brief Zeichnet die Zeit- und Datumssprites auf das Display.
 * Diese Methode wird häufig aufgerufen, besonders während Fading-Animationen oder Bewegungen.
 */
void DisplayManager::drawCurrentSprites()
{
    const bool animationActive = m_isMoving || m_fadeState != FadeState::NONE;
    const unsigned long now = millis();
    if (animationActive && now - m_lastSpriteDrawTime < 33)
    {
        return;
    }
    if (animationActive)
    {
        m_lastSpriteDrawTime = now;
    }

    // --- Zeit-Sprite zeichnen ---
    bool nightMode = isNightMode();

    // Definiere Tag- und Nachtfarben
    const uint16_t timeColorDay = TFT_YELLOW;
    const uint16_t timeColorNight = m_tft.color565(150, 80, 0); // Gedämpftes Orange
    const CRGB timeBaseColorDay = CRGB::Yellow;
    const CRGB timeBaseColorNight = CRGB(150, 80, 0);

    uint16_t timeColor;
    if (!m_isMoving && m_fadeState != FadeState::NONE)
    {
        // Wähle die Basisfarbe für das Fading basierend auf dem Tag/Nacht-Modus
        timeColor = calculateFadedColor(nightMode ? timeBaseColorNight : timeBaseColorDay);
    }
    else
    {
        timeColor = nightMode ? timeColorNight : timeColorDay;
    }

    // Neuzeichnen, wenn:
    // 1. Der Inhalt (Zeit-String) sich geändert hat.
    // 2. Die Position sich geändert hat.
    // 3. Eine Fading-Animation aktiv ist (da sich die Farbe kontinuierlich ändert).
    // 4. Im Moving-Modus (da sich Position und damit die Anzeige ständig ändert).
    bool timeNeedsRedraw = !m_lastTimeStr.equals(m_currentTimeStr) || m_fadeState != FadeState::NONE || m_isMoving || (m_timeX != m_lastTimeX || m_timeY != m_lastTimeY);
    // Wenn der Inhalt sich nicht geändert hat und keine Animation läuft, aber die Position sich geändert hat,
    // muss trotzdem neu gezeichnet werden, um die alte Position zu löschen und die neue zu zeichnen.
    // Die Bedingung (m_timeX != m_lastTimeX || m_timeY != m_lastTimeY) deckt dies ab.

    if (timeNeedsRedraw)
    {
        // Lösche die alte Position, wenn:
        // 1. Sich die Position geändert hat.
        // 2. Sich der Inhalt geändert hat, aber keine Fading-Animation läuft (da sonst der neue Inhalt einfach über den alten gezeichnet wird).
        // Beim Fading wird das Löschen durch das Ausblenden selbst erledigt.
        if ((m_timeX != m_lastTimeX || m_timeY != m_lastTimeY) ||
            (!m_lastTimeStr.equals(m_currentTimeStr) && m_fadeState == FadeState::NONE))
        {
            m_tft.fillRect(m_lastTimeX, m_lastTimeY, m_timeSprite.width(), m_timeSprite.height(), TFT_BLACK);
        }

        // Wähle den zu zeichnenden String basierend auf dem Fading-Zustand.
        // Beim Ausblenden wird der alte String verwendet, ansonsten der neue.
        const char *timeStringToDraw = (m_fadeState == FadeState::FADING_OUT)
                                           ? m_lastTimeStr.c_str()
                                           : m_currentTimeStr.c_str();

        m_timeSprite.fillSprite(TFT_BLACK); // Sprite-Hintergrund löschen
        m_timeSprite.setTextColor(timeColor);
        m_timeSprite.setTextFont(8); // Schriftart für das Sprite explizit setzen
        m_timeSprite.drawString(timeStringToDraw, m_timeSprite.width() / 2, m_timeSprite.height() / 2, 8);
        m_timeSprite.pushSprite(m_timeX, m_timeY, TFT_BLACK);

        // Aktualisiere den 'last' String nur, wenn der neue Inhalt tatsächlich gezeichnet wurde (d.h. nicht beim FADING_OUT).
        // Dies stellt sicher, dass der alte String während des FADING_OUT beibehalten wird.
        if (m_fadeState != FadeState::FADING_OUT)
        {
            m_lastTimeStr = m_currentTimeStr; // Aktualisiere den letzten gezeichneten String
        }
        m_lastTimeX = m_timeX; // Aktualisiere die letzte gezeichnete Position
        m_lastTimeY = m_timeY;
    }

    // --- Datum-Sprite zeichnen ---
    const uint16_t dateColorDay = TFT_CYAN;
    const uint16_t dateColorNight = m_tft.color565(80, 0, 0); // Dunkles Rot
    const CRGB dateBaseColorDay = CRGB::Cyan;
    const CRGB dateBaseColorNight = CRGB(80, 0, 0);

    uint16_t dateColor;
    if (!m_isMoving && m_fadeState != FadeState::NONE)
    {
        dateColor = calculateFadedColor(nightMode ? dateBaseColorNight : dateBaseColorDay);
    }
    else
    {
        dateColor = nightMode ? dateColorNight : dateColorDay;
    }

    bool dateNeedsRedraw = !m_lastDateStr.equals(m_currentDateStr) || m_fadeState != FadeState::NONE || m_isMoving || (m_dateX != m_lastDateX || m_dateY != m_lastDateY);

    if (dateNeedsRedraw)
    {
        if ((m_dateX != m_lastDateX || m_dateY != m_lastDateY) ||
            (!m_lastDateStr.equals(m_currentDateStr) && m_fadeState == FadeState::NONE))
        {
            m_tft.fillRect(m_lastDateX, m_lastDateY, m_dateSprite.width(), m_dateSprite.height(), TFT_BLACK);
        }

        const char *dateStringToDraw = (m_fadeState == FadeState::FADING_OUT)
                                           ? m_lastDateStr.c_str()
                                           : m_currentDateStr.c_str();

        m_dateSprite.fillSprite(TFT_BLACK);
        m_dateSprite.setTextColor(dateColor);
        m_dateSprite.setTextFont(4); // Schriftart für das Sprite explizit setzen
        m_dateSprite.drawString(dateStringToDraw, m_dateSprite.width() / 2, m_dateSprite.height() / 2, 4);
        m_dateSprite.pushSprite(m_dateX, m_dateY, TFT_BLACK);

        // Aktualisiere den 'last' String nur, wenn der neue Inhalt tatsächlich gezeichnet wurde (d.h. nicht beim FADING_OUT).
        if (m_fadeState != FadeState::FADING_OUT)
        {
            m_lastDateStr = m_currentDateStr; // Aktualisiere den letzten gezeichneten String
        }
        m_lastDateX = m_dateX; // Aktualisiere die letzte gezeichnete Position
        m_lastDateY = m_dateY;
    }
}

/**
 * @brief Prüft, ob der Nachtmodus für das Display aktiv sein soll.
 * @return true, wenn es Nacht ist, sonst false.
 */
bool DisplayManager::isNightMode() const
{
    return false;

    time_t now = appContext.ntpTime.getEpochTime();
    time_t sunrise = SunTimeManager::GetInstance()->getSunriseTime();
    time_t sunset = SunTimeManager::GetInstance()->getSunsetTime();

    // Wenn Sonnenzeiten noch nicht berechnet wurden, Nachtmodus deaktivieren.
    if (sunrise == 0 || sunset == 0)
    {
        return false;
    }

    // Definiere einen Puffer von 30 Minuten um Sonnenauf- und -untergang.
    const long bufferSeconds = 30 * 60;

    // Nachtmodus ist aktiv, wenn die aktuelle Zeit NACH (Sonnenuntergang + Puffer)
    // ODER VOR (Sonnenaufgang - Puffer) liegt.
    bool isAfterSunset = now > (sunset + bufferSeconds);
    bool isBeforeSunrise = now < (sunrise - bufferSeconds);

    // Diese Logik funktioniert auch über Mitternacht hinweg.
    return isAfterSunset || isBeforeSunrise;
}

/**
 * @brief Testet die Kommunikation mit dem Touch-Controller durch Auslesen des rohen Z-Wertes.
 */
void DisplayManager::testTouchController()
{
    LOG_INFO("Lese rohen Z-Wert vom Touch-Controller...");
    uint16_t z = m_tft.getTouchRawZ();
    LOG_PRINTF(Log::LOG_LEVEL_INFO, "Touch-Controller roher Z-Wert: %d", z);
    LOG_INFO("Wenn der Wert > 0 ist, funktioniert die Kommunikation wahrscheinlich. Bei Berührung sollte der Wert deutlich ansteigen (z.B. > 300).");
}

void DisplayManager::drawLabelledButton(TFT_eSPI_Button& button, int16_t centerX, int16_t centerY,
                                        uint16_t width, uint16_t height, uint16_t outlineColor,
                                        uint16_t fillColor, uint16_t textColor, const char* label, uint8_t font)
{
    static char emptyLabel[] = "";
    button.initButton(&m_tft, centerX, centerY, width, height, outlineColor, fillColor, textColor, emptyLabel, 1);
    button.drawButton();
    m_tft.setTextColor(textColor, fillColor);
    m_tft.drawCentreString(label, centerX, centerY - 10, font);
}

/**
 * @brief Zeichnet das Hauptmenü auf dem Display.
 */
void DisplayManager::drawMenu()
{
    m_tft.fillScreen(TFT_BLACK);
    m_tft.fillRect(0, 0, m_tft.width(), 46, TFT_DARKGREY);

    const char* title = "Lichtwecker";
    switch (m_currentMenuPage)
    {
    case PAGE_ALARM: title = "Wecker"; break;
    case PAGE_RADIO: title = "Radio"; break;
    case PAGE_LIGHT: title = "Licht"; break;
    case PAGE_INFO: title = "System"; break;
    default: break;
    }
    m_tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    m_tft.drawCentreString(title, m_tft.width() / 2, 8, 4);

    static char clockLabel[] = "Uhr";
    static char menuLabel[] = "Menue";
    static char backToClockLabel[] = "Uhr";
    const int navY = m_tft.height() - 26;
    drawLabelledButton(m_backButton, 60, navY, 104, 44, TFT_WHITE, TFT_BLUE, TFT_WHITE,
                       m_currentMenuPage == PAGE_MAIN ? clockLabel : menuLabel, 4);
    if (m_currentMenuPage != PAGE_MAIN)
    {
        drawLabelledButton(m_forwardButton, m_tft.width() - 60, navY, 104, 44,
                           TFT_WHITE, TFT_DARKGREEN, TFT_WHITE, backToClockLabel, 4);
    }

    const int tileWidth = (m_tft.width() - 36) / 2;
    const int leftCenter = 12 + tileWidth / 2;
    const int rightCenter = m_tft.width() - leftCenter;
    if (m_currentMenuPage == PAGE_MAIN)
    {
        drawLabelledButton(m_alarmButton, leftCenter, 103, tileWidth, 78, TFT_WHITE, TFT_RED, TFT_WHITE, "Wecker", 4);
        drawLabelledButton(m_radioButton, rightCenter, 103, tileWidth, 78, TFT_WHITE, TFT_BLUE, TFT_WHITE, "Radio", 4);
        drawLabelledButton(m_lightButton, leftCenter, 197, tileWidth, 78, TFT_BLACK, TFT_YELLOW, TFT_BLACK, "Licht", 4);
        drawLabelledButton(m_infoButton, rightCenter, 197, tileWidth, 78, TFT_WHITE, TFT_DARKGREEN, TFT_WHITE, "Info", 4);
    }
    else if (m_currentMenuPage == PAGE_ALARM)
    {
        drawAlarmPage();
    }
    else if (m_currentMenuPage == PAGE_RADIO)
    {
        drawRadioPage();
    }
    else if (m_currentMenuPage == PAGE_LIGHT)
    {
        drawLightPage();
    }
    else if (m_currentMenuPage == PAGE_INFO)
    {
        drawInfoPage();
    }
}

void DisplayManager::drawAlarmPage()
{
    const char* dayNames[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
    for (int index = 0; index < AlarmManager::NUM_ALARMS; ++index)
    {
        const auto& alarm = AlarmManager::GetInstance()->getAlarm(index);
        const int rowY = 53 + index * 56;
        m_tft.fillRoundRect(12, rowY, m_tft.width() - 24, 51, 5, TFT_DARKGREY);
        m_tft.drawRoundRect(12, rowY, m_tft.width() - 24, 51, 5, TFT_LIGHTGREY);

        String alarmName = "Wecker ";
        alarmName += String(index + 1);
        m_tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
        m_tft.drawString(alarmName, 22, rowY + 3, 4);

        char alarmTime[6];
        snprintf(alarmTime, sizeof(alarmTime), "%02u:%02u", alarm.hour, alarm.minute);
        m_tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
        m_tft.drawString(alarmTime, 112, rowY + 1, 4);

        String days;
        for (int day = 0; day < 7; ++day)
        {
            if (alarm.days & (1 << day))
            {
                if (!days.isEmpty()) days += " ";
                days += dayNames[day];
            }
        }
        if (days.isEmpty()) days = "Nie";
        m_tft.setTextColor(TFT_CYAN, TFT_DARKGREY);
        m_tft.drawString(days, 247, rowY + 5, 4);

        m_tft.setTextColor(alarm.enabled ? TFT_GREEN : TFT_RED, TFT_DARKGREY);
        m_tft.drawString(alarm.enabled ? "AN" : "AUS", 407, rowY + 5, 4);
        m_tft.setTextColor(TFT_LIGHTGREY, TFT_DARKGREY);
        m_tft.drawString(alarm.radioEnabled ? "Radio: Ein" : "Radio: Aus", 22, rowY + 29, 4);
        m_tft.setTextColor(alarm.sunriseEnabled ? TFT_ORANGE : TFT_LIGHTGREY, TFT_DARKGREY);
        m_tft.drawString(alarm.sunriseEnabled ? "Licht: Ein" : "Licht: Aus", 190, rowY + 29, 4);
    }
}

void DisplayManager::drawRadioPage()
{
    const auto& stations = Config::GetInstance()->GetRadioStations();
    if (stations.empty())
    {
        m_tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        m_tft.drawCentreString("Keine Sender konfiguriert", m_tft.width() / 2, 130, 2);
        return;
    }
    if (m_currentRadioStationIndex >= static_cast<int>(stations.size())) m_currentRadioStationIndex = 0;

    String stationCount = String(m_currentRadioStationIndex + 1) + " / " + String(stations.size());
    m_tft.setTextColor(TFT_CYAN, TFT_BLACK);
    m_tft.drawCentreString(stationCount, m_tft.width() / 2, 55, 2);
    m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
    m_tft.drawCentreString(stations[m_currentRadioStationIndex].name, m_tft.width() / 2, 78, 4);

    static char previousLabel[] = "<";
    static char nextLabel[] = ">";
    static char playLabel[] = "Play";
    static char stopLabel[] = "Stop";
    drawLabelledButton(m_radioPreviousButton, 70, 150, 72, 56, TFT_WHITE, TFT_DARKGREY, TFT_WHITE, previousLabel, 4);
    drawLabelledButton(m_playStopButton, m_tft.width() / 2, 150, 160, 56,
                       TFT_WHITE, m_radioIsPlaying ? TFT_RED : TFT_DARKGREEN, TFT_WHITE,
                       m_radioIsPlaying ? stopLabel : playLabel, 4);
    drawLabelledButton(m_radioNextButton, m_tft.width() - 70, 150, 72, 56, TFT_WHITE, TFT_DARKGREY, TFT_WHITE, nextLabel, 4);

    String volumeText = "Lautstaerke: ";
    volumeText += String(AudioManager::GetInstance()->getVolume());
    volumeText += "%";
    m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
    m_tft.drawCentreString(volumeText, m_tft.width() / 2, 190, 2);
    const int sliderLeft = 30;
    const int sliderWidth = m_tft.width() - 60;
    const int sliderY = 230;
    const int volume = AudioManager::GetInstance()->getVolume();
    m_tft.fillRect(sliderLeft, sliderY - 5, sliderWidth, 10, TFT_DARKGREY);
    m_tft.fillRect(sliderLeft, sliderY - 5, sliderWidth * volume / 100, 10, TFT_CYAN);
    const int thumbX = sliderLeft + sliderWidth * volume / 100;
    m_tft.fillCircle(thumbX, sliderY, 10, TFT_CYAN);
    m_tft.drawCircle(thumbX, sliderY, 10, TFT_WHITE);
}

void DisplayManager::drawLightPage()
{
    static char warmLabel[] = "Warm";
    static char amberLabel[] = "Amber";
    static char whiteLabel[] = "Weiss";
    static char offLabel[] = "Aus";
    // m_tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    // m_tft.drawCentreString("Farbton", m_tft.width() / 2, 57, 4);
    drawLabelledButton(m_lightWarmButton, 100, 105, 118, 72, TFT_WHITE, TFT_ORANGE, TFT_WHITE, warmLabel, 4);
    drawLabelledButton(m_lightAmberButton, 240, 105, 118, 72, TFT_WHITE, TFT_RED, TFT_WHITE, amberLabel, 4);
    drawLabelledButton(m_lightWhiteButton, 380, 105, 118, 72, TFT_WHITE, TFT_WHITE, TFT_BLACK, whiteLabel, 4);

    String brightnessText = "Helligkeit: ";
    brightnessText += String(static_cast<int>(m_lightBrightness) * 100 / 255);
    brightnessText += "%";
    m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
    m_tft.drawCentreString(brightnessText, m_tft.width() / 2, 158, 4);
    const int sliderLeft = 30;
    const int sliderWidth = m_tft.width() - 60;
    const int sliderY = 190;
    m_tft.fillRect(sliderLeft, sliderY - 5, sliderWidth, 10, TFT_DARKGREY);
    m_tft.fillRect(sliderLeft, sliderY - 5, sliderWidth * m_lightBrightness / 255, 10, TFT_ORANGE);
    const int thumbX = sliderLeft + sliderWidth * m_lightBrightness / 255;
    m_tft.fillCircle(thumbX, sliderY, 10, TFT_ORANGE);
    m_tft.drawCircle(thumbX, sliderY, 10, TFT_WHITE);

    drawLabelledButton(m_lightOffButton, m_tft.width() / 2, 248, 150, 42,
                       TFT_WHITE, TFT_RED, TFT_WHITE, offLabel, 4);
}

void DisplayManager::drawInfoPage()
{
    const auto drawCard = [this](const char* label, const String& value, int x, int y)
    {
        m_tft.fillRoundRect(x, y, 220, 42, 4, TFT_DARKGREY);
        m_tft.setTextFont(1);
        m_tft.setTextColor(TFT_CYAN, TFT_DARKGREY);
        m_tft.drawString(label, x + 9, y + 3, 4);
        m_tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
//        const uint8_t valueFont = value.length() <= 10 ? 4 : 2;
        const uint8_t valueFont = 4;
        m_tft.drawString(value, x + 9, y + (valueFont == 4 ? 13 : 17), valueFont);
    };

    const bool connected = WiFi.status() == WL_CONNECTED;
    const int leftX = 12;
    const int rightX = m_tft.width() - 232;
    drawCard("WLAN", connected ? "Verbunden" : "Getrennt", leftX, 55);
    drawCard("IP-Adresse", connected ? WiFi.localIP().toString() : String("--"), rightX, 55);
    drawCard("Signal", connected ? String(WiFi.RSSI()) + " dBm" : String("--"), leftX, 103);
    drawCard("Uhrzeit", appContext.ntpTime.getFormattedTimeShort(), rightX, 103);

    const auto formatSunTime = [](time_t value)
    {
        if (value <= 0) return String("--:--");
        struct tm timeInfo;
        localtime_r(&value, &timeInfo);
        char buffer[6];
        strftime(buffer, sizeof(buffer), "%H:%M", &timeInfo);
        return String(buffer);
    };
    drawCard("Sonnenaufgang", formatSunTime(SunTimeManager::GetInstance()->getSunriseTime()), leftX, 151);
    drawCard("Sonnenuntergang", formatSunTime(SunTimeManager::GetInstance()->getSunsetTime()), rightX, 151);
    drawCard("Version", Config::GetInstance()->GetVersionStringAsCharPtr(), leftX, 199);
    drawCard("Lautstaerke", String(AudioManager::GetInstance()->getVolume()) + "%", rightX, 199);
}

void DisplayManager::drawAlarmScreen()
{
    m_tft.fillScreen(TFT_WHITE);
    m_tft.setTextColor(TFT_RED, TFT_WHITE);
    m_tft.drawCentreString("WECKER", m_tft.width() / 2, 12, 4);

    const String currentTime = appContext.ntpTime.getFormattedTimeShort();
    m_tft.setTextColor(TFT_BLACK, TFT_WHITE);
    m_tft.drawCentreString(currentTime, m_tft.width() / 2, 55, 8);
    m_alarmDisplayedTime = currentTime;

    drawAlarmVolumeControl();

    static char snoozeLabel[] = "Schlummern 5 min";
    static char dismissLabel[] = "Beenden";
    const int buttonY = m_tft.height() - 45;
        drawLabelledButton(m_snoozeButton, m_tft.width() / 2 - 115, buttonY, 210, 60,
                           TFT_BLACK, TFT_YELLOW, TFT_BLACK, snoozeLabel, 4);
        drawLabelledButton(m_dismissAlarmButton, m_tft.width() / 2 + 115, buttonY, 150, 60,
                           TFT_WHITE, TFT_RED, TFT_WHITE, dismissLabel, 4);
}

void DisplayManager::drawAlarmVolumeControl()
{
    const int sliderLeft = 30;
    const int sliderWidth = m_tft.width() - 60;
    const int sliderY = m_tft.height() * 2 / 3;
    const int volume = AudioManager::GetInstance()->getVolume();

    String volumeLabel = "Lautstaerke: ";
    volumeLabel += String(volume);
    volumeLabel += "%";
    m_tft.setTextColor(TFT_BLACK, TFT_WHITE);
    m_tft.drawCentreString(volumeLabel, m_tft.width() / 2, sliderY - 42, 4);

    m_tft.fillRect(sliderLeft, sliderY - 5, sliderWidth, 10, TFT_DARKGREY);
    m_tft.fillRect(sliderLeft, sliderY - 5, sliderWidth * volume / 100, 10, TFT_RED);
    const int thumbX = sliderLeft + sliderWidth * volume / 100;
    m_tft.fillCircle(thumbX, sliderY, 10, TFT_RED);
    m_tft.drawCircle(thumbX, sliderY, 10, TFT_BLACK);
}