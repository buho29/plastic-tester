#include <NetworkManager.h>

NetworkManager network;

void setup() {
    Serial.begin(115200);
    network.begin("miHostName");
    network.connect("Zyxel_E49C", "^t!pcm774K");
}

void loop() {
    network.update();
}
