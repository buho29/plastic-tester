
#include <DataTable.h>
#include <AUnit.h>
#include "data.h"
using aunit::TestRunner;

/********************
	config
********************/

Config config;
test(TestConfig) {
	JsonDocument doc; String str;
	JsonObject obj = doc.to<JsonObject>();


	assertTrue(config.setAdmin("user","pass"));
	assertFalse(config.setAdmin("this have more 32 characters long","pass"));

	assertTrue(config.setWifi("wifi", "pass wifi"));
	assertFalse(config.setWifi("this have more 32 characters long", "pass"));
	
	config.serializeItem(obj,false);
	serializeJsonPretty(doc, str);
	Serial.println(str);

	const char* json = R"(
{
  "wifi_pass": "^t!pcm774K",
  "www_pass": "admin",
  "www_user": "admin",
  "wifi_ssid": "Zyxel_E49C",
  "acc_desc": 4,
  "speed": 8,
  "screw_pitch": 2,
  "micro_step": 8
}
)";

	JsonDocument doc1;
	
	DeserializationError error = deserializeJson(doc1, String(json));
	assertFalse((bool)error);

	obj = doc1.as<JsonObject>();
	assertTrue(config.deserializeItem(obj));

}
/********************
	ResultTest
********************/

DataList<3,ResultItem> results;

test(TestResults) 
{
	//fill table
	int i = 0;
	while (results.size() < results.maxSize)
	{
		String p = "/data/files"+ (String)++i+".json";
		String n = "files"+ (String)i;

		ResultItem* item = results.getEmpty();
		assertTrue(item);
		if (item) {
			item->set(p.c_str(),n.c_str(), "30/01/2025 21:04", "teta");
			assertTrue(results.push(item));
		}
	}

	assertFalse(results.getEmpty());

	//print json
	Serial.println(results.serializeString());

	ResultItem* item = results.first();
	assertTrue(results.shift());
	assertFalse(results.has(item));

	Serial.println("myList");
	for (ResultItem* item : results)
		Serial.printf("\t %s\n", item->pathData);


	results.clear();
	assertEqual((int)results.size(), 0);

	const char* json = R"(
[
  {
    "path": "/data/files1.json",
    "date": "30/01/2025 21:04",
    "description": "teta"
  },
  {
    "path": "/data/files2.json",
    "date": "30/01/2025 21:04",
    "description": "teta"
  },
  {
    "path": "/data/files3.json",
    "date": "30/01/2025 21:04",
    "description": "teta"
  }
]
)";
	assertFalse(results.deserializeData(json));

	const char* json1 = R"(
[
  {
    "path": "/data/files1.json",
    "name": "files1",
    "date": "30/01/2025 21:04",
    "description": "teta"
  },
  {
    "path": "/data/files2.json",
    "name": "files2",
    "date": "30/01/2025 21:04",
    "description": "teta"
  },
  {
    "path": "/data/files3.json",
    "name": "files3",
    "date": "30/01/2025 21:04",
    "description": "teta"
  }
]
)";	
	assertTrue(results.deserializeData(json1));
	assertEqual((int)results.size(), 3);
}


DataList<10,SensorItem> lastResult;


test(TestSensorsList) 
{
	//fill table
	int i = 0;
	while (lastResult.size() < lastResult.maxSize)
	{
		i++;
		SensorItem* item = lastResult.getEmpty();
		assertTrue(item);
		if (item) {
			float d = random(1000)/100.0;
			float f = random(5000)/100.0;
			item->set(d, f,i);
			assertTrue(lastResult.push(item));
		}
	}

	assertFalse(lastResult.getEmpty());

	//print json
	Serial.println(lastResult.serializeString());

	SensorItem* item = lastResult.first();
	assertTrue(lastResult.shift());
	assertFalse(lastResult.has(item));

	Serial.println("result force");
	for (SensorItem* item : lastResult)
		Serial.printf("\t%.3f\n", item->force);


	lastResult.clear();
	assertEqual((int)lastResult.size(), 0);

	const char* json = R"(
[
  {
    "d": 33.72,
    "f": 3.31,
    "t": 1
  },
  {
    "d": 18.22,
    "f": 43.9,
    "t": 2
  },
  {
    "d": 53.08,
    "f": 35.42,
    "t": 3
  },
  {
    "d": 52.68,
    "f": 33.65,
    "t": 4
  },
  {
    "d": 38.52,
    "f": 15.66,
    "t": 5
  }
]
)";
	assertTrue(lastResult.deserializeData(json));
	assertEqual((int)lastResult.size(), 5);
	assertFalse(lastResult.deserializeData("teta"));
}



void setup() 
{
	delay(1000);
	Serial.begin(115200);
}


void loop()
{
	TestRunner::run();
}

/* output:
TestRunner started on 2 test(s).
{
  "wifi_pass": "pass wifi",
  "www_pass": "pass",
  "www_user": "user",
  "wifi_ssid": "wifi",
  "acc_desc": 4,
  "speed": 8,
  "screw_pitch": 2,
  "micro_step": 8
}
Test testConfig passed.
testResultTest
{
  "date": "30/01/2025 21:04",
  "content": "teta",
  "data": [
    {
      "d": 78.87,
      "f": 20.72,
      "t": 1
    },
    {
      "d": 2.36,
      "f": 24.28,
      "t": 2
    },
    {
      "d": 82.72,
      "f": 13.2,
      "t": 3
    },
    {
      "d": 69.07,
      "f": 15.54,
      "t": 4
    }
  ]
}
result force
        20.720
        24.280
        13.200
        15.540
Test testResultTest passed.
TestRunner duration: 0.049 seconds.
TestRunner summary: 2 passed, 0 failed, 0 skipped, 0 timed out, out of 2 test(s).
*/