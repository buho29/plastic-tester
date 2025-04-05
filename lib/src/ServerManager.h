/**
 * @file ServerManager.h
 * @brief Manages the server and WebSocket connections for the plastic tester platform.
 */

#ifndef SERVER_MANAGER_H
#define SERVER_MANAGER_H
// #define CONFIG_ASYNC_TCP_RUNNING_CORE 0
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <functional>
#include <list>
#include <mbedtls/md.h> //encript

/**
 * @class ServerManager
 * @brief Web server manager class for ESP32 with WebSocket support and authentication
 *
 * This class provides a web server implementation with the following features:
 * - Static file serving from LittleFS
 * - WebSocket communication
 * - User authentication system
 * - Public and private API endpoints
 * - System status monitoring
 * - Client notification system
 *
 * Key features:
 * - Serves static files from a specified www root directory
 * - Handles WebSocket connections and events
 * - Implements user authentication with username/password
 * - Generates and validates authentication tokens
 * - Manages authenticated client sessions
 * - Provides system status information (WiFi, heap, flash, etc)
 * - Supports different types of notifications (good, error, warning, popup)
 *
 * Usage example:
 * @code
 * ServerManager server;
 * server.setUserAuth("admin", "password");
 * server.begin("/www/");
 * @endcode
 *
 * The server supports callback registration for:
 * - Client connections
 * - Public data requests
 * - Private (authenticated) data requests
 *
 * @note Requires AsyncTCP, ESPAsyncWebServer, LittleFS, and ArduinoJson libraries
 * @note Default port is 80
 */
class ServerManager
{
public:
    /**
     * @brief Constructor for ServerManager.
     */
    ServerManager() : server(80), ws("/ws") {}

    // Callback for integration with other components
    typedef std::function<bool(JsonObject &, AsyncWebSocketClient *)> ReceivedCallback;
    typedef std::function<void(AsyncWebSocketClient *)> ConnectedCallback;

    /**
     * @brief Initializes the server with the given root directory.
     * @param wwwRoot The root directory for the server.
     */
    void begin(const char *wwwRoot)
    {
        using namespace std::placeholders;

        // Set the WebSocket event handler
        ws.onEvent(std::bind(&ServerManager::onWsEvent, this, _1, _2, _3, _4, _5, _6));
        server.addHandler(&ws);

        server.serveStatic("/", LittleFS, wwwRoot).setDefaultFile("index.html");
        server.on("/file", HTTP_ANY, std::bind(&ServerManager::onFilePage, this, _1),
                  std::bind(&ServerManager::onUploadFile, this, _1, _2, _3, _4, _5, _6));
        server.begin();

        updateJsonFile();
    }

    /**
     * @brief Updates the cached JSON file.
     */
    void updateJsonFile()
    {
        jsonFilesCached = printJsonFiles();
    }


    /**
     * @brief Sets the user authentication credentials.
     * @param user The username.
     * @param pass The password.
     */
    void setUserAuth(char *user, char *pass)
    {
        www_user = user;
        www_pass = pass;
    }

    /**
     * @brief Sets the callback for successful authentication data load.
     * @param callback The callback function.
     */
    void setOnAuthSuccessDataLoad(ReceivedCallback callback)
    {
        privateCallback = callback;
    }

    /**
     * @brief Sets the callback for data load.
     * @param callback The callback function.
     */
    void setOnDataLoad(ReceivedCallback callback)
    {
        publicCallback = callback;
    }

    /**
     * @brief Sets the callback for when a client connects.
     * @param callback The callback function.
     */
    void setOnConnectedClient(ConnectedCallback callback)
    {
        connectedCallback = callback;
    }

    /**
     * @brief Sends a command to the client to open a specific path (page).
     * @param path The path to open.
     * @param client The client to send the command to. If nullptr, sends to all clients.
     */
    void goTo(const char *path, AsyncWebSocketClient *client = nullptr)
    {
        sendCmd("goTo", path, client);
    }

