#include <DataTable.h>
#include <AUnit.h>
using aunit::TestRunner;
/********************
	my Item
********************/
struct MyItem : public Item
{
	uint8_t edad;
	char name[20] = "";

	void set(int id, uint8_t edad, const char * name)
	{
		this->id = id; this->edad = edad;
		strcpy(this->name, name);
	};
	void serializeItem(JsonObject &obj, bool extra) 
	{
		obj["id"] = this->id;
		obj["edad"] = this->edad;
		obj["name"] = this->name;
	};
	bool deserializeItem(JsonObject &obj) {
		if(!obj["id"].is<int>() ||
			!obj["edad"].is<int>() ||
			!obj["name"].is<const char*>()
		) {
			Serial.println("faill deserializeItem MyItem");
			return false;
		}
		set(
			obj["id"],obj["edad"],obj["name"]
		);
		return true;
	};
};

DataTable<7,MyItem> myTable;
DataList<5,MyItem> myList;
DataArray<5,MyItem> myArray;

test(DataList) 
{
	assertEqual(myList.maxSize,5);

	//fill list
	int i = myList.size();
	while (myList.size() < myList.maxSize)
	{
		String t = "list " + (String)++i;

		MyItem* item = myList.getEmpty();
		assertTrue(item);
		if (item) {
			item->set(-1, 19 + i, t.c_str());
			assertTrue(myList.push(item));
		}
	}

	assertFalse(myList.getEmpty());
	
	MyItem* item = myList.first();
	assertTrue(myList.shift());
	assertFalse(myList.has(item));

	Serial.println("myList");
	for (MyItem* item : myList)
		Serial.printf("\t %s\n", item->name);

	//print json
	Serial.println( myList.serializeString());

	myList.clear();
	assertEqual((int)myList.size(), 0);
}
/*
test(DataTable) 
{
	assertEqual(myTable.maxSize,7);
	assertEqual(myList.maxSize,5);
	assertEqual(myArray.maxSize,5);

	//fill table
	int i = myTable.size();
	while (myTable.size() < myTable.maxSize)
	{
		String t = "table " + (String)++i;

		MyItem* item = myTable.getEmpty();
		assertTrue(item);
		if (item) {
			item->set(-1, 69 - i, t.c_str());
			assertTrue(myTable.push(item));
		}
	}

	assertFalse(myTable.getEmpty());

	assertTrue(myTable.remove(4));
	assertFalse(myTable.has(4));
	assertFalse(myTable.has(666));
	assertTrue(myTable.has(0));

	Serial.println("myTable");
	for (auto pair : myTable)
		Serial.printf("\tid %d %s \n", pair.first, pair.second->name);

	Serial.println(myTable.serializeString());

	myTable.clear();
	assertEqual((int)myTable.size(), 0);

	String json = "[{\"id\":1,\"edad\":21,\"name\":\"list 2\"},{\"id\":2,\"edad\":22,\"name\":\"list 3\"}]";
	assertTrue(myTable.deserializeData(json));
	assertEqual((int)myTable.size(), 2);
	assertFalse(myTable.deserializeData(String("teta")));

}
*/
test(DataArray) 
{
	assertEqual(myArray.maxSize,5);

	//fill list
	int i = myArray.size();
	while (myArray.size() < myArray.maxSize)
	{
		String t = "array " + (String)++i;

		MyItem* item = myArray.getEmpty();
		assertTrue(item);
		if (item) {
			item->set(-1, 19 + i, t.c_str());
			assertTrue(myArray.push(item));
		}
	}

	assertFalse(myArray.getEmpty());

	MyItem* item = myArray[2];
	assertTrue(item);
	assertTrue(myArray.remove(item));
	assertFalse(myArray.has(item));

	// fill array
	Serial.println("myArray");
	for (MyItem* item : myArray)
		Serial.printf("\t %s\n", item->name);

	//print json
	Serial.println( myArray.serializeString());

	myArray.clear();
	assertEqual((int)myArray.size(), 0);

}

