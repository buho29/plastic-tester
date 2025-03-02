
#ifndef DATATABLE
#define DATATABLE

#include <map>
#include <list>
#include <iterator>
#include <vector>
#include <ArduinoJson.h>


/**
 * @file dataTable.h
 * @brief Generic library for managing data.
 * @details This library provides classes for managing data in tables, lists, and arrays from JSON.
 *          It uses a fixed-size array of Item instances for memory efficiency and avoids fragmentation.
 *          The library includes classes for DataTable, DataList, and DataArray, each with its own
 *          data structure for storing and managing Item pointers.
 *
 * @author buho29
 */

/**
 * @brief Represents a base item with an ID and serialization/deserialization methods.
 * @details All data items should inherit from this class and implement the serializeItem and deserializeItem methods.
 */
struct Item
{
	static const uint32_t CREATE_NEW = 4294967295;
	uint32_t id = CREATE_NEW;
	virtual void serializeItem(JsonObject &obj, bool extra) = 0;
	virtual bool deserializeItem(JsonObject &obj) = 0;
};

/**
 * @brief Interface for serializable classes.
 * @details Defines methods for serializing and deserializing data to and from JSON format.
 */
class Iserializable
{
public:
	virtual bool deserializeData(const String &json) = 0;
	virtual String serializeString() = 0;
	virtual void serializeData(JsonArray &obj, bool extra) = 0;
};

/**
 * @brief Base class for data storage, providing common functionality for managing items.
 * @tparam N The maximum number of items that can be stored.
 * @tparam T The type of the items to be stored, must inherit from Item.
 */
template <uint N, class T>
class BaseData : public Iserializable
{
protected:
	T items[N];

public:
	const int maxSize = N;

	/**
	 * @brief Gets an empty item from the array.
	 * @return A pointer to an empty item, or nullptr if no empty items are available.
	 */
	virtual T *getEmpty()
	{
		for (int i = 0; i < N; i++)
		{
			T *item = &items[i];
			if (item->id == Item::CREATE_NEW)
				return item;
		}
		return nullptr;
	};

	/**
	 * @brief Creates a new item from a JSON object.
	 * @param obj The JSON object containing the item data.
	 * @return A pointer to the created item, or nullptr if creation fails.
	 */
	T *create(JsonObject &obj)
	{
		T *t = getEmpty();

		if (t && t->deserializeItem(obj))
			return t;
		return nullptr;
	};

	/**
	 * @brief Clears all items in the array by resetting their IDs.
	 */
	virtual void clear()
	{
		for (int i = 0; i < N; i++)
		{
			T *item = &items[i];
			item->id = Item::CREATE_NEW;
		}
	}

	// Iserializable
	/**
	 * @brief Deserializes data from a JSON string.
	 * @param json The JSON string to deserialize.
	 * @return True if deserialization is successful, false otherwise.
	 */
	bool deserializeData(const String &json)
	{
		bool result = false;
		JsonDocument doc;
		// Parse
		uint32_t c = millis();
		DeserializationError error = deserializeJson(doc, json);
		if (error)
		{
			Serial.printf("%s : %s \n", error.f_str(), json.c_str());
			return false;
		}
		// Loop through all the elements of the array
		for (JsonObject obj : doc.as<JsonArray>())
		{
			if (push(create(obj)))
				result = true;
			else
				return false;
		}

		return result;
	};
	/**
	 * @brief Serializes data to a JSON string.
	 * @return The JSON string representing the serialized data.
	 */
	String serializeString()
	{
		JsonDocument doc;
		String str;
		JsonArray root = doc.to<JsonArray>();

		serializeData(root);
		serializeJson(root, str); // Pretty
		return str;
	};

	/**
	 * @brief Serializes data to a JSON array.
	 * @param obj The JSON array to serialize the data to.
	 * @param extra A flag indicating whether to include extra data during serialization.
	 */
	virtual void serializeData(JsonArray &obj, bool extra = false) = 0;
	/**
	 * @brief Pushes an item to the data structure.
	 * @param item A pointer to the item to push.
	 * @return A pointer to the pushed item, or nullptr if the push fails.
	 */
	virtual T *push(T *item) = 0;
};

