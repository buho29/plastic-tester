/*
 Name:		tester
 Created:	23/01/2025 12:49:23
 Author:	buho29
*/

#include "Arduino.h"

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <mbedtls/md.h> //encript

#include "HX711.h"
#include <ESP_FlexyStepper.h>

#include <NetworkManager.h>
#include <FileJsonManager.h>
#include <MotorController.h>

#include "data.h"

NetworkManager network;
FileJsonManager fileManager;


//		WebServer variables
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

//		list authenticated users
std::list<AsyncWebSocketClient *> clientsAuth;
//		host name to mDNS, http://tester.local
const char *hostName = "Tester";
//		permanent app variables, see data.h
//		to save the variables in a json file
Config config;
//		current sencor
SensorItem currentSensor;

const uint8_t MAX_HISTORY = 10;
const uint8_t MAX_RESULT = 200;

DataArray<MAX_HISTORY, ResultItem> history;
DataList<MAX_RESULT, SensorItem> lastResult;

bool lock = false;
bool resetWifi = false;

// HX711 scale
const int LOADCELL_DOUT_PIN = 26;
const int LOADCELL_SCK_PIN = 25;
HX711 scale;

// motor
const uint8_t MOTOR_STEP_PIN = 32;
const uint8_t MOTOR_DIRECTION_PIN = 33;
const uint8_t ENDSTOP_PIN = 27;
MotorController motor(MOTOR_STEP_PIN,MOTOR_DIRECTION_PIN,ENDSTOP_PIN);

void restartWifi()
{
	network.disconnect();
	delay(1000);
	resetWifi = false;
	network.connect(config.wifi_ssid, config.wifi_pass);
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
	GOOD = 0,
	ERROR = 1,
	WARN = 2,
	POPUP = 3
};
// send notifications to user, if client = nullptr send all users
// if type = 0 notify normal
// type = 1 notify error (popup)
// type = 2 notify warn
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

//		crypto
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
				String(config.www_user) + ":" +
				String(config.www_pass) + ":" +
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
	return strcmp(config.www_user, user) == 0 &&
		   strcmp(config.www_pass, pass) == 0;
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

// 			APP
//		print config json
void printJsonConfig()
{
	JsonDocument doc;
	String str;
	JsonObject obj = doc.to<JsonObject>();

	config.serializeItem(obj, false);
	serializeJsonPretty(doc, str);
	Serial.println(str.c_str());
}
//		print history json
void printJsonHistory()
{
	String str = history.serializeString();
	Serial.println(str.c_str());
}

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
//		return String in json format
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
//		return String in json format
String createJsonOption()
{
	// uint32_t c = millis();
	JsonDocument root;
	String str;

	JsonObject doc = root["config"].to<JsonObject>();
	config.serializeItem(doc, true);

	serializeJsonPretty(root, str);
	// Serial.printf("createJsonOption %d ms\n", millis() - c);
	// Serial.println(str.c_str());
	return str;
}
//		return String in json format
String createJsonSensors()
{
	//{"ss":{"d":77.10,"f":46.49}}
	return String("{\"ss\":{\"d\":") +
		   currentSensor.distance + ",\"f\":" +
		   currentSensor.force + "}}";
}
//		TODO
String createJsonHistory()
{
	JsonDocument root;
	String json;
	uint32_t c = millis();

	JsonArray doc = root["hystory"].to<JsonArray>();
	history.serializeData(doc);

	serializeJsonPretty(root, json);

	Serial.printf("printJsonFiles total %d ms\n", millis() - c);

	return json;
}

String createJsonResults(JsonArray &array)
{
	static DataList<MAX_RESULT, SensorItem> result;

	uint32_t c = millis();
	JsonDocument root;
	String json;

	JsonArray doc = root["results"].to<JsonArray>();

	for (JsonVariant v : array)
	{
		uint index = v.as<uint>();
		ResultItem *item = history[index];
		if (item)
		{
			String path = String("/data") + item->pathData;

			JsonObject obj = doc.add<JsonObject>();
			obj["name"] = item->name;
			obj["date"] = item->date;
			obj["description"] = item->description;

			result.clear();

			if (fileManager.readJson(path.c_str(), &result))
			{
				JsonArray arr = obj["data"].to<JsonArray>();
				result.serializeData(arr);
			}
			// Serial.printf("[%d] path:%s\n",index , path.c_str());
		}
	}

	serializeJson(root, json); // Pretty

	Serial.printf("createJsonResults total %d ms\n", millis() - c);

	// Serial.println(json);

	return json;
}

