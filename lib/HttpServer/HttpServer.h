#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <functional>

class HttpServer {
public:
    HttpServer(int port = 80);
    void begin();
    void handleClient();
    void onPost(std::function<void()> handler);
    String arg(const String& name);
    void send(int code, const String& content_type, const String& content);

    AsyncWebServer server;
private:
    std::function<void()> postHandler;
    String currentBody;
    
    void sendResponse(WiFiClient& client, int code, const String& content_type, const String& content);
};

#endif // HTTPSERVER_H