/**
 * @brief Base class for map-based data storage, inheriting from BaseData.
 * @tparam N The maximum number of items that can be stored.
 * @tparam T The type of the items to be stored, must inherit from Item.
 * @tparam K The type of the key used to identify items in the map.
 */
template <uint N, class T, class K>
class MapBaseData : public BaseData<N, T>
{
protected:
	std::map<K, T *> mapItems;

public:
	/**
	 * @brief Returns an iterator to the beginning of the map.
	 * @return An iterator to the beginning of the map.
	 */
	typename std::map<K, T *>::iterator begin()
	{
		return mapItems.begin();
	};
	/**
	 * @brief Returns an iterator to the end of the map.
	 * @return An iterator to the end of the map.
	 */
	typename std::map<K, T *>::iterator end()
	{
		return mapItems.end();
		;
	};

	/**
	 * @brief Accesses an item in the map using the key.
	 * @param key The key of the item to access.
	 * @return A pointer to the item, or nullptr if the key is not found.
	 */
	T *operator[](K key) { return mapItems[key]; };
	/**
	 * @brief Gets the number of items in the map.
	 * @return The number of items in the map.
	 */
	uint32_t size() { return mapItems.size(); };

	/**
	 * @brief Checks if the map contains an item with the given key.
	 * @param key The key to check for.
	 * @return True if the map contains the key, false otherwise.
	 */
	bool has(K key)
	{
		return this->mapItems.find(key) != this->mapItems.end();
	};
	/**
	 * @brief Clears all items in the map and resets the base data.
	 */
	virtual void clear()
	{
		BaseData<N, T>::clear();
		mapItems.clear();
	};
	/**
	 * @brief Removes an item from the map by its key.
	 * @param key The key of the item to remove.
	 * @return True if the item was successfully removed, false otherwise.
	 */
	virtual bool remove(K key)
	{
		if (this->mapItems.find(key) != this->mapItems.end())
		{
			T *item = this->mapItems[key];
			if (this->mapItems.erase(key))
			{
				// lo marcamos para reutilizacion
				item->id = Item::CREATE_NEW;
				return true;
			}
		}

		return false;
	};
	// bool remove(T* item) { return remove(item->id); };
	/**
	 * @brief Prints the IDs of all items in the map for debugging purposes.
	 */
	void printItems()
	{
		for (size_t i = 0; i < N; i++)
		{
			Serial.printf("%d item id: %d", i, this->items[i].id);
			if (has(i))
			{
				Serial.printf(" has map id: %d\n", this->mapItems[i]->id);
			}
			else
				Serial.printf(" not map \n");
		}
	};

	/**
	 * @brief Serializes the data in the map to a JSON array.
	 * @param obj The JSON array to serialize the data to.
	 * @param extra A flag indicating whether to include extra data during serialization.
	 */
	void serializeData(JsonArray &obj, bool extra = false)
	{
		for (auto elem : this->mapItems)
		{
			T *item = elem.second;
			JsonObject o = obj.add<JsonObject>();
			item->serializeItem(o, extra);
		};
	};

	/**
	 * @brief Pushes an item to the map.
	 * @param item A pointer to the item to push.
	 * @return A pointer to the pushed item, or nullptr if the push fails.
	 */
	virtual T *push(T *item) = 0;
};

/**
 * @brief Represents a data table that stores items in a map with unique uint32_t IDs.
 * @tparam N The maximum number of items that can be stored in the table.
 * @tparam T The type of the items to be stored, must inherit from Item.
 */
template <uint N, class T>
class DataTable : public MapBaseData<N, T, uint32_t>
{
protected:
	/**
	 * @brief Gets a unique ID for a new item.
	 * @param id The preferred ID for the item. If Item::CREATE_NEW, a new unique ID is generated.
	 * @return A unique ID for the item.
	 */
	uint getUniqueId(uint id = Item::CREATE_NEW)
	{
		if (id < Item::CREATE_NEW)
			return id;
		for (int i = 0; i < N; i++)
		{
			if (this->mapItems.find(i) == this->mapItems.end())
				return i;
		}
		return Item::CREATE_NEW;
	};

public:
	/**
	 * @brief Pushes an item to the data table.
	 * @param item A pointer to the item to push.
	 * @return A pointer to the pushed item, or nullptr if the push fails.
	 */
	T *push(T *item)
	{
		if (item)
		{
			uint id = getUniqueId(item->id);
			if (id < Item::CREATE_NEW)
			{
				item->id = id;
				this->mapItems.insert(std::make_pair(id, item));
				return item;
			}
		}
		return nullptr;
	};
};