    /**
     * @brief Sends a text message to the client.
     * @param str The text message to send.
     * @param client The client to send the message to. If nullptr, sends to all clients.
     */
    void send(const String &str, AsyncWebSocketClient *client = nullptr)
    {
        if (client != nullptr)
            client->text(str);
        else
            ws.textAll(str);
        // Serial.printf("send [%d] %s \n",str.length(),str.c_str());
    }

    /**
     * @brief Sends a text message to all authenticated clients.
     * @param str The text message to send.
     */
    void sendAllAuth(const String &str)
    {
        for (AsyncWebSocketClient *client : clientsAuth)
            send(str, client);
    }

    /**
     * @enum typeNotify
     * @brief Types of notifications.
     */
    enum typeNotify
    {
        GOOD = 0,  ///< Normal notification
        ERROR = 1, ///< Error popup
        WARN = 2,  ///< Warning notification
        POPUP = 3  ///< Validation popup
    };

    /**
     * @brief Sends a notification message to the client.
     * @param type The type of notification.
     * @param msg The notification message.
     * @param client The client to send the message to. If nullptr, sends to all clients.
     */
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

    /**
     * @brief Sends a command to the client.
     * @param tag The command tag.
     * @param msg The command message.
     * @param client The client to send the command to. If nullptr, sends to all clients.
     */
    void sendCmd(const char *tag, const char *msg,
                 AsyncWebSocketClient *client = nullptr)
    {
        String json = String("{\"") + tag + "\":\"" + msg + "\"}";
        send(json, client);
    }

    /**
     * @brief Sends a popup message to the client.
     * @param title The title of the popup.
     * @param msg The message of the popup.
     * @param cmd The command associated with the popup.
     * @param client The client to send the popup to.
     */
    void sendPopup(String title, String msg, String cmd,
                   AsyncWebSocketClient *client)
    {
        sendMessage(POPUP, title + "|" + msg + "|" + cmd, client);
    }

    /**
     * @brief Sends system information to the client.
     * @param client The client to send the information to.
     */
    void sendSystem(AsyncWebSocketClient *client)
    {
        send(createJsonSystem(), client);
    }

    /**
     * @brief Sends cached JSON files directory to the client.
     * @param client The client to send the information to.
     */
    void sendJsonFiles(AsyncWebSocketClient *client)
    {
        send(jsonFilesCached, client);
    }

    /**
     * @brief Updates the WebSocket clients.
     */
    void update()
    {
        ws.cleanupClients();
    }

private:
    AsyncWebServer server;                         ///< The server instance.
    AsyncWebSocket ws;                             ///< The WebSocket instance.
    std::list<AsyncWebSocketClient *> clientsAuth; ///< List of authenticated clients.
    char *www_user;                                ///< The username for authentication.
    char *www_pass;                                ///< The password for authentication.

    ConnectedCallback connectedCallback; ///< Callback for client connection.
    ReceivedCallback publicCallback;     ///< Callback for public data load.
    ReceivedCallback privateCallback;    ///< Callback for private data load.

    String jsonFilesCached;

