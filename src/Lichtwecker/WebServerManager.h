#ifndef WEBSERVER_MANAGER_H
#define WEBSERVER_MANAGER_H

#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>

class WebServerManager
{
public:
    static WebServerManager* GetInstance();

    void begin();

    // Diese Member sind öffentlich, damit setupWebServerRoutes darauf zugreifen kann.
    // Eine Alternative wäre, setupWebServerRoutes zu einer friend-Funktion zu machen.
    AsyncWebServer server;
    AsyncEventSource events;
    AsyncWebSocket ws;

private:
    WebServerManager();
    ~WebServerManager() = default;
    WebServerManager(const WebServerManager&) = delete;
    WebServerManager& operator=(const WebServerManager&) = delete;

    /**
     * @brief Handler für nicht gefundene Webseiten (404).
     * @param request Der eingehende Web-Request.
     */
    static void notFound(AsyncWebServerRequest *request);
};

#endif // WEBSERVER_MANAGER_H