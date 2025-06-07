#include "networkManager.h"
NetworkManager::NetworkManager(): asyncServer(80), ws("/ws"){

}
// Utility method to send message to all WebSocket clients
void NetworkManager::broadcastMessage(const String &message)
{
   ws.textAll(message);
}

// Utility method to send message to specific WebSocket client
void NetworkManager::sendMessageToClient(uint32_t clientId, const String &message)
{
    ws.text(clientId, message);
}
