/*
 Name:		tester
 Created:	23/01/2025 12:49:23
 Author:	buho29
*/

#include <mbedtls/md.h>//encript 

#include <FS.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <datatable.h>
#include "data.h"

//		WebServer variables
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dnsServer;
IPAddress apIP(8, 8, 4, 4); // The default android DNS
//		list authenticated users
std::list <AsyncWebSocketClient*> clientsAuth;
//		host name to mDNS, http://tester.local
const char * hostName = "Tester";
//		permanent app variables, see data.h
//		to save the variables in a json file
Config config;
//		current sencor
SensorItem currentSensor;

DataArray<10,ResultItem> history;
DataList<100,SensorItem> lastResult;

bool lock = false;
bool resetWifi = false;

//		manage wifi
void beginWifi()
{
	Serial.printf("begin wifi\n");

	WiFi.mode(WIFI_AP_STA);
	WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
	WiFi.softAP(hostName);

	Serial.printf("softIP Address: %s\n",WiFi.softAPIP().toString());

	// if DNSServer is started with "*" for domain name, it will reply with
	// provided IP to all DNS request
	dnsServer.start(53, "*", WiFi.softAPIP());
	
	if (MDNS.begin(hostName)) { // Verifica si la inicialización fue exitosa
		if (!MDNS.addService("http", "tcp", 80)) {
			Serial.println("Error al añadir el servicio http");
		} 
	} else {
		Serial.println("Error inicializando mDNS"); // Imprime un mensaje de error
	}
	
}
void startWifi()
{
	Serial.printf("start wifi\n");
	
	WiFi.begin(config.wifi_ssid, config.wifi_pass);
	
	uint32_t current_time = millis();
	// probamos conectarnos durante 20s
	while (millis() - current_time < 20000) 
	{
		if (WiFi.status() == WL_CONNECTED) 
		{
			Serial.println("WiFi connected");
			Serial.printf("IP Address: %s time: %dms\n",
				WiFi.localIP().toString(),
				millis() - current_time);
			return;
		}
		delay(500);
	}
	WiFi.disconnect(false);
	Serial.printf("wifi: timeout!\n");
}
void restartWifi()
{
	WiFi.disconnect(true);
	delay(1000);
	resetWifi = false;
	startWifi();
}
//		manage files 
String readFile(const char * path)
{
	//Serial.printf("reading file: %s\r\n", path);
	File file = LittleFS.open(path);
	String str = "";

	if (!file || file.isDirectory()) {
		Serial.printf("readJson- failed to open %s for reading\n", path);
		return str;
	}

	while (file.available())
	{
		str += (char)file.read();
	}
	return str;
}
bool writeFile(const char * path, const char * text)
{
	//Serial.printf("path : %s\n",path);
	uint tim = millis();
	File file = LittleFS.open(path, FILE_WRITE);
	if (!file) {
		Serial.printf("- failed to open file for writing path : %s\n",
			path);
		return false;
	}
	if (file.print(text)) {
		uint elapsed = (millis() - tim);
		file.close();
		Serial.printf("- file written in %dms :%s len:%d\n", 
			elapsed, path, strlen(text));
		return true;
	}
	Serial.println("- write failed");
	return false;
}
//		read json data
bool readJson(const char * path, Iserializable * data)
{
	const String & str = readFile(path);
	if (str.length() > 0)
		return data->deserializeData(str);
	return false;
}
bool readJson(const char * path, Item * item)
{
	JsonDocument doc;
	const String & str = readFile(path);

	if (str.length() > 0) 
	{
		// Parse
		DeserializationError error = deserializeJson(doc, str);
		if(!error)
		{
			JsonObject obj = doc.as<JsonObject>();
			return item->deserializeItem(obj);			
		}else Serial.println(error.f_str());
	}
	Serial.printf("error reading file %s\n",path);
	return false;
}
//		write json data
bool writeJson(const char * path, Iserializable * data)
{
	const String & json = data->serializeString();
	return writeFile(path, json.c_str());
}
bool writeJson(const char * path, Item * item)
{
	JsonDocument doc;
	JsonObject obj = doc.to<JsonObject>();
	String str;

	item->serializeItem(obj,false);
	serializeJsonPretty(obj, str);
	return writeFile(path, str.c_str());
}

