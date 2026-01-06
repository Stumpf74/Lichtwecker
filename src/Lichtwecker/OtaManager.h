#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

// Die Klasse ist nun eine reguläre Klasse, kein Singleton mehr.
class OtaManager
{
public:
    /**
     * @brief Konstruktor für den OtaManager.
     */
    OtaManager();
    ~OtaManager() = default;

    /**
     * @brief Initialisiert die Over-The-Air (OTA) Update-Funktionalität.
     */
    void begin();
    /**
     * @brief Verarbeitet eingehende OTA-Anfragen. Muss in der Hauptschleife aufgerufen werden.
     */
    void handle();
    /**
     * @brief Prüft, ob gerade ein OTA-Update aktiv ist.
     * @return true, wenn ein Update läuft, sonst false.
     */
    bool isOtaActive() const;

private:
    OtaManager(const OtaManager&) = delete;
    OtaManager& operator=(const OtaManager&) = delete;

    bool m_isOtaActive;
};

#endif // OTA_MANAGER_H
