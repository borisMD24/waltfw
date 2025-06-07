#include "HttpServer.h"

HttpServer::HttpServer(int port) : server(port) {}

void HttpServer::begin() {
    this->server.begin();
    Serial.println("Serveur HTTP démarré sur le port 80!");
}

void HttpServer::handleClient() {
    WiFiClient client = this->server.available();
    if (client) {
        String currentLine = "";
        String header = "";
        String body = "";
        bool isPost = false;
        int contentLength = 0;
        bool headerComplete = false;
        
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                
                if (!headerComplete) {
                    header += c;
                    
                    if (c == '\n') {
                        if (currentLine.length() == 0) {
                            // Empty line indicates end of headers
                            headerComplete = true;
                            
                            // Check if it's a POST request
                            if (header.indexOf("POST") >= 0) {
                                isPost = true;
                                
                                // Extract content length
                                int contentLengthIndex = header.indexOf("Content-Length:");
                                if (contentLengthIndex >= 0) {
                                    String lengthStr = header.substring(contentLengthIndex + 15);
                                    lengthStr = lengthStr.substring(0, lengthStr.indexOf('\r'));
                                    lengthStr.trim();
                                    contentLength = lengthStr.toInt();
                                }
                            }
                            
                            if (!isPost) {
                                // Not a POST request, send simple response
                                sendResponse(client, 200, "text/plain", "OK - GET request");
                                break;
                            }
                        } else {
                            currentLine = "";
                        }
                    } else if (c != '\r') {
                        currentLine += c;
                    }
                } else {
                    // Reading body
                    body += c;
                    
                    if (body.length() >= contentLength) {
                        // We have the complete body
                        if (postHandler && body.length() > 0) {
                            currentBody = body;
                            postHandler();
                        }
                        sendResponse(client, 200, "text/plain", "OK");
                        break;
                    }
                }
            }
        }
        
        client.stop();
    }
}

void HttpServer::onPost(std::function<void()> handler) {
    postHandler = handler;
}

String HttpServer::arg(const String& name) {
    if (name == "plain") {
        return currentBody;
    }
    return "";
}

void HttpServer::send(int code, const String& content_type, const String& content) {
    // This is handled in sendResponse for the simple implementation
}

void HttpServer::sendResponse(WiFiClient& client, int code, const String& content_type, const String& content) {
    client.println("HTTP/1.1 " + String(code) + " OK");
    client.println("Content-Type: " + content_type);
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.println();
    client.println(content);
}