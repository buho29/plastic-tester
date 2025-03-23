#ifndef FILEJSONMANAGER
#define FILEJSONMANAGER

#include <LittleFS.h>
#include <DataTable.h>

/**
 * @file FileJsonManager.h
 * @brief Class for managing JSON files on LittleFS.
 *
 * @brief This class provides methods for reading, writing, and manipulating JSON files stored in the LittleFS file system.
 * 
 * @author buho29
 */
class FileJsonManager
{
public:

    /**
     * @brief Initializes the LittleFS file system.
     *
     * @return True if the file system was successfully mounted, false otherwise.
     */
    bool begin()
    {
        if (!LittleFS.begin(true))
        {
            Serial.println("An Error has occurred while mounting SPIFFS");
            return false;
        }
        return true;
    }
    /**
     * @brief Reads a JSON file from the specified path and deserializes it into an Iserializable object.
     *
     * @param path The path to the JSON file.
     * @param data A pointer to the Iserializable object to deserialize the JSON data into.
     * @return True if the file was successfully read and deserialized, false otherwise.
     */
    bool readJson(const char *path, Iserializable *data)
    {
        const String &str = readFile(path);
        if (str.length() > 0)
        {
            return data->deserializeData(str);
        }
        return false;
    }
    /**
     * @brief Reads a JSON file from the specified path and deserializes it into an Item object.
     *
     * @param path The path to the JSON file.
     * @param item A pointer to the Item object to deserialize the JSON data into.
     * @return True if the file was successfully read and deserialized, false otherwise.
     */
    bool readJson(const char *path, Item *item)
    {
        JsonDocument doc;
        const String &str = readFile(path);

        if (str.length() > 0)
        {
            // Parse
            DeserializationError error = deserializeJson(doc, str);
            if (!error)
            {
                JsonObject obj = doc.as<JsonObject>();
                return item->deserializeItem(obj);
            }
            else
            {
                Serial.println(error.f_str());
            }
        }
        Serial.printf("error reading file %s\n", path);
        return false;
    }

    /**
     * @brief Writes a JSON representation of an Iserializable object to a file.
     *
     * @param path The path to the file to write the JSON data to.
     * @param data A pointer to the Iserializable object to serialize.
     * @return True if the file was successfully written, false otherwise.
     */
    bool writeJson(const char *path, Iserializable *data)
    {
        const String &json = data->serializeString();
        return writeFile(path, json.c_str());
    }
    /**
     * @brief Writes a JSON representation of an Item object to a file.
     *
     * @param path The path to the file to write the JSON data to.
     * @param item A pointer to the Item object to serialize.
     * @return True if the file was successfully written, false otherwise.
     */
    bool writeJson(const char *path, Item *item)
    {
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();
        String str;

        item->serializeItem(obj, false);
        serializeJsonPretty(obj, str);
        return writeFile(path, str.c_str());
    }
    /**
     * @brief Deletes a file.
     *
     * @param file The path to the file to delete.
     * @return True if the file was successfully deleted, false otherwise.
     */
    bool deleteFile(const String &file)
    {
        if (LittleFS.exists(file) && LittleFS.remove(file))
        {
            Serial.println("- file deleted");
            return true;
        }
        else
        {
            Serial.println("- delete failed");
            return false;
        }
    }
    /**
     * @brief Exist a file.
     *
     * @param file The path to the file to delete.
     * @return True if the file exist, false otherwise.
     */
    bool exists(const String &file)
    {
        return LittleFS.exists(file);
    }   

    /**
     * @brief Renames a file.
     *
     * @param path1 The current path to the file.
     * @param path2 The new path for the file.
     * @return True if the file was successfully renamed, false otherwise.
     */
    bool renameFile(const char *path1, const char *path2)
    {
        Serial.printf("Renaming file %s to %s\r\n", path1, path2);
        if (LittleFS.rename(path1, path2))
        {
            Serial.println("- file renamed");
            return true;
        }
        else
        {
            Serial.println("- rename failed");
            return false;
        }
    }
    private:
    /**
     * @brief Writes text to a file.
     *
     * @param path The path to the file to write to.
     * @param text The text to write to the file.
     * @return True if the file was successfully written, false otherwise.
     */
    bool writeFile(const char *path, const char *text)
    {
        uint tim = millis();
        File file = LittleFS.open(path, FILE_WRITE);
        if (!file)
        {
            Serial.printf("- failed to open file for writing path: %s\n", path);
            return false;
        }
        if (file.print(text))
        {
            uint elapsed = (millis() - tim);
            file.close();
            Serial.printf("- file written in %dms: %s len: %d\n", elapsed, path, strlen(text));
            return true;
        }
        Serial.println("- write failed");
        return false;
    }
    /**
     * @brief Reads the content of a file into a String.
     *
     * @param path The path to the file to read.
     * @return The content of the file as a String. Returns an empty String if the file could not be read.
     */
    String readFile(const char *path)
    {
        File file = LittleFS.open(path, "r"); // Abrir en modo lectura
        String str = "";

        if (!file || file.isDirectory())
        {
            Serial.printf("readJson - failed to open %s for reading\n", path);
            return str;
        }

        const size_t bufferSize = 512;
        char buffer[bufferSize];

        while (file.available())
        {
            size_t bytesRead = file.readBytes(buffer, bufferSize);
            str.concat(buffer, bytesRead); // Concatenar el buffer al String
        }

        file.close();
        return str;
    }

    /**
     * @brief Lists the contents of a directory.
     *
     * @param dirname The path to the directory to list.
     * @param levels The number of directory levels to list recursively.
     */
    void listDir(const char *dirname, uint8_t levels)
    {
        Serial.printf("Listing directory: %s\r\n", dirname);

        File root = LittleFS.open(dirname);
        if (!root)
        {
            Serial.println("- failed to open directory");
            return;
        }
        if (!root.isDirectory())
        {
            Serial.println(" - not a directory");
            return;
        }

        File file = root.openNextFile();
        while (file)
        {
            if (file.isDirectory())
            {
                Serial.print("  DIR : ");
                Serial.println(file.name());
                if (levels)
                {
                    listDir(file.name(), levels - 1);
                }
            }
            else
            {
                Serial.print("  FILE: ");
                Serial.print(file.name());
                Serial.print("  SIZE: ");
                Serial.println(file.size());
            }
            file = root.openNextFile();
        }
    }
};
#endif