//		send texts to client
//		if client = nullptr send text to all clients 
void send(const String& str, AsyncWebSocketClient* client = nullptr)
{
	if(client != nullptr) client->text(str);
	else ws.textAll(str);
	//Serial.printf("send [%d] %s \n",str.length(),str.c_str());
}
//		send text all client auth
//		ex: when the configuration has been modified, 
//		used to send the modified changes to all authenticated users.
void sendAllAuth(const String & str)
{
	for(AsyncWebSocketClient * client : clientsAuth)
		send(str,client);
}
// send notifications to user, if client = nullptr send all users
// if type = 0 notify normal
// type = 1 notify error (popup)
// type = 2 notify warn 
void sendMessage(uint8_t type, const String & msg, 
	AsyncWebSocketClient * client = nullptr)
{
	JsonDocument doc;

	JsonObject o = doc["message"].to<JsonObject>();

	o["type"] = type;
	o["content"] = msg;

	String json;
	serializeJson(doc, json);

	send(json,client);

	Serial.println(json);
}
// send short commands to client
void sendCmd(const char* tag,const char* msg, 
	AsyncWebSocketClient * client = nullptr)
{
	String json = String("{\"")+tag+"\":\"" + msg + "\"}";
	send(json,client);
}

//		crypto
String sha1(const String & msg)
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

	for (uint16_t i = 0; i < size; i++) {
		String hex = String(shaResult[i], HEX);
		if (hex.length() < 2) {
			hex = "0" + hex;
		}
		hashStr += hex;
	}

	return hashStr;
}
// create a unique key for client
String createToken(const IPAddress & ip)
{
	return sha1("jardin:" + 
		String(config.www_user) + ":" +
		String(config.www_pass) + ":" +
		ip.toString()
	);
}
// 		check auth from token
bool isAuthenticate(const String & token, const IPAddress & ip)
{
	return token == createToken(ip);
}
//		check auth from user/pass
bool isValideAuth(const char * user, const char * pass)
{
	return strcmp(config.www_user, user) == 0 &&
		strcmp(config.www_pass, pass) == 0;
}

//		manage authenticated users 
void saveClientAuth(AsyncWebSocketClient * client)
{
	auto it = std::find(clientsAuth.begin(), clientsAuth.end(), client);
	if (it == clientsAuth.end()) {
		clientsAuth.push_back(client);
	}
	Serial.printf("clientAuth save size%d\n", clientsAuth.size());
}
void removeClientAuth(AsyncWebSocketClient * client)
{
	//buscamos si esta en la lista de logeados si es asi se borra
	auto it = std::find(clientsAuth.begin(), clientsAuth.end(), client);
	if (it != clientsAuth.end()) {
		clientsAuth.erase(it);
	}

	Serial.printf("clientAuth remove size%d\n", clientsAuth.size());
}

// 			APP
//		print config json
void printJsonConfig()
{
	JsonDocument doc; String str;
	JsonObject obj = doc.to<JsonObject>();

	config.serializeItem(obj,false);
	serializeJsonPretty(doc, str);
	Serial.println(str.c_str());
}
//		print history json
void printJsonHistory()
{
	String str = history.serializeString();
	Serial.println(str.c_str());
}
//		return String in json format
String createJsonSystem()
{
	uint32_t c = millis();
	JsonDocument root;String str;

	JsonObject doc = root["system"].to<JsonObject>();
	
	doc["ip"] = WiFi.localIP().toString();
	doc["softApIp"] = WiFi.softAPIP().toString();
	doc["gateway"] = WiFi.gatewayIP().toString();
	doc["signalStrengh"] = -WiFi.RSSI();

	doc["freeHeap"] = String(ESP.getFreeHeap()/1024)+"Kb";
	doc["heapSize"] = String(ESP.getHeapSize()/1024)+"Kb";

	doc["flashSize"] = String(ESP.getFreeSketchSpace()/1024)+"Kb";
	doc["flashUsed"] = String(ESP.getSketchSize()/1024)+"Kb";
	
	doc["totalSpace"] = String(LittleFS.totalBytes()/1024)+"Kb";
	doc["usedSpace"] = String(LittleFS.usedBytes()/1024)+"Kb";
	
	serializeJsonPretty(root, str);
	Serial.printf("createJsonOption %d ms\n", millis() - c);
	Serial.println(str.c_str());
	return str; 
}
//		return String in json format
String createJsonOption(){
	//qquint32_t c = millis();
	JsonDocument root; String str;

	JsonObject doc = root["config"].to<JsonObject>();
	config.serializeItem(doc,true);

	serializeJsonPretty(root, str);
	//Serial.printf("createJsonOption %d ms\n", millis() - c);
	//Serial.println(str.c_str());
	return str; 
}
//		return String in json format  
String createJsonSensors()
{
	currentSensor.set(
		random(10000)/100.0,
		random(5000)/100.0,0);
	//{"ss":{"d":77.10,"f":46.49}}
	
	return String("{\"ss\":{\"d\":") + 
		currentSensor.distance + ",\"f\":" + 
		currentSensor.force + "}}";
}
//		TODO
String createJsonHistory()
{
	JsonDocument root; String json;
	uint32_t c = millis();
	
	JsonArray doc = root["hystory"].to<JsonArray>();
	history.serializeData(doc);
	
	serializeJsonPretty(root, json);
	
	Serial.printf("printJsonFiles total %d ms\n", millis() - c);

	return json;
}

