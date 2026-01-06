#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>

/**
 * @class InputManager
 * @brief Verwaltet Benutzereingaben, wie z.B. den Touch-Button.
 *
 * Diese Klasse ist als Singleton implementiert und kümmert sich um das
 * Entprellen und die Verarbeitung von Touch-Eingaben.
 */
class InputManager
{
public:
    static InputManager* GetInstance();

    void begin();
    void handle();

private:
    InputManager();
    ~InputManager() = default;
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    // --- Touch-Button Konfiguration ---
    static const int TOUCH_PIN = T3;       // GPIO 15 ist T3
    static const int TOUCH_THRESHOLD = 40; // Schwellenwert, muss evtl. angepasst werden
    static const unsigned long DEBOUNCE_TIME = 500; // 500ms Entprellzeit

    unsigned long m_lastTouchTime;
    unsigned long m_nextCheckTime;
};

#endif // INPUT_MANAGER_H