/**
 * @brief Represents a data list that stores items in a linked list.
 * @tparam N The maximum number of items that can be stored in the list.
 * @tparam T The type of the items to be stored, must inherit from Item.
 */
template <uint N, class T>
class DataList : public BaseData<N, T>
{
protected:
	std::list<T *> listItems;

public:
	/**
	 * @brief Returns an iterator to the beginning of the list.
	 * @return An iterator to the beginning of the list.
	 */
	typename std::list<T *>::iterator begin()
	{
		return listItems.begin();
	};
	/**
	 * @brief Returns an iterator to the end of the list.
	 * @return An iterator to the end of the list.
	 */
	typename std::list<T *>::iterator end()
	{
		return listItems.end();
	};

	/**
	 * @brief Gets the number of items in the list.
	 * @return The number of items in the list.
	 */
	uint32_t size() { return listItems.size(); };
	/**
	 * @brief Pushes an item to the end of the list.
	 * @param item A pointer to the item to push.
	 * @return A pointer to the pushed item, or nullptr if the push fails.
	 */
	T *push(T *item)
	{
		if (item)
		{
			item->id = 1;
			listItems.push_back(item);
			return item;
		}
		return nullptr;
	};
	/**
	 * @brief Checks if the list contains the given item.
	 * @param item A pointer to the item to check for.
	 * @return True if the list contains the item, false otherwise.
	 */
	bool has(T *item)
	{
		auto it = std::find(listItems.begin(), listItems.end(), item);
		if (it != listItems.end())
			return true;
		return false;
	};
	/**
	 * @brief Removes an item from the list.
	 * @param item A pointer to the item to remove.
	 * @return True if the item was successfully removed, false otherwise.
	 */
	bool remove(T *item)
	{
		auto it = std::find(listItems.begin(), listItems.end(), item);

		if (it != listItems.end())
		{

			listItems.erase(it);

			// lo marcamos para reutilizacion
			item->id = Item::CREATE_NEW;

			return true;
		}

		return false;
	};
	/**
	 * @brief Clears all items in the list and resets the base data.
	 */
	void clear()
	{
		BaseData<N, T>::clear();
		listItems.clear();
	};
	/**
	 * @brief Removes the first item from the list.
	 * @return True if an item was successfully removed, false otherwise.
	 */
	bool shift()
	{
		if (size() <= 0)
			return false;

		// lo marcamos para reutilizacion
		listItems.front()->id = Item::CREATE_NEW;
		listItems.pop_front();
		return true;
	};
	/**
	 * @brief Removes the last item from the list.
	 * @return True if an item was successfully removed, false otherwise.
	 */
	bool pop()
	{
		if (size() <= 0)
			return false;
		// lo marcamos para reutilizacion
		listItems.back()->id = Item::CREATE_NEW;
		listItems.pop_back();
		return true;
	};
	/**
	 * @brief Gets the last item in the list.
	 * @return A pointer to the last item in the list.
	 */
	T *last()
	{
		return listItems.back();
	}
	/**
	 * @brief Gets the first item in the list.
	 * @return A pointer to the first item in the list.
	 */
	T *first()
	{
		return listItems.front();
	}

	/**
	 * @brief Prints the IDs of all items in the list for debugging purposes.
	 */
	void printItems()
	{
		for (size_t i = 0; i < N; i++)
		{
			Serial.printf("%d item id: %d", i, this->items[i].id);
			if (has(&this->items[i]))
			{
				Serial.printf(" has listItems\n");
			}
			else
				Serial.printf(" not listItems \n");
		}
	};
	/**
	 * @brief Serializes the data in the list to a JSON array.
	 * @param root The JSON array to serialize the data to.
	 * @param extra A flag indicating whether to include extra data during serialization.
	 */
	void serializeData(JsonArray &root, bool extra = false)
	{

		for (T *item : listItems)
		{
			JsonObject o = root.add<JsonObject>();
			item->serializeItem(o, extra);
		}
	};
};

