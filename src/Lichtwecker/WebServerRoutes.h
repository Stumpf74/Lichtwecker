#ifndef WEBSERVER_ROUTES_H
#define WEBSERVER_ROUTES_H

#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>

void setupWebServerRoutes(AsyncWebServer& server, AsyncEventSource& events, AsyncWebSocket& ws);

#endif // WEBSERVER_ROUTES_H