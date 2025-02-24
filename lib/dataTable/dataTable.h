/*
 Name:		tester
 Created:	23/01/2025 12:49:23
 Author:	buho29

 		libreria generica para manejar datos

 - hay q heredar de item y implementar de/serializacion json
 - DataTable y DataList tiene un array de instancias de Item
   el array.length es fijo y las instancias reutilizable (para evitar fragmentacion de la memoria?)
 - DataTable
  y DataList tiene un map/list de punteros de Item se maneja item desde los punteros

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
		if(!obj["id"].is<int>() &&
			!obj["edad"].is<int>() &&
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

	//fill table
	int i = myTable.size();
	while (myTable.size() < myTable.maxSize)
	{
		String name = "table " + (String)++i;

		MyItem* item = myTable.getEmpty();
		assertTrue(item);
		if (item) {
			item->set(-1, 69 - i, name.c_str());
			assertTrue(myTable.push(item));
		}
	}


	Serial.println("myTable");
	for (auto pair : myTable)
		Serial.printf("\tid %d %s \n", pair.first, pair.second->name);
*/

#ifndef _DATATABLE_h
#define _DATATABLE_h

#include <map>
#include <list>
#include <iterator>
#include <vector>

#include <ArduinoJson.h>

struct Item {
	static const uint32_t CREATE_NEW = 4294967295;
	uint32_t id = CREATE_NEW;
	virtual void serializeItem(JsonObject &obj,bool extra) = 0;
	virtual bool deserializeItem(JsonObject &obj) = 0;
};

class Iserializable {
public:
	virtual bool deserializeData(const String & json) = 0;
	virtual String serializeString() = 0;
	virtual void serializeData(JsonArray &obj, bool extra) = 0;
};

template <uint N,class T>
class BaseData:public Iserializable {
protected:
	T items[N];
public:
	const int maxSize = N;

	virtual T* getEmpty()
	{
		for (int i = 0; i < N; i++)
		{
			T * item = &items[i];
			if (item->id == Item::CREATE_NEW) return item;
		}
		return nullptr;
	};

	T* create(JsonObject &obj) 
	{
		T* t = getEmpty();

		if (t && t->deserializeItem(obj))
			return t;
		return nullptr;
	};

	virtual void clear() {
		for (int i = 0; i < N; i++)
		{
			T* item = &items[i];
			item->id = Item::CREATE_NEW;
		}
	}

	//Iserializable
	bool deserializeData(const String & json) {
		bool result = false;
		JsonDocument doc;
		// Parse
		uint32_t c = millis();
		DeserializationError error = deserializeJson(doc, json);
		if(error){
			Serial.printf("%s : %s \n",error.f_str(),json.c_str());
			return false;
		}
		// Loop through all the elements of the array
		for (JsonObject obj : doc.as<JsonArray>()) 
		{
			if(push(create(obj)))
				result = true;
			else return false;
		}

		return result;
	};
	String serializeString() {
		JsonDocument doc; String str;
		JsonArray root = doc.to<JsonArray>();
		
		serializeData(root);
		serializeJson(root, str);//Pretty
		return str;
	};

	virtual void serializeData(JsonArray &obj, bool extra=false) = 0;
	virtual T* push(T* item) = 0;
};

// map base
template <uint N, class T, class K>
class MapBaseData :public BaseData<N,T > {
protected:
	std::map<K, T*> mapItems;
public:
	typename std::map<K, T*>::iterator begin() {
		return mapItems.begin();
	};
	typename std::map<K, T*>::iterator end() {
		return mapItems.end();;
	};

	T* operator[](K key) { return mapItems[key]; };
	uint32_t size() { return mapItems.size(); };

	bool has(K key) { 
		return this->mapItems.find(key) != this->mapItems.end(); 
	};
	virtual void clear() { 
		BaseData<N, T >::clear();
		mapItems.clear(); 
	};
	virtual bool remove(K key) {
		if (this->mapItems.find(key) != this->mapItems.end()) {
			T* item = this->mapItems[key];
			if (this->mapItems.erase(key)) {
				//lo marcamos para reutilizacion
				item->id = Item::CREATE_NEW;
				return true;
			}
		}

		return false;
	};
	//bool remove(T* item) { return remove(item->id); };
	void printItems() {
		for (size_t i = 0; i < N; i++)
		{
			Serial.printf("%d item id: %d", i, this->items[i].id);
			if (has(i)) {
				Serial.printf(" has map id: %d\n", this->mapItems[i]->id);
			}
			else Serial.printf(" not map \n");
		}
	};

	void serializeData(JsonArray &obj, bool extra = false) {
		for (auto elem : this->mapItems)
		{
			T* item = elem.second;
			JsonObject o = obj.add<JsonObject>();
			item->serializeItem(o, extra);
		};
	};