String createJsonResults(JsonArray &array)
{
	static DataList<10,SensorItem> result;

	uint32_t c = millis();
	JsonDocument root; String json;

	JsonArray doc = root["results"].to<JsonArray>();

	for(JsonVariant v : array) {
		uint index = v.as<uint>();
		ResultItem* item = history[index];
		if(item){
			String path = String("/data")+item->pathData;
			
			JsonObject obj = doc.add<JsonObject>();
			obj["name"] = item->name;
			obj["date"] = item->date;
			obj["description"] = item->description;

			result.clear();
			if (readJson(path.c_str(), &result))
			{
				JsonArray arr = obj["data"].to<JsonArray>();
				result.serializeData(arr);
			}
			//Serial.printf("[%d] path:%s\n",index , path.c_str());			
		}
	
	}
	
	serializeJson(root, json);//Pretty
	
	Serial.printf("createJsonResults total %d ms\n", millis() - c);
	//Serial.println(json);

	return json;
}

String createJsonLastResult()
{
	static DataList<10,SensorItem> result;

	uint32_t c = millis();
	JsonDocument root; String json;

	JsonArray doc = root["lastResult"].to<JsonArray>();
	lastResult.serializeData(doc);
	
	serializeJson(root, json);//Pretty
	
	//Serial.printf("createJsonResults total %d ms\n", millis() - c);
	Serial.println(json);

	return json;
}

void runTest(){
	lastResult.clear();
	for (uid_t i = 0; i < 10; i++)
	{
		SensorItem* item = lastResult.getEmpty();
		if (item) {
			float d = random(100)/100.0;
			float f = random(5000)/100.0;
			item->set(d, f,i);
			lastResult.push(item);
		}
	}
	
}

// primera contacto 
void sendHistory(AsyncWebSocketClient * client)
{
	uint32_t c = millis();
	JsonDocument root; String json;
	
	JsonArray doc = root["history"].to<JsonArray>();
	history.serializeData(doc);

	serializeJsonPretty(root, json);
	send(json,client);
	
	Serial.printf("sendHistory total %d ms\n", millis() - c);
}

// manage data results
void deleteResult(uint8_t index, AsyncWebSocketClient * client)
{
	ResultItem* item = history[index];
	if(item){
		String file = String("/data")+item->pathData;
		
		if(history.remove(item) &&
			LittleFS.exists(file) && 
			LittleFS.remove(file)
		){
			if(writeJson("/data/results.json", &history))
			{
				sendMessage(0, "result deleted",client);
				sendHistory(nullptr);// send all udpdate
			}
			else sendMessage(1, "error write history file",client);
		}else sendMessage(1, "error result not found file",client);
	}else sendMessage(1, "error deleted not found result",client);
}

void saveResult(JsonObject &obj, AsyncWebSocketClient * client)
{
	if (obj["id"].is<uint8_t>() && obj["desc"].is<const char*>())
	{
		uint8_t index = obj["id"];
		ResultItem* item = history[index];
		if(item){
			strcpy(item->description,obj["desc"]);
			if(writeJson("/data/results.json", &history)){
				sendMessage(0, item->name+String(" saved"),client);
				sendHistory(nullptr);// send all udpdate;
			}else sendMessage(1, "error write history file",client);
		}else sendMessage(1, "error result not found file",client);
	}else sendMessage(1, "error result not saved",client);
}