String createJsonLastResult()
{
	static DataList<10, SensorItem> result;

	uint32_t c = millis();
	JsonDocument root;
	String json;

	JsonArray doc = root["lastResult"].to<JsonArray>();
	lastResult.serializeData(doc);

	serializeJson(root, json); // Pretty

	// Serial.printf("createJsonResults total %d ms\n", millis() - c);
	Serial.println(json);

	return json;
}

// primera contacto
void sendHistory(AsyncWebSocketClient *client)
{
	uint32_t c = millis();
	JsonDocument root;
	String json;

	JsonArray doc = root["history"].to<JsonArray>();
	history.serializeData(doc);

	serializeJsonPretty(root, json);
	send(json, client);

	Serial.printf("sendHistory total %d ms\n", millis() - c);
}

// manage data results
void deleteResult(uint8_t index, AsyncWebSocketClient *client)
{
	ResultItem *item = history[index];
	if (item)
	{
		String file = String("/data") + item->pathData;

		if (history.remove(item) && fileManager.deleteFile(file))
		{
			if (fileManager.writeJson("/data/results.json", &history))
			{
				sendMessage(GOOD, "result deleted", client);
				sendHistory(nullptr); // send all udpdate
			}
			else
				sendMessage(ERROR, "error write history file", client);
		}
		else
			sendMessage(ERROR, "error result not found file", client);
	}
	else
		sendMessage(ERROR, "error deleted not found result", client);
}
void saveResult(JsonObject &obj, AsyncWebSocketClient *client)
{
	if (obj["id"].is<uint8_t>() && obj["desc"].is<const char *>())
	{
		uint8_t index = obj["id"];
		ResultItem *item = history[index];
		if (item)
		{
			strcpy(item->description, obj["desc"]);
			if (fileManager.writeJson("/data/results.json", &history))
			{
				sendMessage(GOOD, item->name + String(" saved"), client);
				sendHistory(nullptr); // send all udpdate;
			}
			else
				sendMessage(ERROR, "error write history file", client);
		}
		else
			sendMessage(ERROR, "error result not found file", client);
	}
	else
		sendMessage(ERROR, "error result not saved", client);
}
void newResult(JsonObject &obj, AsyncWebSocketClient *client)
{
	ResultItem *item = history.getEmpty();

	if (!item)
	{
		sendMessage(ERROR, String("Only ") + history.maxSize + " items can be saved, please delete some", client);
		sendCmd("goTo", "/history", client);
	}
	else if (lastResult.size() < 1)
	{
		sendMessage(WARN, " last result not found", client);
	}
	else if (
		obj["name"].is<const char *>() &&
		obj["desc"].is<const char *>() &&
		obj["date"].is<const char *>())
	{
		const char *name = obj["name"];
		const char *date = obj["date"];
		const char *desc = obj["desc"];

		String p = String("/result/") + name + ".json";
		String file = "/data" + p;
		const char *path = p.c_str();

		if (fileManager.exists(file))
		{
			sendMessage(ERROR, "the name already exists choose another", client);
		}
		else if (item->isValide(path, name, date, desc))
		{
			Serial.printf("new result %s %s\n", file.c_str(), path);
			//	save result
			Serial.printf("%s\n", lastResult.serializeString().c_str());
			if (fileManager.writeJson(file.c_str(), &lastResult))
			{
				item->set(path, name, date, desc);
				history.push(item);
				if (fileManager.writeJson("/data/results.json", &history))
				{
					String path = String("/result/") + name;
					sendCmd("goTo", path.c_str(), client);
					sendMessage(GOOD, name + String(" created!"), client);
					// clear result in client
					lastResult.clear();
					send(createJsonLastResult(), client);
					// send all update;
					sendHistory(nullptr);
				}
				else
					sendMessage(ERROR, "error write history file", client);
			}
			else
				sendMessage(ERROR, "error write result file", client);
		}
		else
			sendMessage(ERROR, "error input data not is valid", client);
	}
}

