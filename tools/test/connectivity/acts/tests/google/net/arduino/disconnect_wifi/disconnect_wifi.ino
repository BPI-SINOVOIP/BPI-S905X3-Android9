#include <ESP8266WiFi.h>

void setup() {
    delay(1000); // wait for a second to read from serial port after flashing
    Serial.begin(9600);
    Serial.println("disconnect: setup(): CALL: Setup Begin");
    Serial.println("disconnect: setup(): INFO: Setting baud rate to 9600");

    wifiStatus();
    disconnectWifi();

    Serial.println("disconnect: setup(): CALL: Setup End");
}

void loop() {
    wifiStatus();
    scanNetworks();
}

void disconnectWifi() {
    Serial.println("disconnect: setup(): CALL: Disconnect Begin");
    WiFi.disconnect();
    while (WiFi.status() == WL_CONNECTED) {
        Serial.println("disconnect: setup(): INFO: WiFi connected");
        delay(1000);
    }
    Serial.println("disconnect: setup(): CALL: Disconnect End");
}

void wifiStatus() {
    Serial.println("disconnect: wifiStatus(): CALL: Status Begin");
    Serial.println("disconnect: loop(): INFO: WiFi disconnected");
    Serial.print("disconnect: wifiStatus(): STATUS: ");
    Serial.println(WiFi.status());
    Serial.print("disconnect: wifiStatus(): IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("disconnect: wifiStatus(): SSID: ");
    Serial.println(WiFi.SSID());
    delay(1000);
    Serial.println("disconnect: wifiStatus(): CALL: Status End");
}

void scanNetworks() {
    Serial.println("disconnect: scanNetworks(): CALL: Scan Begin");
    int n = WiFi.scanNetworks();
    if (n == 0) {
        Serial.println("disconnect: scanNetworks(): INFO: No networks found");
        Serial.println("disconnect: scanNetworks(): COUNT: 0");
    } else {
        Serial.println("disconnect: scanNetworks(): INFO: WiFi Networks Found");
        Serial.print("COUNT: ");
        Serial.println(n);

        for (int i = 0; i < n; ++i) {
            Serial.print("SSID: ");
            Serial.println(WiFi.SSID(i));
            Serial.print("RSSI: ");
            Serial.println(WiFi.RSSI(i));
        }
    }

    delay(5000); // Wait a bit before scanning again
    Serial.println("disconnect: scanNetworks(): CALL: Scan End");
}
