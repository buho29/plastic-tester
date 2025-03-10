/*
 Name:		tester
 Created:	23/01/2025 12:49:23
 Author:	buho29
*/

#include "Arduino.h"

#include "HX711.h"

#include <NetworkManager.h>
#include <FileJsonManager.h>
#include <MotorController.h>
#include <ServerManager.h>

#include "data.h"

// host name to mDNS, http://tester.local
const char *hostName = "plastic_tester";
// motor
const uint8_t MOTOR_STEP_PIN = 32;
const uint8_t MOTOR_DIRECTION_PIN = 33;
const uint8_t ENDSTOP_PIN = 27;
// HX711 scale

const int LOADCELL_DOUT_PIN = 26;
const int LOADCELL_SCK_PIN = 25;

MotorController motor(MOTOR_STEP_PIN, MOTOR_DIRECTION_PIN, ENDSTOP_PIN);
NetworkManager network;
FileJsonManager fileManager;
ServerManager server;

HX711 scale;

//permanent app variables, see data.h
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

void restartWifi()
{
	network.disconnect();
	delay(1000);
	resetWifi = false;
	network.connect(config.wifi_ssid, config.wifi_pass);
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
String createJsonSensors()
{
	//{"ss":{"d":77.10,"f":46.49}}
	return String("{\"ss\":{\"d\":") +
		   currentSensor.distance + ",\"f\":" +
		   currentSensor.force + "}}";
}
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
void clientConnected(AsyncWebSocketClient *client)
{
	uint32_t c = millis();
	JsonDocument root;
	String json;

	JsonArray doc = root["history"].to<JsonArray>();
	history.serializeData(doc);

	serializeJsonPretty(root, json);
	server.send(json, client);

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
				server.sendMessage(ServerManager::GOOD, "result deleted", client);
				clientConnected(nullptr); // send all udpdate
			}
			else
				server.sendMessage(ServerManager::ERROR, "error write history file", client);
		}
		else
			server.sendMessage(ServerManager::ERROR, "error result not found file", client);
	}
	else
		server.sendMessage(ServerManager::ERROR, "error deleted not found result", client);
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
				server.sendMessage(ServerManager::GOOD, item->name + String(" saved"), client);
				clientConnected(nullptr); // send all udpdate;
			}
			else
				server.sendMessage(ServerManager::ERROR, "error write history file", client);
		}
		else
			server.sendMessage(ServerManager::ERROR, "error result not found file", client);
	}
	else
		server.sendMessage(ServerManager::ERROR, "error result not saved", client);
}
void newResult(JsonObject &obj, AsyncWebSocketClient *client)
{
	ResultItem *item = history.getEmpty();

	if (!item)
	{
		server.sendMessage(ServerManager::ERROR, String("Only ") + history.maxSize + " items can be saved, please delete some", client);
		server.goTo("/history", client);
	}
	else if (lastResult.size() < 1)
	{
		server.sendMessage(ServerManager::WARN, " last result not found", client);
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
			server.sendMessage(ServerManager::ERROR, "the name already exists choose another", client);
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
					server.goTo(path.c_str(), client);
					server.sendMessage(ServerManager::GOOD, name + String(" created!"), client);
					// clear result in client
					lastResult.clear();
					server.send(createJsonLastResult(), client);
					// send all update;
					clientConnected(nullptr);
				}
				else
					server.sendMessage(ServerManager::ERROR, "error write history file", client);
			}
			else
				server.sendMessage(ServerManager::ERROR, "error write result file", client);
		}
		else
			server.sendMessage(ServerManager::ERROR, "error input data not is valid", client);
	}
}

enum Step
{
	STOP = 0,
	START = 1,
	MEASURING = 2, // measuring
};
Step testStep = STOP;
enum State
{
	EMPTY = 0,
	RUNHOME = 1,
	TESTRUN = 2
};
State state = EMPTY;
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
		 int t = micros();
		if (state != TESTRUN)
			readSensors();
		//
		server.send(createJsonSensors());
		c = millis();
		//Serial.printf("readsensor %.2f\n",(micros()-t)/1000.0);
	}
}