enum Step
{
	STOP = 0,
	START = 1,
	MEASURING = 2, // measuring
};
Step testStep = STOP;
enum action
{
	EMPTY = 0,
	GOHOME = 1,
	RUNHOME = 2,
	TESTRUN = 3
};
action state = EMPTY;
void setupSensors()
{

	scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
	float s = -2316138 / 44.34; // read/kg
	scale.set_scale(s);
	scale.tare();
}
void readSensors()
{
	if (scale.is_ready())
	{
		currentSensor.set(
			motor.getPosition(),
			scale.get_units(1), 0);
	}
}
void updateSensors()
{
	static uint32_t c = 3000;
	if (millis() - c > 200)
	{
		// int t = micros();
		if (state != TESTRUN)
			readSensors();
		//
		send(createJsonSensors());
		c = millis();
		// Serial.printf("readsensor %.2f\n",(micros()-t)/1000.0);
	}
}

bool limitChecked = false;
bool isLimitChecked(AsyncWebSocketClient *client)
{
	if (!limitChecked)
	{
		sendPopup("Warn",
				  "Attention!,<br>The limit will be checked.",
				  "{\"checkLimit\":1}", client);
		return false;
	}
	return true;
}


void defaultConfigMotor()
{
	// 200steps/revolution   stepping 1/8 reduction 60:10 screw pitch 2mm
	const float steps_mm = 200 * config.micro_step * 6 / config.screw_pitch;
	motor.setConfigMotor(steps_mm,config.speed,config.acc_desc,config.invert_motor);
	motor.setConfigHome(config.home_pos,config.max_travel);
}
// Función callback
void handleMotorChange(MotorController *m)
{
    if (m->getState() == MotorController::LIMIT_LEFT)
    {
        Serial.println("Limit Left");
		sendMessage(WARN, "Limit left!");
    }
    else if (m->getState() == MotorController::LIMIT_RIGHT)
    {
        Serial.println("Limit RIGTH");
		if (!limitChecked)
		{
			limitChecked = true;
			motor.setHome(config.home_pos);
			motor.goHome();
		}
		sendMessage(WARN, "Limit switch active!");
    } 
	else if (m->getState() == MotorController::MOTION_END)
    {
        //Serial.printf("motion end ldir: %d",motor.getLastDirection());
        //if(motor.getLastDirection()> 0) motor.moveAbsolute(10);
        //else motor.moveAbsolute(20);
    }
}

void setupMotor()
{
	motor.begin();
	defaultConfigMotor();
	motor.setHome(0);
	motor.goHome();
	motor.setCallback(handleMotorChange);
}

bool testReadyToStop = false;
float testTriggerWeigth = 0.3; // 0.1 - 5 kg
float testDist = 5.0;		   // 1 - 10 mm

void clearTest()
{
	state = EMPTY;
	testReadyToStop = false;
	testStep = STOP;
}
void updateTest()
{
	static uint32_t currentTime = 0;
	static float zeroPos = 0.0;
	static int32_t zeroTime = 0;

	if (millis() - currentTime > 100)
	{
		currentTime = millis();

		readSensors();

		float force = currentSensor.force;
		float pos = currentSensor.distance;

		switch (testStep)
		{
		case START:
			if (force >= testTriggerWeigth)
			{
				motor.setSpeed(config.speed / 4);
				motor.moveRelative(testDist);
				testStep = MEASURING;
				zeroTime = millis();
				zeroPos = pos;
				Serial.printf("start weigth %.2f\n", pos);
				Serial.println("trigger weigth");
			}
			break;
		case MEASURING:
			bool exit = false;

			SensorItem *item = lastResult.getEmpty();
			if (item)
			{
				item->set(pos - zeroPos, force, currentTime - zeroTime);
				lastResult.push(item);
			}
			else
			{
				Serial.println("error item overflow");
				sendMessage(ERROR, "the time has run out");
				exit = true;
			}

			// cuando superamos 1kg preparamos para parar
			if (force > 1)
				testReadyToStop = true;

			if (exit || (testReadyToStop && force < 0.5) ||
				force > config.max_force - 2
				// ||motor.isMotionEnd()
			){
				motor.stop();
				motor.setSpeed(config.speed);
				clearTest();
				send(createJsonLastResult());
				sendCmd("goTo", "/result/n");
				sendMessage(GOOD, "Test finish!!");
			}
			break;
		}
	}
}
void startTest(){
	Serial.printf("run test %.2f %.2f\n", testDist, testTriggerWeigth);
	lastResult.clear();
	state = TESTRUN;
	testStep = START;
	scale.tare();

	motor.setSpeed(config.speed / 2);
	motor.goToSwitch();
}