	virtual T* push(T* item) = 0;
};

// Table
template < uint N,class T>
class DataTable :public MapBaseData<N,T, uint32_t> {
protected:
	uint getUniqueId(uint id = Item::CREATE_NEW) {
		if (id < Item::CREATE_NEW) return id;
		for (int i = 0; i < N; i++)
		{
			if (this->mapItems.find(i) == this->mapItems.end())
				return i;
		}
		return Item::CREATE_NEW;
	};
public:
	T* push(T * item) {
		if (item) {
			uint id = getUniqueId(item->id);
			if (id < Item::CREATE_NEW) {
				item->id = id;
				this->mapItems.insert(std::make_pair(id, item));
				return item;
			}
		}
		return nullptr;
	};
};

// List
template <uint N,class T>
class DataList :public BaseData<N,T> {
protected:
	std::list<T *> listItems;
public:
	typename std::list<T*>::iterator begin() {
		return listItems.begin();
	};
	typename std::list<T*>::iterator end() {
		return listItems.end();
	};

	uint32_t size() { return  listItems.size(); };
	T* push(T* item) {
		if (item) {
			item->id = 1;
			listItems.push_back(item);
			return item;
		}
		return nullptr;
	};
	bool has(T* item) {
		auto it = std::find(listItems.begin(), listItems.end(), item);
		if (it != listItems.end()) return true;
		return false;
	};
	bool remove(T*item) {
		auto it = std::find(listItems.begin(), listItems.end(), item);

		if (it != listItems.end()) {

			listItems.erase(it);

			//lo marcamos para reutilizacion
			item->id = Item::CREATE_NEW;

			return true;
		}

		return false;
	};
	void clear() {
		BaseData<N, T >::clear();
		listItems.clear();
	};
	bool shift() {
		if (size() <= 0) return false;

		//lo marcamos para reutilizacion
		listItems.front()->id = Item::CREATE_NEW;
		listItems.pop_front();
		return true;
	};
	bool pop() {
		if (size() <= 0) return false;
		//lo marcamos para reutilizacion
		listItems.back()->id = Item::CREATE_NEW;
		listItems.pop_back();
		return true;
	};
	T* last() {
		return listItems.back();
	}
	T* first() {
		return listItems.front();
	}

	void printItems() {
		for (size_t i = 0; i < N; i++)
		{
			Serial.printf("%d item id: %d", i, this->items[i].id);
			if (has(&this->items[i])) 
			{
				Serial.printf(" has listItems\n");
			}
			else Serial.printf(" not listItems \n");
		}
	};
	void serializeData(JsonArray &root, bool extra = false) {

		for (T* item : listItems)
		{
			JsonObject o = root.add<JsonObject>();
			item->serializeItem(o, extra);
		}
		
	};
};

// array
template <uint N,class T>
class DataArray :public BaseData<N,T> {
protected:
	std::vector<T *> arrayItems;
public:
	typename std::vector<T*>::iterator begin() {
		return arrayItems.begin();
	};
	typename std::vector<T*>::iterator end() {
		return arrayItems.end();
	};

	uint32_t size() { return  arrayItems.size(); };
	T* push(T* item) {
		if (item) {
			item->id = 1;
			arrayItems.push_back(item);
			return item;
		}
		return nullptr;
	};
	bool has(T* item) {
		auto it = std::find(arrayItems.begin(), arrayItems.end(), item);
		if (it != arrayItems.end()) return true;
		return false;
	};
	bool remove(T*item) {
		auto it = std::find(arrayItems.begin(), arrayItems.end(), item);

		if (it != arrayItems.end()) {

			arrayItems.erase(it);

			//lo marcamos para reutilizacion
			item->id = Item::CREATE_NEW;

			return true;
		}

		return false;
	};
	void clear() {
		BaseData<N, T >::clear();
		arrayItems.clear();
	}
	
	bool pop() {
		if (size() <= 0) return false;
		//lo marcamos para reutilizacion
		arrayItems.back()->id = Item::CREATE_NEW;
		arrayItems.pop_back();
		return true;
	}
	T* last() {
		return arrayItems.back();
	}
	T* first() {
		return arrayItems.front();
	}

	T* operator[](uint index) {
		if (size() <= index) return nullptr;
		return arrayItems[index];
	}

	void printItems() {
		for (size_t i = 0; i < N; i++)
		{
			Serial.printf("%d item id: %d", i, this->items[i].id);
			if (has(&this->items[i])) 
			{
				Serial.printf(" has arrayItems\n");
			}
			else Serial.printf(" not arrayItems \n");
		}
	};
	
	void serializeData(JsonArray &root, bool extra = false) {

		for (T* item : arrayItems)
		{
			JsonObject o = root.add<JsonObject>();
			item->serializeItem(o, extra);
		}
		
	};
};
#endif