void newResult(JsonObject &obj , AsyncWebSocketClient * client)
{
	ResultItem* item = history.getEmpty();

	if(!item){
		sendMessage(1, String("Only ")+
			history.maxSize+" items can be saved, please delete some",client);
	}else if(lastResult.size() < 1){
		sendMessage(2, " last result not found",client);
	}else if (
		obj["name"].is<const char*>() && 
		obj["desc"].is<const char*>() && 
		obj["date"].is<const char*>()
	){
		const char * name = obj["name"];
		const char * date = obj["date"];
		const char * desc = obj["desc"];

		String p = String("/result/") + name + ".json";
		String file = "/data" + p;
		const char * path = p.c_str();

		if(LittleFS.exists(file)){
			sendMessage(1, "the name already exists choose another",client);
		} 
		else if(item->isValide(path,name,date,desc))
		{
			if(writeJson(file.c_str(), &lastResult))
			{
				item->set( path,name,date,desc);
				history.push(item);
				if(writeJson("/data/results.json", &history))
				{
					String path = String("/result/") + name;
					sendCmd("goTo", path.c_str() ,client);
					sendMessage(0, name + String(" created!"),client);
					//clear result in client
					lastResult.clear();
					send(createJsonLastResult(),client);
					// send all update;
					sendHistory(nullptr);					
				} else sendMessage(1, "error write history file",client);
			}else sendMessage(1, "error write result file",client);
		} else sendMessage(1, "error input data not is valid",client);
	}
}

bool lock = false;
bool resetWifi = false;
//		received json string from user
void receivedJson(AsyncWebSocketClient* client, const String & json)
{
	lock = true;
	Serial.println(json);
	JsonDocument doc;
	// Parse json
	DeserializationError error = deserializeJson(doc, json);
	if(error) {
		Serial.printf("error deserialize receivedJson %s\n",
			error.f_str());
		sendMessage(1,"error deserialize receivedJson",client);
		return;
	}

	JsonObject root = doc.as<JsonObject>();

	// un cliente quiere acceder a datos publicos
	if(root["load"].is<JsonObject>())
	{
		JsonObject obj = root["load"];
		
		//"{"load":{"paths":["/result/pla.json","/result/abs.json","/result/petg.json"]}}
		if(obj["indexes"].is<JsonArray>())
		{
			JsonArray j = obj["indexes"].as<JsonArray>();
			send(createJsonResults(j),client);
		}
		//exit
		return;
	}

	// un cliente intenta conectarse user/pass
	if (root["login"].is<JsonObject>()) {
		JsonObject user = root["login"];
		if (user["name"].is<const char*>() && user["pass"].is<const char*>()) {
			const char * name = user["name"];
			const char * pass = user["pass"];

			Serial.printf("loggin user %s pass%S", name, pass);

			if (isValideAuth(name, pass)) 
			{
				String token = createToken(client->remoteIP());
				sendCmd("token", token.c_str(),client);
				//go home
				sendCmd("goTo", "/", client);

				sendMessage(0, String("Welcome ")+name, client);
				return;
			}else {
				sendMessage(1, "Error Login", client);
				return;
			}
		}
	}
	
	// comprobamos el token si es valido
	bool auth = false;
	if (root["token"].is<const char*>()) {
		const char * token = root["token"];
		
		auth = isAuthenticate(token, client->remoteIP());
	}

	// si no estamos identificado ir a login
	if (!auth) {
		// llevamos al cliente a la pagina de login
		sendCmd("goTo", "/login");
		sendMessage(1, "Need to login", client);
		//no seguimos
		return;
	}

	saveClientAuth(client);

	// un usuario auth quiere acceder a las datos restringidos
	if (root["loadAuth"].is<int8_t>()) 
	{
		int8_t page = root["loadAuth"];

		switch (page)
		{
		case 0:
			send(createJsonOption(),client);break;
		case 1:
			send(createJsonSystem(),client);break;
		default:
			removeClientAuth(client);break;
		}
	}
	
	// comandos para manejar el Esp32
	if (root["cmd"].is<JsonObject>()) 
	{
		JsonObject o = root["cmd"].as<JsonObject>();

		if (o["stop"].is<uint8_t>()) 
		{
			sendMessage(0, "Stop emergency!!");
		} 
		else if (o["move"].is<JsonObject>()) 
		{
			JsonObject m = o["move"].as<JsonObject>();
			if (m["dist"].is<float>() && m["dir"].is<int8_t>())
			{ 
				float dist = m["dist"]; int8_t dir = m["dir"];
				
				char buffer[60]; 
				sprintf(buffer,"moving %.3fmm in %d direction\n", dist,dir);
				sendMessage(0, String(buffer));
			}
		} 
		else if (o["run"].is<uint>()) 
		{
			uint8_t step = o["run"];
			if(step == 1) {
				runTest(),
				send(createJsonLastResult(),client);
				sendCmd("goTo", "/result/n",client);
				sendMessage(0, "Test finish!!",client);
			}
			Serial.printf("run step= %d\n",step);
		} 
		else if (o["sethome"].is<uint>()) 
		{
			sendMessage(0, "saved sethome position");
		} 
		else if (o["restar"].is<uint>()) 
		{
			sendMessage(1, "Rebooting Esp32");
			ESP.restart();
		} 
		else if (o["delete"].is<uint8_t>()) 
		{	
			uint8_t id = o["delete"];
			deleteResult(id,client);
		} 
		else if (o["new"].is<JsonObject>()) 
		{	
			JsonObject m = o["new"].as<JsonObject>();
			newResult(m,client);
		}
		else if (o["save"].is<JsonObject>()) 
		{	
			JsonObject m = o["save"].as<JsonObject>();
			saveResult(m,client);
		} 
		
	}

	// un cliente intenta modificar config
	bool modified = false;
	if (root["config"].is<JsonObject>()) {
		JsonObject con = root["config"];

		if (con["wifi_ssid"].is<const char*>() && con["wifi_pass"].is<const char*>())
		{
			if(config.setWifi(con["wifi_ssid"], con["wifi_pass"]))
			{
				modified = true;
				resetWifi = true;
			}
		}
		if (con["www_user"].is<const char*>() && con["www_pass"].is<const char*>())
		{
			if(config.setAdmin(con["www_user"],con["www_pass"])){
				modified = true;
				sendCmd("logout", "1");			
			}
		}
		//		motor and speed
		if (con["speed"].is<float>() && con["acc_desc"].is<float>()) 
		{
			config.setSpeed(con["speed"],con["acc_desc"]);
			modified = true;
			//TODO actualizar speed
		}
		if (con["screw_pitch"].is<float>() && con["micro_step"].is<uint8_t>()) 
		{
			config.setMotor(con["screw_pitch"],con["micro_step"]);
			modified = true;
			//TODO actualizar motor
		}

		if (modified) {
			if(writeJson("/data/config.json", &config)){
				sendMessage(0, "Options edited");
				// enviar cambiaos a todos los clientes auth
				sendAllAuth(createJsonOption());				
			}else sendMessage(1, "error write config file",client);
		}
	}
	lock = false;
}

