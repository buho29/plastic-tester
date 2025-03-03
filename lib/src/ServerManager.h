#ifndef SERVER_MANAGER_H
#define SERVER_MANAGER_H

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <functional>
#include <list>
#include <mbedtls/md.h> //encript

class ServerManager
{
public:

    ServerManager() : server(80), ws("/ws") {}

    // Callback para integración con otros componentes
    typedef std::function<bool(JsonObject &, AsyncWebSocketClient *)> ReceivedCallback;
    typedef std::function<void(AsyncWebSocketClient *)> ConnectedCallback;
    
    void begin(const char *wwwRoot)
    {

        // Set the WebSocket event handler
        ws.onEvent(std::bind(&ServerManager::onWsEvent, this,
                             std::placeholders::_1, std::placeholders::_2,
                             std::placeholders::_3, std::placeholders::_4,
                             std::placeholders::_5, std::placeholders::_6));

        server.addHandler(&ws);
        server.serveStatic("/", LittleFS, wwwRoot).setDefaultFile("index.html");
        server.begin();
    }

    void setUserAuth(char *user, char *pass)
    {
        www_user = user;
        www_pass = pass;
    }

    void setOnAuthSuccessDataLoad(ReceivedCallback callback)
    {
        privateCallback = callback;
    }

    void setOnDataLoad(ReceivedCallback callback)
    {
        publicCallback = callback;
    }

    void setOnConnectedClient(ConnectedCallback callback)
    {
        connectedCallback = callback;
    }

    // send client to open path (page)
    void goTo(const char* path,AsyncWebSocketClient *client = nullptr){
        sendCmd("goTo", path, client);
    }
    //		send texts to client
    //		if client = nullptr send text to all clients
    void send(const String &str, AsyncWebSocketClient *client = nullptr)
    {
        if (client != nullptr)
            client->text(str);
        else
            ws.textAll(str);
        // Serial.printf("send [%d] %s \n",str.length(),str.c_str());
    }
    //		send text all client auth
    //		ex: when the configuration has been modified,
    //		used to send the modified changes to all authenticated users.
    void sendAllAuth(const String &str)
    {
        for (AsyncWebSocketClient *client : clientsAuth)
            send(str, client);
    }

    enum typeNotify
    {
        GOOD = 0,// notify normal
        ERROR = 1,// popup error
        WARN = 2,// notify warn
        POPUP = 3// popup validation 
    };
    // send notifications to user, if client = nullptr send all users
    // if type = 0
    // type = 1 notify error (popup)
    // type = 2 
    void sendMessage(typeNotify type, const String &msg,
                     AsyncWebSocketClient *client = nullptr)
    {
        JsonDocument doc;

        JsonObject o = doc["message"].to<JsonObject>();

        o["type"] = type;
        o["content"] = msg;

        String json;
        serializeJson(doc, json);

        send(json, client);

        Serial.println(json);
    }
    // send short commands to client
    void sendCmd(const char *tag, const char *msg,
                 AsyncWebSocketClient *client = nullptr)
    {
        String json = String("{\"") + tag + "\":\"" + msg + "\"}";
        send(json, client);
    }
    // open a popup window in client
    void sendPopup(String title, String msg, String cmd,
                   AsyncWebSocketClient *client)
    {
        sendMessage(POPUP, title + "|" + msg + "|" + cmd, client);
    }

    void sendSystem(AsyncWebSocketClient *client)
    {
        send(createJsonSystem(), client);
    }

    void update()
    {
        ws.cleanupClients();
    }
private:
    AsyncWebServer server;
    AsyncWebSocket ws;
    std::list<AsyncWebSocketClient *> clientsAuth;
    char *www_user;
    char *www_pass;

    ConnectedCallback connectedCallback;
    ReceivedCallback publicCallback;
    ReceivedCallback privateCallback;

