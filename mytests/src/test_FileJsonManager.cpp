#include "FileJsonManager.h"
#include <AUnit.h>
using aunit::TestRunner;

struct TestItem : public Item {
public:
    int value = 0;
    String name;

    bool deserializeItem(JsonObject& obj) override {
        value = obj["value"];
        name = obj["name"].as<String>();
        return true;
    }

    void serializeItem(JsonObject& obj, bool minimal) override {
        obj["value"] = value;
        obj["name"] = name;
    }
};

test(FileJsonManager_begin) {
    FileJsonManager manager;
    assertTrue(manager.begin());
}

test(FileJsonManager_writeAndReadJson) {
    FileJsonManager manager;
    manager.begin();

    // Test with Item
    TestItem writeItem;
    writeItem.value = 42;
    writeItem.name = "test";
    
    assertTrue(manager.writeJson("/test.json", &writeItem));
    
    TestItem readItem;
    assertTrue(manager.readJson("/test.json", &readItem));
    assertEqual(readItem.value, 42);
    assertEqual(readItem.name, "test");
    assertTrue(manager.deleteFile(String("/test.json")));
}

test(FileJsonManager_deleteFile) {
    FileJsonManager manager;
    manager.begin();

    String testFile = "/delete_test.json";
    TestItem item;
    item.value = 1;
    
    assertTrue(manager.writeJson(testFile.c_str(), &item));
    assertTrue(manager.exists(testFile));
    assertTrue(manager.deleteFile(testFile));
    assertFalse(manager.exists(testFile));
}

test(FileJsonManager_readNonexistentFile) {
    FileJsonManager manager;
    manager.begin();

    TestItem item;
    assertFalse(manager.readJson("/nonexistent.json", &item));
}

// Verify file operations with invalid paths
test(FileJsonManager_invalidPaths) {
    FileJsonManager manager;
    manager.begin();

    TestItem item;
    assertFalse(manager.readJson("", &item));
    assertFalse(manager.writeJson("", &item));
}

// Run the tests
void setup() {
    Serial.begin(115200);
    while(!Serial); // Wait for serial port
    delay(1000);
}

void loop() {
    aunit::TestRunner::run();
}