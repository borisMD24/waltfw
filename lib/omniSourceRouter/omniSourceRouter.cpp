#include <omniSourceRouter.h>
#include <algorithm> // Pour std::remove et std::erase
#include <ArduinoJson.h>
#include <functional>

#include <vector>
#include <networkManager.h>

// Static pointer to instance for callback
OmniSourceRouter *OmniSourceRouter::instance = nullptr;

OmniSourceRouter::OmniSourceRouter(NetworkManager *nm)
{
    // Set static instance for callbacks
    instance = this;
    this->nm = nm;
    // Subscribe to UDP messages
    // this->udpManager.subscribe([this](const String &message)
    //                            { this->inspectBody(message); });

    // Initialize WebSocket client (if still needed for outgoing connections)
    // Setup HTTP server routes and WebSocket server
}

void OmniSourceRouter::begin(){
    this->setupHttpRoutes();
    this->setupWebSocketServer();
    nm->webSocket.begin("192.168.1.18", 3000, "/cable");
    nm->webSocket.onEvent(OmniSourceRouter::webSocketEventStatic);
    nm->webSocket.setReconnectInterval(5000);
 
}

OmniSourceRouter::~OmniSourceRouter()
{
    // Cleanup resources
    nm->ws.cleanupClients();
}

void OmniSourceRouter::setupWebSocketServer()
{
    // Setup WebSocket server event handlers
    nm->ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
               { this->onWebSocketEvent(server, client, type, arg, data, len); });

    // Add WebSocket handler to server
    nm->asyncServer.addHandler(&nm->ws);
}

void OmniSourceRouter::onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->final && info->index == 0 && info->len == len)
        {
            // The whole message is in a single frame and we got all of it's data
            if (info->opcode == WS_TEXT)
            {
                String message = "";
                for (size_t i = 0; i < len; i++)
                {
                    message += (char)data[i];
                }
                
                // Parse the incoming WebSocket message
                DynamicJsonDocument doc(1024);
                DeserializationError error = deserializeJson(doc, message);
                
                if (!error && doc.is<JsonObject>()) {
                    JsonObject json = doc.as<JsonObject>();
                    this->inspectBody(json);
                }
            }
        }
    }
    break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        break;
    }
}

void OmniSourceRouter::addSource(const std::string &sourceName)
{
    sources.push_back(sourceName);
}

void OmniSourceRouter::removeSource(const std::string &sourceName)
{
    // Utilisation de std::remove pour supprimer une source et std::erase pour ajuster le vecteur
    sources.erase(std::remove(sources.begin(), sources.end(), sourceName), sources.end());
}

void OmniSourceRouter::inspectBody(JsonObject body)
{
    if (body.containsKey("target"))
    {
        String target = body["target"].as<String>();

        // Find callback for this target
        OmniSourceRouterCallback* callback = findCallback(target);
        if (callback) {
            
            // Execute with cooldown management
            bool executed = cooldownManager.executeWithCooldown(*callback, body);
            if (!executed) {
            }
        }
    }
}

void OmniSourceRouter::setupHttpRoutes()
{
    // Enable CORS for all responses
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    // Handle OPTIONS requests for CORS preflight
    nm->asyncServer.on("/*", HTTP_OPTIONS, [](AsyncWebServerRequest *request)
                   { request->send(200); });

    // Setup POST handler for all routes
    nm->asyncServer.onRequestBody([this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
                              {
    if (request->method() == HTTP_POST) {
        // Reconstituer le corps en String
        String body = "";
        for (size_t i = 0; i < len; i++) {
            body += (char)data[i];
        }

        // Parser le JSON
        DynamicJsonDocument doc(1024); // Ajuste la taille si t'as des payloads velus
        DeserializationError error = deserializeJson(doc, body);

        if (error) {
            Serial.print("Erreur JSON : ");
            request->send(400, "application/json", "{\"error\":\"invalid json\"}");
            return;
        }

        if (!doc.is<JsonObject>()) {
            request->send(400, "application/json", "{\"error\":\"expected JSON object\"}");
            return;
        }

        JsonObject json = doc.as<JsonObject>();
        this->inspectBody(json); // Appel classe

        request->send(200, "application/json", "{\"status\":\"ok\"}");
    } });

    // Handle POST requests completion
    nm->asyncServer.on("/*", HTTP_POST, [](AsyncWebServerRequest *request)
                   { request->send(200, "application/json", "{\"status\":\"received\"}"); });

    // Handle GET requests (optional - for testing)
    nm->asyncServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                   { request->send(200, "text/plain", "OmniSourceRouter Server Running"); });

    // Handle WebSocket upgrade requests
    nm->asyncServer.on("/ws", HTTP_GET, [](AsyncWebServerRequest *request)
                   { request->send(200, "text/plain", "WebSocket endpoint"); });

    // Handle not found
    nm->asyncServer.onNotFound([](AsyncWebServerRequest *request)
                           { request->send(404, "text/plain", "Not found"); });
}