    /**
     * @brief Handles received JSON messages from the client.
     * @param client The client that sent the message.
     * @param json The JSON message.
     */
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
                    goTo("/", client);

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
            goTo("/login", client);
            sendMessage(ERROR, "Need to login", client);
            // no seguimos
            return;
        }

        saveClientAuth(client);

        if (privateCallback)
            privateCallback(root, client);
    }

    /**
     * @brief Handles WebSocket events.
     * @param server The WebSocket server.
     * @param client The WebSocket client.
     * @param type The type of WebSocket event.
     * @param arg Additional arguments for the event.
     * @param data The data associated with the event.
     * @param len The length of the data.
     */
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

    /**
     * @brief Generates a SHA-1 hash of the given message.
     * @param msg The message to hash.
     * @return The SHA-1 hash of the message.
     */
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

    /**
     * @brief Creates a unique token for the client based on its IP address.
     * @param ip The IP address of the client.
     * @return The generated token.
     */
    String createToken(const IPAddress &ip)
    {
        return sha1("jardin:" +
                    String(www_user) + ":" +
                    String(www_pass) + ":" +
                    ip.toString());
    }

    /**
     * @brief Checks if the token is valid for the given IP address.
     * @param token The token to check.
     * @param ip The IP address of the client.
     * @return True if the token is valid, false otherwise.
     */
    bool isAuthenticate(const String &token, const IPAddress &ip)
    {
        return token == createToken(ip);
    }

    bool isAuthenticate(AsyncWebServerRequest *request)
    {
        if (request->hasHeader("Authorization"))
        {
            String authStr = request->header("Authorization");
            // remove "Basic "
            String token = authStr.substring(6);
            return isAuthenticate(token, request->client()->remoteIP());
        }
        Serial.println("not head");
        return false;
    }

    /**
     * @brief Checks if the provided username and password are valid.
     * @param user The username.
     * @param pass The password.
     * @return True if the credentials are valid, false otherwise.
     */
    bool isValideAuth(const char *user, const char *pass)
    {
        return strcmp(www_user, user) == 0 &&
               strcmp(www_pass, pass) == 0;
    }

    /**
     * @brief Saves the authenticated client to the list of authenticated clients.
     * @param client The client to save.
     */
    void saveClientAuth(AsyncWebSocketClient *client)
    {
        auto it = std::find(clientsAuth.begin(), clientsAuth.end(), client);
        if (it == clientsAuth.end())
        {
            clientsAuth.push_back(client);
        }
        Serial.printf("clientAuth save size%d\n", clientsAuth.size());
    }

    /**
     * @brief Removes the client from the list of authenticated clients.
     * @param client The client to remove.
     */
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

    /**
     * @brief Gets the WiFi status as a string.
     * @return The WiFi status.
     */
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

    // File
    void onUploadFile(AsyncWebServerRequest *request, const String &filename,
                      size_t index, uint8_t *data, size_t len, bool final)
    {
        static bool authenticate = false;

        String path = "/data";
        if (request->hasArg("path"))
        {
            path = request->arg("path");
        }

        if (!index)
        {

            if (isAuthenticate(request))
            {

                String p = path + "/" + filename;
                Serial.printf("UploadStart: %s\n", p.c_str());
                request->_tempFile = LittleFS.open(p, "w");
                authenticate = true;
            }
            else
                authenticate = false;
        }

        // Serial.println((request->_tempFile ? "siiii" : "nçooooo"));

        if (authenticate && request->_tempFile)
        {
            if (len)
            {
                request->_tempFile.write(data, len);
            }
            if (final)
            {
                request->_tempFile.close();
                authenticate = false;
                Serial.printf(" existe %d", LittleFS.exists(filename));
            }
        }

        for (size_t i = 0; i < len; i++)
        {
            Serial.write(data[i]);
        }

        if (final)
            Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index + len);
    }

    void listDir(const char *dirname, const JsonArray &rootjson, uint8_t levels)
    {
        // Serial.printf("Listing %s \n", dirname);

        //uint32_t c = millis();

        File root = LittleFS.open(dirname, "r");
        if (!root)
        {
            Serial.printf("- failed to open directory %s\n", dirname);
            return;
        }
        if (!root.isDirectory())
        {
            Serial.println(" - not a directory");
            return;
        }

        String path = String(dirname);

        JsonObject obj = rootjson.add<JsonObject>();

        obj["path"] = path;
        JsonArray dir = obj["files"].to<JsonArray>();

        File file = root.openNextFile();
        while (file)
        {
            if (file.isDirectory())
            {
                if (levels)
                {

                    String newPath = path;
                    if (!newPath.endsWith("/"))
                        newPath += "/";
                    newPath += file.name();

                    // Llamada recursiva con el array hijo
                    listDir(newPath.c_str(), rootjson, levels - 1);
                }
            }
            else
            {
                JsonObject obj = dir.add<JsonObject>();
                obj["name"] = String(file.name());
                obj["size"] = file.size();
            }
            file = root.openNextFile();
        }
        //Serial.printf("%dms\n", millis() - c);

        root.close();
    }

    String printJsonFiles()
    {
        JsonDocument doc;
        uint32_t c = millis();

        const JsonObject root = doc.to<JsonObject>();

        const JsonArray files = root["root"].to<JsonArray>();

        const JsonObject www = files.add<JsonObject>();
        www["path"] = "/www/";
        const JsonArray wwwFolders = www["folders"].to<JsonArray>();
        listDir("/www/", wwwFolders, 3);

        const JsonObject &data = files.add<JsonObject>();
        data["path"] = "/data/";
        const JsonArray &dataFolders = data["folders"].to<JsonArray>();
        listDir("/data/", dataFolders, 3);

        String json;
        serializeJson(root, json);

        Serial.printf("printJsonFiles total %d ms\n", millis() - c);

        return json;
    }

    // fix cors errors
    void sendResponse(AsyncWebServerRequest *request, AsyncWebServerResponse *response)
    {
        // limit max requests
        response->addHeader("Connection", "Keep-Alive");
        response->addHeader("Keep-Alive", "max=2");
        // fix cors errors
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Access-Control-Allow-Headers", "Authorization, Content-Type"); 
        request->send(response);
    }

    //		file
    void onFilePage(AsyncWebServerRequest *request)
    {
        if (request->method() == HTTP_OPTIONS)
        {
            
            AsyncWebServerResponse *response = request->beginResponse(200);
            response->addHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
            response->addHeader("Access-Control-Allow-Headers", "Authorization, Content-Type");
            sendResponse(request, response);
            Serial.println("HTTP_OPTIONS");
            return;
        }

        if (!isAuthenticate(request))
        {
            // no authorized bye bye
            Serial.println("bye bye");
            sendResponse(request, request->beginResponse(401));
            return;
        }

        if (request->method() == HTTP_GET)
        {
            if (request->hasArg("download"))
            {

                String filename = request->arg("download");
                Serial.println("Download Filename: " + filename);

                if (LittleFS.exists(filename))
                    sendResponse(request, request->beginResponse(LittleFS, filename, String(), true));
                else
                    sendResponse(request, request->beginResponse(404));
            }
            else if (request->hasArg("delete"))
            {

                String filename = request->arg("delete");
                Serial.println("delete Filename: " + filename);

                if (LittleFS.exists(filename) && LittleFS.remove(filename))
                {
                    sendResponse(request, request->beginResponse(200));
                    updateJsonFile();
                    sendAllAuth(jsonFilesCached);
                }
                else
                    sendResponse(request, request->beginResponse(404));
            }
        }
        // upload
        else if (request->method() == HTTP_POST)
        {
            String path = "/data";
            if (request->hasArg("path"))
                path = request->arg("path");

            if (request->hasParam("file", true, true) &&
                LittleFS.exists(path + "/" + request->getParam("file", true, true)->value()))
            {

                sendResponse(request, request->beginResponse(200)); // ok
                updateJsonFile();
                sendAllAuth(jsonFilesCached);
            }
            else
                sendResponse(request, request->beginResponse(404)); // error
        }
    }

    // end File

    /**
     * @brief Creates a JSON string with system information.
     * @return The JSON string with system information.
     */
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
        info["uptime"] = formatUptime(millis());

        serializeJsonPretty(root, str);

        Serial.printf("createJsonSystem %d ms\n", millis() - c);
        // Serial.println(str.c_str());
        return str;
    }
    String formatUptime(uint32_t milliseconds)
    {
        uint32_t total_seconds = milliseconds / 1000;

        uint32_t hours = total_seconds / 3600;
        uint32_t remaining = total_seconds % 3600;
        uint32_t minutes = remaining / 60;
        uint32_t seconds = remaining % 60;

        char buffer[12]; // Suficiente para "XXX:XX:XX"
        snprintf(buffer, sizeof(buffer),
                 "%02lu:%02lu:%02lu",
                 (unsigned long)hours,
                 (unsigned long)minutes,
                 (unsigned long)seconds);

        return String(buffer);
    }
};
#endif // SERVER_MANAGER_H