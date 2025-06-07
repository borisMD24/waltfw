#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H
#include <udpManager.h>
#include <ESPAsyncWebServer.h>  // ESPAsyncWebServer library
#include <AsyncWebSocket.h>     // AsyncWebSocket from ESPAsyncWebServer
#include <WebSocketsClient.h>   // For WebSocket client (outgoing connections)

class NetworkManager{
public:

    NetworkManager();
    void broadcastMessage(const String &message);
    void setupWebSocketServer();
    
    void sendMessageToClient(uint32_t clientId, const String& message);
    UDPManager udpManager;
    WebSocketsClient webSocket;  // For outgoing WebSocket connections
    AsyncWebServer asyncServer;  // Async HTTP server
    AsyncWebSocket ws;           // WebSocket server
};

#endif 