/********************
	diccio lang
*******************
struct StrItem :public Item
{
	char value[50] = "";

	void set(uint32_t id, const char* str) {
		this->id = id;
		strcpy(this->value, str);
	};
	void serializeItem(JsonObject &obj, bool extra) {
		obj["id"] = this->id;
		obj["value"] = this->value;
	};
	bool deserializeItem(JsonObject &obj) {
		if (!obj["id"].is<uint32_t>() ||
			!obj["value"].is<const char*>()) 
		{
			Serial.println("faill deserializeItem StrItem");
			return false;
		}
		set(obj["id"],obj["value"]);
		return true;
	};
};

template <uint N>
class StringTable :public DataTable<N, StrItem> {
public:
	String get(uint32_t key) {
		if (this->has(key)) {
			StrItem * str = this->mapItems[key];
			return String(str->value);
		}
		return String(" String not Found ");
	};
	void add(uint32_t key, const char * str) {
		StrItem * item = this->getEmpty();
		if (item != nullptr) {
			item->set(key, str);
			this->push(item);
		}
	};
};

StringTable<30> lang;

enum Str1 {
	welcome, errorLogin, reqLogin
};

test(testLang) 
{
	Serial.println("testLang");

	lang.add(Str1::welcome, "Bienvenido ");
	lang.add(Str1::errorLogin, "usuario o pass no valido ");
	lang.add(Str1::reqLogin, "Requierre logearse");

	Serial.printf("Str1::welcome : %s\n",
		lang.get(Str1::welcome).c_str()
	);
	//lang
	for (auto pair : lang)
		Serial.printf("lang key :%d = %s\n", pair.first, pair.second->value);

	const char* json = "[{\"id\":0,\"value\":\"Bienvenido \"},{\"id\":1,\"value\":\"usuario o pass no valido \"},{\"id\":2,\"value\":\"Requierre logearse\"}]";
	assertEqual(json, lang.serializeString().c_str());

	Serial.println(lang.serializeString().c_str());
}
*/

/********************
	result test
*******************
struct SensorItem :public Item
{
	float distance; float force;
	uint32_t time;

	void set(float distance, float force, uint32_t time) 
	{
		this->distance = distance; this->force = force;
		 this->time = time;
	};
	void serializeItem(JsonObject & obj, bool extra = false) 
	{
		obj["d"] = this->distance; obj["f"] = this->force;
		obj["t"] = this->time;
	};
	bool deserializeItem(JsonObject & obj)
	{
		if (!obj["d"].is<float>() || !obj["f"].is<float>() ||
			!obj["t"].is<uint>() )
		{
			Serial.println("faill deserializeItem SensorItem");
			return false;
		}
		set( obj["d"], obj["f"], obj["t"] );
		return true;
	};
};

template <uint N>
class ResultTest :public DataList<N, SensorItem> {
public:
	char date[40] = "";
	char content[200] ="";
	
	void set(const char * date, const char * content)
	{
		strcpy(this->date, date);
		strcpy(this->content, content);
	};

	String serializeString() 
	{
		JsonDocument root; String str;

		root["date"] = this->date;
		root["content"] = this->content;

		JsonArray arr = root["data"].to<JsonArray>();
		this->serializeData(arr);
		serializeJsonPretty(root, str);//
		return str;
	}

	bool deserializeData(const String & json) 
	{
		bool result = false;
		JsonDocument root;
		// Parse
		DeserializationError error = deserializeJson(root, json);
		if(error){
			Serial.printf("ResultTest %s : %s \n",error.f_str(),json.c_str());
			return false;
		}

		if(!root["date"].is<const char*>() ||
			!root["content"].is<const char*>())
		{
			Serial.println("faill deserialize SensorItem ResultTest");
			return false;
		}
		set(root["date"],root["content"]);

		if(root["data"].is<JsonArray>()){
			// Loop through all the elements of the array
			for (JsonObject obj : root["data"].as<JsonArray>()) 
			{
				if(this->push(this->create(obj)))
					result = true;
				else return false;
			}			
		}else Serial.println("faill deserialize SensorItem data not found");



		return result;
	}
	
};

ResultTest<4> result;
test(testResultTest) 
{
	Serial.println("testResultTest");
	result.set("30/01/2025 21:04","teta");

	//fill list
	uint i = result.size();
	while (result.size() < result.maxSize)
	{
		i++;
		SensorItem* item = result.getEmpty();
		assertTrue(item);
		if (item) {
			float d = random(10000)/100.0;
			float f = random(5000)/100.0;
			item->set(d, f,i);
			assertTrue(result.push(item));
		}
	}

	Serial.println( result.serializeString());


	Serial.println("result force");
	for (SensorItem* item : result)
		Serial.printf("\t%.3f\n", item->force);

	SensorItem* item = result.first();
	assertTrue(result.shift());
	assertFalse(result.has(item));

	result.clear();
	assertEqual((int)result.size(), 0);

	const char* json = R"(
{
  "date": "30/01/2025 21:04",      
  "content": "teta",
  "data": [
    {
      "d": 10.1,
      "f": 1.97,
      "t": 1
    },
    {
      "d": 58.56,
      "f": 38.95,
      "t": 2
    },
    {
      "d": 73.31,
      "f": 36.77,
      "t": 3
    }
  ]
}
)";
	assertTrue(result.deserializeData(String(json)));
	assertEqual((int)result.size(), 3);

}
*/

void setup() 
{
	delay(1000);
	Serial.begin(115200);
}


void loop()
{
	TestRunner::run();
}

/**/