void OmniSourceRouter::webSocketEventStatic(WStype_t type, uint8_t *payload, size_t length)
{
    if (instance)
    {
        instance->webSocketEvent(type, payload, length);
    }
}

void OmniSourceRouter::webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        break;
    case WStype_CONNECTED:
        Serial.printf("WebSocket Client Connected to: %s\n", payload);
        nm->webSocket.sendTXT("{\"action\":\"join\", \"room\":\"leds\"}");
        break;
    case WStype_TEXT:
    {
        String message = String((char *)payload);
        message = message.substring(0, length); // Assure la bonne longueur du message

        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, message);

        if (!error)
        {
            // Vérifie si la clé "message" existe
            if (doc.containsKey("message") && doc["message"].is<JsonObject>())
            {
                JsonObject messageBody = doc["message"].as<JsonObject>();
                this->inspectBody(messageBody); // ici on envoie le vrai JSON objet
            }
        }
    }
    break;
    case WStype_ERROR:
        break;
    default:
        break;
    }
}

void OmniSourceRouter::handle()
{
    if (!this->httpStarted)
    {
        nm->asyncServer.begin();
        this->httpStarted = true;
    }
    //this->udpManager.processUDP();
    nm->webSocket.loop(); // For WebSocket client
    
    // Process pending cooldown calls
    this->update();
    
    // Note: AsyncWebServer handles requests automatically, no need to call handleClient()
}

void OmniSourceRouter::routeData(const std::string &data)
{
    for (const auto &source : sources)
    {
        // Exemple de traitement des données pour chaque source
    }
}


// Original addCallback method (struct-based)
void OmniSourceRouter::addCallback(OmniSourceRouterCallback cb){
    this->routerCallbacks.push_back(cb);
}

// New addCallback method with individual parameters
void OmniSourceRouter::addCallback(const String& target, std::function<void(JsonObject&)> callback, unsigned long cooldownMs) {
    OmniSourceRouterCallback cb(target, callback, cooldownMs);
    this->routerCallbacks.push_back(cb);
    
    // Set cooldown in manager
    cooldownManager.setCooldown(target, cooldownMs);
    
    Serial.printf("✅ Callback ajouté pour target : %s (cooldown: %lu ms)\n", target.c_str(), cooldownMs);
}

void OmniSourceRouter::delCallback(String target) {
    auto it = std::remove_if(
        this->routerCallbacks.begin(),
        this->routerCallbacks.end(),
        [&target](const OmniSourceRouterCallback& cb) {
            return cb.target == target;
        });

    if (it != this->routerCallbacks.end()) {
        this->routerCallbacks.erase(it, this->routerCallbacks.end());
        Serial.printf("🗑️ Callback supprimé pour target : %s\n", target.c_str());
    } else {
        Serial.printf("🚫 Aucun callback trouvé pour target : %s\n", target.c_str());
    }
}

// Update method to process pending calls (should be called in main loop)
void OmniSourceRouter::update() {
    cooldownManager.processPendingCalls(routerCallbacks);
}

// Get remaining cooldown time for a target
unsigned long OmniSourceRouter::getRemainingCooldown(const String& target) {
    return cooldownManager.getRemainingCooldown(target);
}

// Check if target has a pending call
bool OmniSourceRouter::hasPendingCall(const String& target) {
    return cooldownManager.hasPendingCall(target);
}