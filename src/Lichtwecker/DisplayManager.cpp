#include <algorithm> // Für std::min
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
                                   m_lastMoveTime(0), // Initialisiere mit leeren Strings
                                   m_currentTimeStr(""),
                                   m_currentDateStr(""),
                                   m_displayState(STATE_CLOCK) // Starte im Uhr-Zustand
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

    pinMode(TFT_LED, OUTPUT);
    digitalWrite(TFT_LED, HIGH);
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

    // calibrateTouch();

    // Hier die Kalibrierungsdaten von der Konsole einfügen:
    uint16_t calData[5] = { 580, 2882, 215, 3627, 4 };
    m_tft.setTouch(calData);

    m_tft.setRotation(1); // Querformat, passe dies bei Bedarf an (0-3)

    // --- Sprite-Größen dynamisch berechnen ---
    // Berechne die Größe für das Uhrzeit-Sprite (Font 8)
    m_tft.setTextFont(8);
    int16_t timeWidth = m_tft.textWidth("23:59"); // "HH:MM" hat immer 5 Zeichen, dies ist die max. Breite
    int16_t timeHeight = m_tft.fontHeight(8);

    // Berechne die Größe für das Datums-Sprite (Font 4)
    m_tft.setTextFont(4);
    int16_t dateWidth = m_tft.textWidth("31.12.2025"); // "DD.MM.YYYY" hat immer 10 Zeichen
    int16_t dateHeight = m_tft.fontHeight(4);

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

    // --- Initialisiere die Menü-Buttons am unteren Rand ---
    uint16_t btn_w = 100;
    uint16_t btn_h = 40;
    uint16_t btn_y = m_tft.height() - (btn_h / 2) - 10; // 10px vom unteren Rand
    // "Zurück"-Button links
    m_backButton.initButton(&m_tft, (btn_w / 2) + 20, btn_y, btn_w, btn_h, TFT_WHITE, TFT_BLUE, TFT_WHITE, "Zurueck", 1);
    // "Vor"-Button rechts
    m_forwardButton.initButton(&m_tft, m_tft.width() - (btn_w / 2) - 20, btn_y, btn_w, btn_h, TFT_WHITE, TFT_DARKGREEN, TFT_WHITE, "Vor", 1);
}

/**
 * @brief Verarbeitet periodische Aufgaben, wie das Abfragen von Touch-Eingaben.
 */
void DisplayManager::handle()
{
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

    // Prüfe, ob sich der Inhalt geändert hat. m_lastTimeStr/m_lastDateStr wird in drawCurrentSprites aktualisiert
    // und repräsentiert den zuletzt gezeichneten Zustand.
    bool contentHasChanged = !m_lastTimeStr.equals(m_currentTimeStr) || !m_lastDateStr.equals(m_currentDateStr);

    // Starte den Positionswechsel, wenn das Intervall abgelaufen ist ODER sich der Inhalt geändert hat, und keine Animation läuft.
    if (m_fadeState == FadeState::NONE && (now - m_lastPositionChangeTime >= relocationInterval || contentHasChanged))
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
    if (millis() - m_lastMoveTime < 16)
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
    return m_tft.getTouch(&x, &y);
}

/**
 * @brief Verarbeitet eine Touch-Eingabe basierend auf dem aktuellen Anzeigezustand.
 * @param x Die X-Koordinate der Berührung.
 * @param y Die Y-Koordinate der Berührung.
 */
void DisplayManager::handleTouch(uint16_t x, uint16_t y)
{
    switch (m_displayState)
    {
        case STATE_CLOCK:
            // Bei jeder Berührung im Uhr-Modus: Wechsle zum Menü
            m_displayState = STATE_MENU;
            drawMenu();
            break;

        case STATE_MENU:
            // Prüfe, ob der "Zurück"-Button gedrückt wurde
            if (m_backButton.contains(x, y))
            {
                m_backButton.press(true); // Visuelles Feedback
                m_backButton.drawButton();
                delay(100); // Kurze Verzögerung für das visuelle Feedback
                m_backButton.press(false);

                // Wechsle zurück zum Uhr-Modus
                m_displayState = STATE_CLOCK;
                drawUi(); // Zeichnet den schwarzen Hintergrund neu
                // Die Uhr wird im nächsten handle()-Durchlauf automatisch neu gezeichnet
            }
            // Prüfe, ob der "Vor"-Button gedrückt wurde
            else if (m_forwardButton.contains(x, y))
            {
                m_forwardButton.press(true);
                m_forwardButton.drawButton();
                delay(100);
                m_forwardButton.press(false);
                LOG_INFO("Vor-Button gedrueckt. (Noch keine Funktion)");
            }
            break;
    }
}


/**
 * @brief Führt die Touchscreen-Kalibrierung durch und gibt die Kalibrierungsdaten auf dem seriellen Monitor aus.
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
        const char* timeStringToDraw = (m_fadeState == FadeState::FADING_OUT) 
                                     ? m_lastTimeStr.c_str() 
                                     : m_currentTimeStr.c_str();

        m_timeSprite.fillSprite(TFT_BLACK); // Sprite-Hintergrund löschen
        m_timeSprite.setTextColor(timeColor);
        m_timeSprite.setTextFont(8); // Schriftart für das Sprite explizit setzen
        m_timeSprite.drawString(timeStringToDraw, m_timeSprite.width() / 2, m_timeSprite.height() / 2, 8);
        m_timeSprite.pushSprite(m_timeX, m_timeY, TFT_BLACK);

        // Aktualisiere den 'last' String nur, wenn der neue Inhalt tatsächlich gezeichnet wurde (d.h. nicht beim FADING_OUT).
        // Dies stellt sicher, dass der alte String während des FADING_OUT beibehalten wird.
        if (m_fadeState != FadeState::FADING_OUT) {
            m_lastTimeStr = m_currentTimeStr; // Aktualisiere den letzten gezeichneten String
        }
        m_lastTimeX = m_timeX;            // Aktualisiere die letzte gezeichnete Position
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

        const char* dateStringToDraw = (m_fadeState == FadeState::FADING_OUT)
                                     ? m_lastDateStr.c_str()
                                     : m_currentDateStr.c_str();

        m_dateSprite.fillSprite(TFT_BLACK);
        m_dateSprite.setTextColor(dateColor);
        m_dateSprite.setTextFont(4); // Schriftart für das Sprite explizit setzen
        m_dateSprite.drawString(dateStringToDraw, m_dateSprite.width() / 2, m_dateSprite.height() / 2, 4);
        m_dateSprite.pushSprite(m_dateX, m_dateY, TFT_BLACK);

        // Aktualisiere den 'last' String nur, wenn der neue Inhalt tatsächlich gezeichnet wurde (d.h. nicht beim FADING_OUT).
        if (m_fadeState != FadeState::FADING_OUT) {
            m_lastDateStr = m_currentDateStr; // Aktualisiere den letzten gezeichneten String
        }
        m_lastDateX = m_dateX;            // Aktualisiere die letzte gezeichnete Position
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

/**
 * @brief Zeichnet das Hauptmenü auf dem Display.
 */
void DisplayManager::drawMenu()
{
    m_tft.fillScreen(TFT_BLACK);
    m_backButton.drawButton();
    m_forwardButton.drawButton();
}