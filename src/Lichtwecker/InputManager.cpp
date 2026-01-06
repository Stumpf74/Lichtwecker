#include "InputManager.h"
#include "Log.h"
#include "DisplayManager.h" // Zugriff auf das Display für Touch-Abfragen
#include "LightController.h"

/**
 * @brief Konstruktor für den InputManager.
 */
InputManager::InputManager() : 
    m_lastTouchTime(0),
    m_nextCheckTime(0)
{
}

/**
 * @brief Gibt die Singleton-Instanz des InputManagers zurück.
 */
InputManager* InputManager::GetInstance()
{
    static InputManager instance;
    return &instance;
}

/**
 * @brief Initialisiert den InputManager.
 */
void InputManager::begin()
{
    LOG_INFO("Init InputManager");
    // touchRead benötigt kein explizites pinMode.
    // Initialisiere die Zeitvariablen.
    m_lastTouchTime = 0;
    m_nextCheckTime = millis();
}

/**
 * @brief Verarbeitet periodisch die Eingaben.
 * Diese Methode sollte in der Hauptschleife (loop) aufgerufen werden.
 */
void InputManager::handle()
{
    if (millis() >= m_nextCheckTime)
    {
        m_nextCheckTime = millis() + 200; // Prüfintervall kann etwas länger sein

        uint16_t t_x, t_y = 0; // Variablen für die Touch-Koordinaten
        // Prüfe, ob der Bildschirm berührt wird und eine Entprellzeit vergangen ist
        if (DisplayManager::GetInstance()->getTouch(t_x, t_y) && (millis() - m_lastTouchTime > DEBOUNCE_TIME))
        {
            m_lastTouchTime = millis();
            LOG_PRINTF(Log::LOG_LEVEL_DEBUG, "Bildschirm berührt bei X=%d, Y=%d. Leite an DisplayManager weiter.", t_x, t_y);
            DisplayManager::GetInstance()->handleTouch(t_x, t_y);
        }
    }
}