#include "WebServerManager.h"
#include "WebServerRoutes.h"
#include "Log.h"

WebServerManager::WebServerManager() :
    server(80),
    events("/events"),
    ws("/ws")
{
}

WebServerManager* WebServerManager::GetInstance()
{
    static WebServerManager instance;
    return &instance;
}

void WebServerManager::begin()
{
    LOG_INFO("Init WebServerManager");
    setupWebServerRoutes(server, events, ws);
    server.onNotFound(notFound);
    server.begin();
}

void WebServerManager::notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}