void receivedConfig(AsyncWebSocketClient *client, JsonObject &root)
{
	bool modified = false;

	JsonObject con = root;

	if (root["wifi_ssid"].is<const char *>() &&
		root["wifi_pass"].is<const char *>())
	{
		if (config.setWifi(root["wifi_ssid"], root["wifi_pass"]))
		{
			modified = true;
			resetWifi = true;
		}
	}
	if (root["www_user"].is<const char *>() && root["www_pass"].is<const char *>())
	{
		if (config.setAdmin(root["www_user"], root["www_pass"]))
		{
			modified = true;
			sendCmd("logout", "1");
		}
	}
	//		motor and speed
	if (root["speed"].is<float>() && root["acc_desc"].is<float>() &&
		root["screw_pitch"].is<float>() && root["micro_step"].is<uint8_t>() &&
		root["invert_motor"].is<bool>())
	{
		config.setSpeed(root["speed"], root["acc_desc"]);
		config.setMotor(root["screw_pitch"], root["micro_step"],
						root["invert_motor"]);
		defaultConfigMotor();
		modified = true;
	}

	if (root["home_pos"].is<float>() && root["max_travel"].is<float>() &&
		root["max_force"].is<float>())
	{
		config.setHome(root["home_pos"], root["max_travel"], root["max_force"]);
		modified = true;
	}

	if (modified)
	{
		if (fileManager.writeJson("/data/config.json", &config))
		{
			sendMessage(GOOD, "Options edited");
			// enviar cambiaos a todos los clientes auth
			sendAllAuth(createJsonOption());
		}
		else
			sendMessage(ERROR, "error write config file", client);
	}
}
void receivedCmd(AsyncWebSocketClient *client, JsonObject &root)
{

	if (root["stop"].is<uint8_t>())
	{
		clearTest();
		motor.emergencyStop();
		sendMessage(WARN, "Stop emergency!!", client);
	}
	else if (root["move"].is<JsonObject>())
	{
		JsonObject m = root["move"].as<JsonObject>();
		if (m["dist"].is<float>() && m["dir"].is<int8_t>())
		{
			if (!isLimitChecked(client))
				return;
			float dist = m["dist"];
			int8_t dir = m["dir"];

			motor.moveRelative(dist * dir);
			char buffer[60];
			sprintf(buffer, "moving %.3fmm in %d direction\n", dist, dir);
			sendMessage(GOOD, String(buffer), client);
		}
	}
	else if (root["run"].is<JsonObject>())
	{
		if (!isLimitChecked(client))
			return;

		JsonObject obj = root["run"].as<JsonObject>();
		if (obj["dist"].is<float>() && obj["trigger"].is<float>())
		{
			testDist = obj["dist"];
			testTriggerWeigth = obj["trigger"];
			startTest();
			sendMessage(GOOD, "Test started!", client);
		}
	}
	else if (root["sethome"].is<uint>())
	{
		uint8_t home = root["sethome"];

		if (home == 0)
		{
			if (motor.isRunning())
				sendMessage(ERROR, "First stop motor", client);
			else if (isLimitChecked(client))
			{
				//distanceSwitch += stepper.getCurrentPositionInMillimeters();
				// set home
				//setHome();
			}
		}
		else if (home == 1)
		{
			// go home
			motor.goHome();
			sendMessage(GOOD, "Go Home", client);
		}
	}
	else if (root["checkLimit"].is<uint>() && !limitChecked)
	{
		motor.goToSwitch();
	}
	else if (root["tare"].is<uint>())
	{
		if (motor.isRunning())
			sendMessage(ERROR, "First stop motor", client);
		else
		{
			scale.tare();
			sendMessage(GOOD, "Tare scale", client);
		}
	}
	else if (root["restar"].is<uint8_t>())
	{
		sendMessage(ERROR, "Rebooting Esp32", client);
		ESP.restart();
	}
	else if (root["delete"].is<uint8_t>())
	{
		uint8_t id = root["delete"];
		deleteResult(id, client);
	}
	else if (root["new"].is<JsonObject>())
	{
		JsonObject m = root["new"].as<JsonObject>();
		newResult(m, client);
	}
	else if (root["save"].is<JsonObject>())
	{
		JsonObject m = root["save"].as<JsonObject>();
		saveResult(m, client);
	}
}
//		received json string from user
void receivedJson(AsyncWebSocketClient *client, const String &json)
{
	lock = true;
	Serial.println(json);
	JsonDocument doc;
	// Parse json
	DeserializationError error = deserializeJson(doc, json);
	if (error)
	{
		Serial.printf("error deserialize receivedJson %s\n",
					  error.f_str());
		sendMessage(ERROR, "error deserialize receivedJson", client);
		return;
	}

	JsonObject root = doc.as<JsonObject>();

	// un cliente quiere acceder a datos publicos
	if (root["loadData"].is<JsonObject>())
	{
		JsonObject obj = root["loadData"];

		//"{"load":{"paths":["/result/pla.json","/result/abs.json","/result/petg.json"]}}
		if (obj["indexes"].is<JsonArray>())
		{
			JsonArray j = obj["indexes"].as<JsonArray>();
			send(createJsonResults(j), client);
		}
		// exit
		return;
	}

	// un cliente intenta conectarse user/pass
	if (root["login"].is<JsonObject>())
	{
		JsonObject user = root["login"];
		if (user["name"].is<const char *>() && user["pass"].is<const char *>())
		{
			const char *name = user["name"];
			const char *pass = user["pass"];

			Serial.printf("loggin user %s pass%S", name, pass);

			if (isValideAuth(name, pass))
			{
				String token = createToken(client->remoteIP());
				sendCmd("token", token.c_str(), client);
				// go home
				sendCmd("goTo", "/", client);

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
		sendCmd("goTo", "/login");
		sendMessage(ERROR, "Need to login", client);
		// no seguimos
		return;
	}

	saveClientAuth(client);

	// un usuario auth quiere acceder a las datos restringidos
	if (root["loadDataAuth"].is<int8_t>())
	{
		int8_t page = root["loadDataAuth"];

		switch (page)
		{
		case 0:
			send(createJsonOption(), client);
			break;
		case 1:
			send(createJsonSystem(), client);
			break;
		default:
			removeClientAuth(client);
			break;
		}
	}

	// comandos para manejar el Esp32
	if (root["cmd"].is<JsonObject>())
	{
		JsonObject o = root["cmd"].as<JsonObject>();
		receivedCmd(client, o);
	}

	// un cliente intenta modificar config
	if (root["config"].is<JsonObject>())
	{
		JsonObject con = root["config"];
		receivedConfig(client, con);
	}
	lock = false;
}


//		event websocket
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
			   AwsEventType type, void *arg, uint8_t *data, size_t len)
{
	if (type == WS_EVT_CONNECT)
	{
		Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
		sendHistory(client);
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
// 		start WebServer
void startWebServer()
{
	ws.onEvent(onWsEvent);
	server.addHandler(&ws);
	//	root
	server.serveStatic("/", LittleFS, "/www/").setDefaultFile("index.html");
	// si la pag no existe lo mandamos a root
	server.onNotFound([](AsyncWebServerRequest *request)
					  {
						  // request->redirect("/");
					  });

	server.begin();
	Serial.println("started WebServer");
}

void updateConfig()
{
	static uint32_t c = 0;
	// cada s comprobamos si es necesario reiniciar wifi :/
	if (resetWifi && millis() - c > 1000 && !lock)
	{
		c = millis();
		sendMessage(ERROR, "Restarting wifi");
		restartWifi();
		if (network.isConnected())
			sendMessage(GOOD, "Wifi connected");
		else
			sendMessage(ERROR, "Wifi not connected");
	}
}


//		setup
void setup()
{
	Serial.begin(115200);
	delay(1000);
	Serial.println("\n\n");

	fileManager.begin();

	// leemos config
	if (!fileManager.readJson("/data/config.json", &config))
		fileManager.writeJson("/data/config.json", &config);

	// leemos historial
	if (!fileManager.readJson("/data/results.json", &history))
		fileManager.writeJson("/data/results.json", &history);

	// printJsonConfig();
	// printJsonHistory();
	setupSensors();
	setupMotor();

	network.begin(hostName);
	network.connect(config.wifi_ssid, config.wifi_pass);

	startWebServer();
}
//		loop
void loop()
{
	motor.checkLimit();
	if (state == TESTRUN)
		updateTest();
	updateSensors();
	updateConfig();
	network.update();
	// limpiamos clientes no conectados
	ws.cleanupClients();
}
