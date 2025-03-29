/*
 Name:		tester
 Created:	23/01/2025 12:49:23
 Author:	buho29
*/

#include "Arduino.h"
#include <algorithm>

#include "HX711.h"

#include <NetworkManager.h>
#include <FileJsonManager.h>
#include <MotorController.h>
#include <ServerManager.h>

#include "data.h"
#include "TestAnalyzer.h"

// host name to mDNS, http://plastester.local
const char *hostName = "plastester";
// HX711 scale
const int LOADCELL_DOUT_PIN = 26;
const int LOADCELL_SCK_PIN = 25;
const float CALIBRATING_FACTOR = -2316138 / 44.34; // read/kg
HX711 scale;

// motor
const uint8_t MOTOR_STEP_PIN = 32;
const uint8_t MOTOR_DIRECTION_PIN = 33;
const uint8_t ENDSTOP_PIN = 27;
MotorController motor(MOTOR_STEP_PIN, MOTOR_DIRECTION_PIN, ENDSTOP_PIN);

FileJsonManager fileManager;
ServerManager server;
NetworkManager network;
bool resetWifi = false;

// permanent app variables, see data.h
//		to save the variables in a json file
Config config;

//		current sencor
SensorItem currentSensor;
TestAnalyzer analyzer;

const uint8_t MAX_HISTORY = 20;
DataArray<MAX_HISTORY, HistoryItem> history;

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
	static DataArray<TestAnalyzer::MAX_RESULT, SensorItem> result;

	uint32_t c = millis();
	JsonDocument root;
	String json;

	JsonArray doc = root["results"].to<JsonArray>();

	for (JsonVariant v : array)
	{
		uint index = v.as<uint>();
		HistoryItem *item = history[index];
		if (item)
		{
			String path = String("/data") + item->pathData;

			JsonObject obj = doc.add<JsonObject>();
			obj["name"] = item->name;
			obj["date"] = item->date;
			obj["description"] = item->description;
			obj["avg_count"] = item->averageCount;
			obj["length"] = item->length;
			obj["area"] = item->area;

			if (fileManager.readJson(path.c_str(), &result))
			{
				JsonArray arr = obj["data"].to<JsonArray>();
				result.serializeData(arr);
				// Serial.printf("[%d] path:%s\n", index, path.c_str());
				// Serial.println(result.serializeString() + "\n");
			}
			//
		}
	}

	serializeJson(root, json); // Pretty

	Serial.printf("createJsonResults total %d ms\n", millis() - c);

	// Serial.println(json);

	return json;
}
String createJsonLastResult()
{
	uint32_t c = millis();
	JsonDocument root;
	String json;

	JsonArray doc = root["lastResult"].to<JsonArray>();
	analyzer.accumulated_data.serializeData(doc);

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
	HistoryItem *item = history[index];
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
	if (obj["id"].is<uint8_t>() && 
		obj["desc"].is<const char *>() && 
		obj["area"].is<float>() && 
		obj["length"].is<float>() )
	{
		uint8_t index = obj["id"];
		HistoryItem *item = history[index];
		
		if (item)
		{
			strcpy(item->description, obj["desc"]);
			item->area = obj["area"];
			item->length = obj["length"];

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
void renameResult(JsonObject &obj, AsyncWebSocketClient *client)
{
	if (obj["id"].is<uint8_t>() && obj["name"].is<const char *>())
	{
		uint8_t index = obj["id"];
		const char *name = obj["name"];

		HistoryItem *item = history[index];

		String p = String("/result/") + name + ".json";
		String file = "/data" + p;
		const char *path = p.c_str();

		if (item)
		{
			if (fileManager.exists(file))
			{
				server.sendMessage(ServerManager::ERROR, "the name already exists choose another", client);
			}
			else if (item->isValide(path, name, item->date, item->description))
			{
				String old = String("/data") + item->pathData;
				if (fileManager.renameFile(old.c_str(), file.c_str()))
				{
					strcpy(item->pathData, path);
					strcpy(item->name, name);
					if (fileManager.writeJson("/data/results.json", &history))
					{
						String path = String("/result/") + name;
						server.goTo(path.c_str(), client);
						server.sendMessage(ServerManager::GOOD, name + String(" renamed!"), client);
						// send all update;
						clientConnected(nullptr);
					}
					else
						server.sendMessage(ServerManager::ERROR, "error write history file", client);
				}
				else
					server.sendMessage(ServerManager::ERROR, "error rename file", client);
			}
			else
				server.sendMessage(ServerManager::ERROR, "error input data not is valid", client);
		}
		else
			server.sendMessage(ServerManager::ERROR, "error result not found file", client);
	}
	else
		server.sendMessage(ServerManager::ERROR, "error result bad parameter rename", client);
}
void newResult(JsonObject &obj, AsyncWebSocketClient *client)
{
	HistoryItem *item = history.getEmpty();

	if (!item)
	{
		server.sendMessage(ServerManager::ERROR, String("Only ") + history.maxSize + " items can be saved, please delete some", client);
		server.goTo("/history", client);
	}
	else if (analyzer.isEmpty())
	{
		server.sendMessage(ServerManager::WARN, " last result not found", client);
	}
	else if (
		obj["name"].is<const char *>() &&
		obj["desc"].is<const char *>() &&
		obj["date"].is<const char *>() &&
		obj["length"].is<float>() &&
		obj["area"].is<float>())
	{
		const char *name = obj["name"];
		const char *date = obj["date"];
		const char *description = obj["desc"];
		float length = obj["length"];
		float area = obj["area"];

		String p = String("/result/") + name + ".json";
		String file = "/data" + p;
		const char *path = p.c_str();

		if (fileManager.exists(file))
		{
			server.sendMessage(ServerManager::ERROR, "the name already exists choose another", client);
		}
		else if (item->isValide(path, name, date, description))
		{
			Serial.printf("new result %s %s\n", file.c_str(), path);
			//	save result
			Serial.printf("%s\n", analyzer.accumulated_data.serializeString().c_str());
			if (fileManager.writeJson(file.c_str(), &analyzer.accumulated_data))
			{
				item->set(path, name, date, description, length, area);
				history.push(item);
				if (fileManager.writeJson("/data/results.json", &history))
				{
					String path = String("/result/") + name;
					server.goTo(path.c_str(), client);
					server.sendMessage(ServerManager::GOOD, name + String(" created!"), client);
					// TODO crear cmd puto vago
					//clear result in client 
					analyzer.clear();
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
	scale.set_scale(CALIBRATING_FACTOR);
	scale.tare(80);
}
void readSensors()
{
	if (scale.is_ready())
	{
		currentSensor.set(
			motor.getPosition(),
			scale.get_units(1));
	}
	else
		currentSensor.distance = motor.getPosition();
}
void updateSensors()
{
	static uint32_t c = 0;
	if (millis() - c > 200)
	{
		// int t = micros();
		if (state != State::TESTRUN)
			readSensors();

		server.send(createJsonSensors());
		c = millis();
		// Serial.printf("readsensor %.2f\n",(micros()-t)/1000.0);
	}
}

// validation user action
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
bool isRunning(AsyncWebSocketClient *client)
{
	if (motor.isRunning())
	{
		server.sendMessage(ServerManager::ERROR, "First stop motor", client);
		return true;
	}
	return false;
}
bool isTestRunning(AsyncWebSocketClient *client)
{
	if (state == TESTRUN)
	{
		server.sendMessage(ServerManager::ERROR, "The test is running, please stop it or wait.", client);
		return true;
	}
	return false;
}
bool isHomePos(AsyncWebSocketClient *client)
{
	if (motor.getPosition() != 0)
	{
		server.sendPopup("Warn",
						 "Attention!,<br>The motor is not in the home position.",
						 "{\"sethome\":1}", client);
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
// motor callback
void onMotorEvent(MotorController *m)
{

	Serial.printf("motion event ldir: %d pos: %.2f state %d\n", m->getDirection(), m->getPosition(), m->getState());
	if (m->getState() == MotorController::LIMIT_LEFT)
	{
		Serial.println("Limit Left");
		server.sendMessage(ServerManager::WARN, "Limit Left!");
	}
	else if (m->getState() == MotorController::LIMIT_RIGHT)
	{
		Serial.println("Limit RIGTH");
		if (!limitChecked && state == State::RUNHOME)
		{
			limitChecked = true;
			m->setCurrentPosition(config.home_pos);
			m->goHome();
			state = State::EMPTY;
		}
		server.sendMessage(ServerManager::WARN, "Limit Rigth!");
	}
	else if (m->getState() == MotorController::MOTION_END)
	{
		server.sendMessage(ServerManager::GOOD, "Motion End!");
		// Serial.printf("motion end ldir: %d",m->getDirection());
		//  if(m->getDirection()> 0) m->moveTo(10);
		//  else m->moveTo(20);
	}
}

void setupMotor()
{
	motor.begin(true);
	defaultConfigMotor();
	motor.setOnMotorEvent(onMotorEvent);
}

////////////////////////////////////////
//		TEST
////////////////////////////////////////

bool testReadyToStop = false;
float testTriggerWeigth = 0.3; // 0.1 - 5 kg
float testDist = 5.0;		   // 1 - 10 mm
float testSpeed = 1.0;		   // 0.1 - 5 kg
float testAcceleration = 2.0;  // 1 - 10 mm

void addAverage(uint8_t index, AsyncWebSocketClient *client)
{

	if (analyzer.isEmpty())
	{
		server.sendMessage(ServerManager::WARN, " last result not found", client);
		return;
	}

	HistoryItem *item = history[index];
	if (!item)
	{
		server.sendMessage(ServerManager::ERROR, "error Update Average not found result", client);
		return;
	}

	String path = String("/data") + item->pathData;

	if (!fileManager.readJson(path.c_str(), &analyzer.accumulated_data))
	{
		server.sendMessage(ServerManager::ERROR, "error read result file", client);
		return;
	}

	// Increment the counter of processed series
	analyzer.addTest(++item->averageCount);

	// save result
	if (fileManager.writeJson(path.c_str(), &analyzer.accumulated_data))
	{
		// save history
		if (fileManager.writeJson("/data/results.json", &history))
		{
			String url = String("/result/") + item->name;
			server.goTo(url.c_str(), client);
			server.sendMessage(ServerManager::GOOD,
							   item->name + String(" Update Average n° ") + (item->averageCount + 1), client);
			// clear result in client
			analyzer.clear();// TODO crear cmd puto vago
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

void startTest()
{
	Serial.printf("run test %.2f %.2f\n", testDist, testTriggerWeigth);
	analyzer.clear();
	analyzer.clearData();
	state = TESTRUN;
	testStep = START;
	scale.tare(10);

	motor.setSpeedAcceleration(testSpeed * 2, testAcceleration);
	motor.jogging();
}
void clearTest()
{
	motor.setSpeedAcceleration(config.speed, config.acc_desc);
	state = EMPTY;
	testReadyToStop = false;
	testStep = STOP;
}
void updateTest()
{
	static uint32_t currentTime = 0;
	static float zeroPos = 0.0;
	static int32_t zeroTime = 0;
	static uint8_t count = 0;

	if (millis() - currentTime > TestAnalyzer::TEST_STEP_TIME)
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
				motor.setSpeedAcceleration(testSpeed, testAcceleration);
				motor.move(testDist);
				testStep = MEASURING;
				zeroTime = 0;
				zeroPos = pos;
				Serial.printf("Test started: weight %.2fkg, position %.2fmm\n", force, pos);
				Serial.printf("Trigger weight: %.2fkg\n", testTriggerWeigth);
			}
			break;

		case MEASURING:
			zeroTime += TestAnalyzer::TEST_STEP_TIME;

			if (!analyzer.addPoint(pos - zeroPos, force, zeroTime))
			{
				Serial.println("Error: Sensor data overflow");
				server.sendMessage(ServerManager::ERROR, "The time has run out");
				clearTest();
				motor.goHome();
				return;
			}

			// Prepare to stop if force exceeds threshold
			if (force > 1.0 && !testReadyToStop)
				testReadyToStop = true;

			bool shouldStop = (testReadyToStop && force < 0.5);
			bool exceededMaxForce = (force > config.max_force - 2);
			bool motionEnded = motor.isMotionEnd();

			if (exceededMaxForce || motionEnded || shouldStop)
			{
				if (shouldStop && count < 1)
				{
					count++;
					return;
				}

				Serial.println("Test finished:");
				if (exceededMaxForce)
					Serial.println("\n Exceeded max force \n");
				if (motionEnded)
					Serial.println("\n Test Motion ended \n");
				Serial.printf("Force dropped below threshold: %d (force: %.2fkg)\n", shouldStop, force);

				clearTest();
				motor.goHome();
				analyzer.addTest();
				server.send(createJsonLastResult());
				server.goTo("/result/n");
				server.sendMessage(ServerManager::GOOD, "Test finished successfully!");
				count = 0;
			}
			break;
		}
	}
}

////////////////////////////////////////
//		SERVER
////////////////////////////////////////

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
		bool invert = root["invert_motor"];
		float pitch = root["screw_pitch"];
		uint8_t step = root["micro_step"];

		if (invert != config.invert_motor ||
			pitch != config.screw_pitch ||
			step != config.micro_step)
		{
			limitChecked = false;
		}

		config.setSpeedAcceleration(root["speed"], root["acc_desc"]);
		config.setMotor(pitch, step, invert);
		defaultConfigMotor();
		modified = true;
	}
	//	Home
	if (root["home_pos"].is<float>() && root["max_travel"].is<float>() &&
		root["max_force"].is<float>())
	{
		float newPos = root["home_pos"]; // 15
		if (limitChecked)
		{
			// [ ] sin probar!!
			float pos = motor.getPosition(); //-10
			float old = config.home_pos;	 // 20
			float delta = newPos - old;		 // 15 - 20 //-5
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
		motor.emergencyStop();
		clearTest();
		server.sendMessage(ServerManager::WARN, "Stop emergency!!", client);
	}
	// el test
	else if (root["move"].is<JsonObject>())
	{
		JsonObject m = root["move"].as<JsonObject>();
		if (m["dist"].is<float>() && m["dir"].is<int8_t>())
		{
			if (!isLimitChecked(client))
				return;

			float dist = m["dist"];
			int8_t dir = m["dir"];

			if (dir == 0)
			{
				motor.stop();
				clearTest();
				server.sendMessage(ServerManager::GOOD, "Stopped!", client);
				return;
			}

			if (isTestRunning(client))
				return;

			char buffer[60];
			float pos = dist * dir;
			sprintf(buffer, "Moving %.3fmm \n", pos);
			if (motor.move(pos))
			{
				server.sendMessage(ServerManager::GOOD, String(buffer), client);
			}
			else
			{
				server.sendMessage(ServerManager::WARN, "Limit reachead, not " + String(buffer), client);
			}
		}
	}
	else if (isTestRunning(client))
		return;
	else if (root["run"].is<JsonObject>())
	{
		if (!isLimitChecked(client) || isRunning(client) || !isHomePos(client))
			return;

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
			if (!isRunning(client) && isLimitChecked(client))
			{
				motor.setHome(motor.getPosition());
				server.sendMessage(ServerManager::GOOD, "Set Home applied", client);
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
		state = State::RUNHOME;
		motor.jogging(false);
	}
	else if (root["calibrate"].is<uint>())
	{
		limitChecked = false;
		isLimitChecked(client);
	}
	else if (root["tare"].is<uint>())
	{
		if (motor.isRunning())
			server.sendMessage(ServerManager::ERROR, "First stop motor", client);
		else
		{
			scale.tare(80);
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
	else if (root["rename"].is<JsonObject>())
	{
		JsonObject m = root["rename"].as<JsonObject>();
		renameResult(m, client);
	}
	else if (root["add_avg"].is<uint8_t>())
	{
		uint8_t id = root["add_avg"];
		addAverage(id, client);
	}
}

bool lock = false;
bool clientLoadPublic(JsonObject &root, AsyncWebSocketClient *client)
{
	lock = true;
	bool exit = false;
	// un cliente quiere acceder a datos publicos
	if (root["loadData"].is<JsonObject>() && !isRunning(client))
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
	lock = false;
	return exit;
}
bool clientLoadPrivate(JsonObject &root, AsyncWebSocketClient *client)
{
	lock = true;
	// un usuario auth quiere acceder a las datos restringidos
	if (root["loadDataAuth"].is<int8_t>() && !isTestRunning(client))
	{
		int8_t page = root["loadDataAuth"];

		switch (page)
		{
		case 0:
			server.send(createJsonOption(), client);
			break;
		case 1:
			server.sendSystem(client);
			server.sendJsonFiles(client);
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
	if (root["config"].is<JsonObject>() && !isRunning(client))
	{
		JsonObject con = root["config"];
		receivedConfig(client, con);
	}
	lock = false;
	return true;
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

void update()
{
	static uint32_t c = 0;
	// cada 5s comprobamos si es necesario reiniciar wifi :/
	if (resetWifi && millis() - c > 5000 && !lock)
	{
		c = millis();
		server.sendMessage(ServerManager::ERROR, "Restarting wifi");
		network.disconnect();
		delay(1000);
		resetWifi = false;
		network.connect(config.wifi_ssid, config.wifi_pass);
		if (network.isConnected())
			server.sendMessage(ServerManager::GOOD, "Wifi connected");
		else
			server.sendMessage(ServerManager::ERROR, "Wifi not connected");
	}
}

////////////////////////////////////////
//		APP
////////////////////////////////////////
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
	server.update();
	network.update();
	if (state == TESTRUN)
		updateTest();
	updateSensors();
	update();
}