/**
 * @brief Represents a data array that stores items in a dynamic array (vector).
 * @tparam N The maximum number of items that can be stored in the array.
 * @tparam T The type of the items to be stored, must inherit from Item.
 */
template <uint N, class T>
class DataArray : public BaseData<N, T>
{
protected:
	std::vector<T *> arrayItems;

public:
	/**
	 * @brief Returns an iterator to the beginning of the array.
	 * @return An iterator to the beginning of the array.
	 */
	typename std::vector<T *>::iterator begin()
	{
		return arrayItems.begin();
	};
	/**
	 * @brief Returns an iterator to the end of the array.
	 * @return An iterator to the end of the array.
	 */
	typename std::vector<T *>::iterator end()
	{
		return arrayItems.end();
	};

	/**
	 * @brief Gets the number of items in the array.
	 * @return The number of items in the array.
	 */
	uint32_t size() { return arrayItems.size(); };
	/**
	 * @brief Pushes an item to the end of the array.
	 * @param item A pointer to the item to push.
	 * @return A pointer to the pushed item, or nullptr if the push fails.
	 */
	T *push(T *item)
	{
		if (item)
		{
			item->id = 1;
			arrayItems.push_back(item);
			return item;
		}
		return nullptr;
	};
	/**
	 * @brief Checks if the array contains the given item.
	 * @param item A pointer to the item to check for.
	 * @return True if the array contains the item, false otherwise.
	 */
	bool has(T *item)
	{
		auto it = std::find(arrayItems.begin(), arrayItems.end(), item);
		if (it != arrayItems.end())
			return true;
		return false;
	};
	/**
	 * @brief Removes an item from the array.
	 * @param item A pointer to the item to remove.
	 * @return True if the item was successfully removed, false otherwise.
	 */
	bool remove(T *item)
	{
		auto it = std::find(arrayItems.begin(), arrayItems.end(), item);

		if (it != arrayItems.end())
		{

			arrayItems.erase(it);

			// lo marcamos para reutilizacion
			item->id = Item::CREATE_NEW;

			return true;
		}

		return false;
	};
	/**
	 * @brief Clears all items in the array and resets the base data.
	 */
	void clear()
	{
		BaseData<N, T>::clear();
		arrayItems.clear();
	}

	/**
	 * @brief Removes the last item from the array.
	 * @return True if an item was successfully removed, false otherwise.
	 */
	bool pop()
	{
		if (size() <= 0)
			return false;
		// lo marcamos para reutilizacion
		arrayItems.back()->id = Item::CREATE_NEW;
		arrayItems.pop_back();
		return true;
	}
	/**
	 * @brief Gets the last item in the array.
	 * @return A pointer to the last item in the array.
	 */
	T *last()
	{
		return arrayItems.back();
	}
	/**
	 * @brief Gets the first item in the array.
	 * @return A pointer to the first item in the array.
	 */
	T *first()
	{
		return arrayItems.front();
	}

	/**
	 * @brief Accesses an item in the array by its index.
	 * @param index The index of the item to access.
	 * @return A pointer to the item at the given index, or nullptr if the index is out of bounds.
	 */
	T *operator[](uint index)
	{
		if (size() <= index)
			return nullptr;
		return arrayItems[index];
	}

	/**
	 * @brief Prints the IDs of all items in the array for debugging purposes.
	 */
	void printItems()
	{
		for (size_t i = 0; i < N; i++)
		{
			Serial.printf("%d item id: %d", i, this->items[i].id);
			if (has(&this->items[i]))
			{
				Serial.printf(" has arrayItems\n");
			}
			else
				Serial.printf(" not arrayItems \n");
		}
	};

	/**
	 * @brief Serializes the data in the array to a JSON array.
	 * @param root The JSON array to serialize the data to.
	 * @param extra A flag indicating whether to include extra data during serialization.
	 */
	void serializeData(JsonArray &root, bool extra = false)
	{

		for (T *item : arrayItems)
		{
			JsonObject o = root.add<JsonObject>();
			item->serializeItem(o, extra);
		}
	};
};

#endif