bool limitChecked = false;
bool isLimitChecked(AsyncWebSocketClient *client)
{
	if (!limitChecked)
	{
		server.sendPopup("Warn",
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
	motor.setConfigMotor(steps_mm, config.speed, config.acc_desc, config.invert_motor);
	motor.setConfigHome(config.home_pos, config.max_travel);
}
// Función callback
void onMotorEvent(MotorController *m)
{
	if (m->getState() == MotorController::LIMIT_LEFT)
	{
		Serial.println("Limit Left");
		server.sendMessage(ServerManager::WARN, "Limit left!");
	}
	else if (m->getState() == MotorController::LIMIT_RIGHT)
	{
		Serial.println("Limit RIGTH");
		if (!limitChecked && state == State::RUNHOME )
		{
			limitChecked = true;
			motor.setCurrentPosition(config.home_pos);
			motor.goHome();
			state = State::EMPTY;
		}
		server.sendMessage(ServerManager::WARN, "Limit switch active!");
	}
	else if (m->getState() == MotorController::MOTION_END)
	{
		Serial.printf("motion end ldir: %d",motor.getDirection());
		// if(motor.getDirection()> 0) motor.moveTo(10);
		// else motor.moveTo(20);
	}
}

void setupMotor()
{
	motor.begin();
	defaultConfigMotor();
	motor.setOnMotorEvent(onMotorEvent);
}

bool testReadyToStop = false;
float testTriggerWeigth = 0.3; // 0.1 - 5 kg
float testDist = 5.0;		   // 1 - 10 mm
float testSpeed = 1.0; // 0.1 - 5 kg
float testAcceleration = 2.0;		   // 1 - 10 mm

void startTest()
{
	Serial.printf("run test %.2f %.2f\n", testDist, testTriggerWeigth);
	lastResult.clear();
	state = TESTRUN;
	testStep = START;
	scale.tare(2);
	
	// [ ] configurar speed / acce
	motor.setSpeedAcceleration(testSpeed * 2,testAcceleration);
	motor.seekLimitSwitch();
}
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
				// [ ] configurar speed / acce
				motor.setSpeedAcceleration(testSpeed,testAcceleration);
				motor.move(testDist);
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
				server.sendMessage(ServerManager::ERROR, "the time has run out");
				exit = true;
			}

			// cuando superamos 1kg preparamos para parar
			if (force > 1)
				testReadyToStop = true;

			if (exit || (testReadyToStop && force < 0.5) ||
				force > config.max_force - 2 ||
				motor.isMotionEnd())
			{
				motor.stop();
				motor.setSpeedAcceleration(config.speed,config.acc_desc);
				clearTest();
				server.send(createJsonLastResult());
				server.goTo("/result/n");
				server.sendMessage(ServerManager::GOOD, "Test finish!!");
			}
			break;
		}
	}
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
			server.sendCmd("logout", "1");
		}
	}
	//		motor and speed
	if (root["speed"].is<float>() && root["acc_desc"].is<float>() &&
		root["screw_pitch"].is<float>() && root["micro_step"].is<uint8_t>() &&
		root["invert_motor"].is<bool>())
	{
		config.setSpeedAcceleration(root["speed"], root["acc_desc"]);
		config.setMotor(root["screw_pitch"], root["micro_step"],
						root["invert_motor"]);
		defaultConfigMotor();
		modified = true;
	}
	//	Home
	if (root["home_pos"].is<float>() && root["max_travel"].is<float>() &&
		root["max_force"].is<float>())
	{
		float newPos = root["home_pos"];//15
		if(limitChecked){
			// [ ] sin probar!!
			float pos = motor.getPosition();//-10
			float old = config.home_pos;//20
			float delta = newPos - old; //15 - 20 //-5
			motor.setCurrentPosition(pos + delta);
		}

		config.setHome(newPos, root["max_travel"], root["max_force"]);
		defaultConfigMotor();
		modified = true;
	}

	if (modified)
	{
		if (fileManager.writeJson("/data/config.json", &config))
		{
			server.sendMessage(ServerManager::GOOD, "Options edited");
			// enviar cambiaos a todos los clientes auth
			server.sendAllAuth(createJsonOption());
		}
		else
			server.sendMessage(ServerManager::ERROR, "error write config file", client);
	}
}
void receivedCmd(AsyncWebSocketClient *client, JsonObject &root)
{

	if (root["stop"].is<uint8_t>())
	{
		clearTest();
		motor.emergencyStop();
		server.sendMessage(ServerManager::WARN, "Stop emergency!!", client);
	}
	// el test
	else if (state == TESTRUN)
	{
		server.sendMessage(ServerManager::WARN,
						   "The test is running, please stop it or wait.", client);
		return;
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

			motor.move(dist * dir);
			char buffer[60];
			sprintf(buffer, "moving %.3fmm in %d direction\n", dist, dir);
			server.sendMessage(ServerManager::GOOD, String(buffer), client);
		}
	}
	else if (root["run"].is<JsonObject>())
	{
		if (!isLimitChecked(client))
			return;
		
		
		// [ ] configurar speed / acce
		JsonObject obj = root["run"].as<JsonObject>();
		if (obj["dist"].is<float>() && obj["trigger"].is<float>() && 
			obj["speed"].is<float>() && obj["acc_desc"].is<float>())
		{
			testDist = obj["dist"];
			testTriggerWeigth = obj["trigger"];
			testSpeed = obj["speed"];
			testAcceleration = obj["acc_desc"];
			startTest();
			server.sendMessage(ServerManager::GOOD, "Test started!", client);
		}
	}
	else if (root["sethome"].is<uint>())
	{
		uint8_t home = root["sethome"];

		if (home == 0)
		{
			if (motor.isRunning())
				server.sendMessage(ServerManager::ERROR, "First stop motor", client);
			else if (isLimitChecked(client))
			{
				motor.setHome(motor.getPosition());
				// distanceSwitch += stepper.getCurrentPositionInMillimeters();
				//  set home
				// setCurrentPosition();
				// TODO TODO
				// BUG bug
				// FIXME fix
				// HACK hack???
				// [x] toma!!
				// [ ] ijaaa
				// XXX
				//
			}
		}
		else if (home == 1)
		{
			// go home
			motor.goHome();
			server.sendMessage(ServerManager::GOOD, "Go Home", client);
		}
	}
	else if (root["checkLimit"].is<uint>() && !limitChecked)
	{
		motor.seekLimitSwitch();
		state =State::RUNHOME;
	}
	else if (root["tare"].is<uint>())
	{
		if (motor.isRunning())
			server.sendMessage(ServerManager::ERROR, "First stop motor", client);
		else
		{
			scale.tare();
			server.sendMessage(ServerManager::GOOD, "Tare scale", client);
		}
	}
	else if (root["restar"].is<uint8_t>())
	{
		server.sendMessage(ServerManager::ERROR, "Rebooting Esp32", client);
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

bool clientLoadPublic(JsonObject &root, AsyncWebSocketClient *client)
{
	bool exit = false;
	// un cliente quiere acceder a datos publicos
	if (root["loadData"].is<JsonObject>())
	{
		JsonObject obj = root["loadData"];

		//"{"load":{"paths":["/result/pla.json","/result/abs.json","/result/petg.json"]}}
		if (obj["indexes"].is<JsonArray>())
		{
			JsonArray j = obj["indexes"].as<JsonArray>();
			server.send(createJsonResults(j), client);
		}
		// exit
		return true;
	}
	return exit;
}
bool clientLoadPrivate(JsonObject &root, AsyncWebSocketClient *client)
{
	lock = true;
	// un usuario auth quiere acceder a las datos restringidos
	if (root["loadDataAuth"].is<int8_t>() && state != State::TESTRUN)
	{
		int8_t page = root["loadDataAuth"];

		switch (page)
		{
		case 0:
			server.send(createJsonOption(), client);
			break;
		case 1:
			server.sendSystem(client);
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
	if (root["config"].is<JsonObject>() && state != State::TESTRUN)
	{
		JsonObject con = root["config"];
		receivedConfig(client, con);
	}
	lock = false;
	return false;
}
// 		start WebServer
void startWebServer()
{
	server.setOnConnectedClient(clientConnected);
	server.setOnDataLoad(clientLoadPublic);
	server.setOnAuthSuccessDataLoad(clientLoadPrivate);
	server.setUserAuth(config.www_user, config.www_pass);

	server.begin("/www/");
	Serial.println("started WebServer");
}

void updateConfig()
{
	static uint32_t c = 0;
	// cada s comprobamos si es necesario reiniciar wifi :/
	if (resetWifi && millis() - c > 1000 && !lock)
	{
		c = millis();
		server.sendMessage(ServerManager::ERROR, "Restarting wifi");
		restartWifi();
		if (network.isConnected())
			server.sendMessage(ServerManager::GOOD, "Wifi connected");
		else
			server.sendMessage(ServerManager::ERROR, "Wifi not connected");
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
	
	static uint32_t c = 0;
	static uint32_t t = 0;
	// Serial.printf("readsensor %.2f\n",(micros()-t)/1000.0);
	//t = micros();
	motor.checkLimit();
	network.update();
	if (state == TESTRUN)
		updateTest();
	updateSensors();
	updateConfig();
	// limpiamos clientes no conectados
	server.update();
	if (millis() - c > 1000)
	{
		
		//Serial.printf("readsensor %.2f %d\n",(micros()-t)/1000.0,tskIDLE_PRIORITY);
		c = millis();
	}/**/
}