    void receivedJson(AsyncWebSocketClient *client, const String &json)
    {
        Serial.println(json);
        JsonDocument doc;

        // Parse JSON
        DeserializationError error = deserializeJson(doc, json);
        if (error)
        {
            Serial.printf("Error deserializing receivedJson: %s\n", error.f_str());
            sendMessage(ERROR, "Error deserializing receivedJson", client);
            return;
        }

        JsonObject root = doc.as<JsonObject>();

        if (publicCallback && publicCallback(root, client))
            return;

        // Handle login request
        if (root["login"].is<JsonObject>())
        {
            JsonObject user = root["login"];
            if (user["name"].is<const char *>() && user["pass"].is<const char *>())
            {
                const char *name = user["name"];
                const char *pass = user["pass"];

                // Serial.printf("loggin user %s pass%S", name, pass);

                if (isValideAuth(name, pass))
                {
                    String token = createToken(client->remoteIP());
                    sendCmd("token", token.c_str(), client);
                    // go home
                    goTo( "/", client);

                    sendMessage(GOOD, String("Welcome ") + name, client);
                    return;
                }
                else
                {
                    sendMessage(ERROR, "Error Login", client);
                    return;
                }
            }
        }

        // Check token authentication
        // comprobamos el token si es valido
        bool auth = false;
        if (root["token"].is<const char *>())
        {
            const char *token = root["token"];

            auth = isAuthenticate(token, client->remoteIP());
        }

        // si no estamos identificado ir a login
        if (!auth)
        {
            // llevamos al cliente a la pagina de login
            goTo("/login",client);
            sendMessage(ERROR, "Need to login", client);
            // no seguimos
            return;
        }

        saveClientAuth(client);

        if (privateCallback)
            privateCallback(root, client);
    }