//		event websocket 
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
	if (type == WS_EVT_CONNECT) {
		Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
		sendHistory(client);
	}
	else if (type == WS_EVT_DISCONNECT) {
		removeClientAuth(client);
		Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
	}
	else if (type == WS_EVT_ERROR) {
		Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
	}
	else if (type == WS_EVT_PONG) {
		Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char*)data : "");
	}
	else if (type == WS_EVT_DATA) {
		AwsFrameInfo * info = (AwsFrameInfo*)arg;
		String msg = "";
		if (info->final && info->index == 0 && info->len == len) {
			//the whole message is in a single frame and we got all of it's data
			//Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);

			if (info->opcode == WS_TEXT) {
				for (size_t i = 0; i < info->len; i++) {
					msg += (char)data[i];
				}
			}
			receivedJson(client,msg);
		}
	}
}

// 		start WebServer
void startWebServer() 
{
	ws.onEvent(onWsEvent);
	server.addHandler(&ws);
	//	root
	server.serveStatic("/", LittleFS, "/www/").setDefaultFile("index.html");
	// si la pag no existe lo mandamos a root
	server.onNotFound([](AsyncWebServerRequest *request) {
		//request->redirect("/");
	});

	server.begin();
	Serial.println("started WebServer");
}

//		setup
void setup() 
{
	Serial.begin(115200);
	delay(1000);
	Serial.println("\n\n");

	if(!LittleFS.begin(true))
		Serial.println("An Error has occurred while mounting LittleFS");
	
	// leemos config
	if (!readJson("/data/config.json", &config))
		writeJson("/data/config.json", &config);
	
	// leemos historial
	if (!readJson("/data/results.json", &history))
		writeJson("/data/results.json", &history);

	printJsonConfig();
	printJsonHistory();

	beginWifi();
	startWifi();
	startWebServer();
}
//		loop
void loop() {
	static uint32_t c = 3000;
	static uint32_t c1 = 0;
	
	dnsServer.processNextRequest();
	//cada 0.5s enviamos info del sensor a todos los clientes
	if (millis() - c > 5000) {
		//int t = micros();
		send(createJsonSensors());
		c = millis();
		//Serial.println(micros()-t);
	}	
	//cada s comprobamos si es necesario reiniciar wifi :/
	if (millis() - c1 > 1000 && !lock && resetWifi) {
		c1 = millis();
		sendMessage(1, "Restarting wifi");
		restartWifi();
		if (WiFi.status() == WL_CONNECTED) 
			sendMessage(0, "Wifi connected");
		else 
			sendMessage(1, "Wifi not connected");
	}
	ws.cleanupClients();
}