    void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                   AwsEventType type, void *arg, uint8_t *data, size_t len)
    {
        if (type == WS_EVT_CONNECT)
        {
            Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
            if (connectedCallback)
                connectedCallback(client);
        }
        else if (type == WS_EVT_DISCONNECT)
        {
            removeClientAuth(client);
            Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
        }
        else if (type == WS_EVT_ERROR)
        {
            Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
        }
        else if (type == WS_EVT_PONG)
        {
            Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
        }
        else if (type == WS_EVT_DATA)
        {
            AwsFrameInfo *info = (AwsFrameInfo *)arg;
            String msg = "";
            if (info->final && info->index == 0 && info->len == len)
            {
                // the whole message is in a single frame and we got all of it's data
                // Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);
                if (info->opcode == WS_TEXT)
                {
                    for (size_t i = 0; i < info->len; i++)
                    {
                        msg += (char)data[i];
                    }
                }
                receivedJson(client, msg);
            }
        }
    }

    //		credentials
    String sha1(const String &msg)
    {
        const char *payload = msg.c_str();
        const int size = 20;

        byte shaResult[size];

        mbedtls_md_context_t ctx;
        mbedtls_md_type_t md_type = MBEDTLS_MD_SHA1;

        const size_t payloadLength = strlen(payload);

        mbedtls_md_init(&ctx);
        mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
        mbedtls_md_starts(&ctx);
        mbedtls_md_update(&ctx, (const unsigned char *)payload, payloadLength);
        mbedtls_md_finish(&ctx, shaResult);
        mbedtls_md_free(&ctx);

        String hashStr = "";

        for (uint16_t i = 0; i < size; i++)
        {
            String hex = String(shaResult[i], HEX);
            if (hex.length() < 2)
            {
                hex = "0" + hex;
            }
            hashStr += hex;
        }

        return hashStr;
    }
    // create a unique key for client
    String createToken(const IPAddress &ip)
    {
        return sha1("jardin:" +
                    String(www_user) + ":" +
                    String(www_pass) + ":" +
                    ip.toString());
    }
    // 		check auth from token
    bool isAuthenticate(const String &token, const IPAddress &ip)
    {
        return token == createToken(ip);
    }
    //		check auth from user/pass
    bool isValideAuth(const char *user, const char *pass)
    {
        return strcmp(www_user, user) == 0 &&
               strcmp(www_pass, pass) == 0;
    }

    //		manage authenticated users
    void saveClientAuth(AsyncWebSocketClient *client)
    {
        auto it = std::find(clientsAuth.begin(), clientsAuth.end(), client);
        if (it == clientsAuth.end())
        {
            clientsAuth.push_back(client);
        }
        Serial.printf("clientAuth save size%d\n", clientsAuth.size());
    }
    void removeClientAuth(AsyncWebSocketClient *client)
    {
        // buscamos si esta en la lista de logeados si es asi se borra
        auto it = std::find(clientsAuth.begin(), clientsAuth.end(), client);
        if (it != clientsAuth.end())
        {
            clientsAuth.erase(it);
        }

        Serial.printf("clientAuth remove size%d\n", clientsAuth.size());
    }

    //create json info system
    String getStatusWifi()
    {
        switch (WiFi.status())
        {
        case WL_CONNECTED:
            return "Connected";
        case WL_NO_SSID_AVAIL:
            return "SSID not available.";
        case WL_CONNECT_FAILED:
            return "Connection failed";
        case WL_CONNECTION_LOST:
            return "Connection lost";
        case WL_DISCONNECTED:
            return "Disconnected";
        }
        return "Unknown status.";
    }
    String createJsonSystem()
    {
        uint32_t c = millis();
        JsonDocument root;
        String str;

        JsonObject doc = root["system"].to<JsonObject>();

        JsonObject wifi = doc[" WIFI"].to<JsonObject>();

        wifi["status"] = getStatusWifi();
        wifi["ip"] = WiFi.localIP().toString();
        wifi["softApIp"] = WiFi.softAPIP().toString();
        wifi["gateway"] = WiFi.gatewayIP().toString();
        wifi["signalStrengh"] = -WiFi.RSSI();

        // Obtener información del heap
        size_t freeHeap = ESP.getFreeHeap();
        size_t maxAllocHeap = ESP.getMaxAllocHeap();
        size_t heapSize = ESP.getHeapSize();

        JsonObject heap = doc["HEAP"].to<JsonObject>();

        heap["size"] = String(heapSize / 1024) + "Kb";
        heap["used"] = String((heapSize - freeHeap) / 1024) + "Kb";
        heap["free"] = String(freeHeap / 1024) + "Kb";
        heap["maxAllocHeap"] = String(maxAllocHeap / 1024) + "Kb";
        heap["minFree"] = String(ESP.getMinFreeHeap() / 1024) + "Kb";
        // Calcular la fragmentación del heap
        float heapFrag = 100.0 - (100.0 * maxAllocHeap / freeHeap);
        heap["fragmentation"] = String(heapFrag) + "%";

        JsonObject flash = doc["FLASH"].to<JsonObject>();
        flash["size"] = String(ESP.getFreeSketchSpace() / 1024) + "Kb";
        flash["used"] = String(ESP.getSketchSize() / 1024) + "Kb";

        JsonObject lfs = doc["LittleFS"].to<JsonObject>();
        lfs["size"] = String(LittleFS.totalBytes() / 1024) + "Kb";
        lfs["used"] = String(LittleFS.usedBytes() / 1024) + "Kb";
        // Información adicional del sistema
        JsonObject info = doc["INFO"].to<JsonObject>();
        info["temperature"] = String(temperatureRead()) + "°C";
        info["uptime"] = String(millis() / 1000) + "s";

        serializeJsonPretty(root, str);

        Serial.printf("createJsonSystem %d ms\n", millis() - c);
        // Serial.println(str.c_str());
        return str;
    }

};

/*

ServerManager server;
char www_user[32] = "admin";
char www_pass[64] = "admin";

void clientConnected(AsyncWebSocketClient* client){
    ;
}

bool clientLoadPublic(JsonObject& request, AsyncWebSocketClient* client){
    bool exit = false ;

    return exit;
}
bool clientLoadPrivate(JsonObject& request, AsyncWebSocketClient* client){
    bool exit = false ;

    return exit;
}

void setup() {
    // Configuración inicial...

    server.setConnectedCallback(clientConnected);
    server.setOnDataLoad(clientLoadPublic);
    server.setOnAuthSuccessDataLoad(clientLoadPrivate);
    server.setUserAuth(www_user,www_pass);

    server.begin("/www/");
}

void loop() {
    server.update();
    // Resto del código...
}*/

#endif // SERVER_MANAGER_H