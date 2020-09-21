#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <WiFiUdp.h>

const char* ssid = "wifi_tethering_test";
const char* password = "password";

unsigned int localPort = 8888;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];
WiFiUDP Udp;


void setup() {
    delay(1000); // wait for a second to read from serial port after flashing
    Serial.begin(9600);
    Serial.println("connect: setup(): CALL: Setup Begin");
    Serial.println("connect: setup(): INFO: Setting baud rate to 9600");

    wifiStatus();
    connectWifi();

    Udp.begin(localPort);
    Serial.println("connect: setup(): CALL: Setup End");
}

void loop() {
    wifiStatus();
    udpPackets();
}

void connectWifi() {
    Serial.println("connect: connectWifi(): CALL: Connect Begin");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("connect: setup(): INFO: WiFi disconnected");
        delay(1000);
    }
    Serial.println("connect: connectWifi(): CALL: Connect End");
}

void wifiStatus() {
    Serial.println("connect: wifiStatus(): CALL: Status Begin");
    Serial.println("connect: wifiStatus(): INFO: WiFi connected");
    Serial.print("connect: wifiStatus(): STATUS: ");
    Serial.println(WiFi.status());
    Serial.print("connect: wifiStatus(): IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("connect: wifiStatus(): SSID: ");
    Serial.println(WiFi.SSID());
    bool ret = Ping.ping("www.google.com", 3);
    Serial.print("connect: wifiStatus(): PING: ");
    if (ret) {
        Serial.println("1");
    } else {
        Serial.println("0");
    }

    delay(250);
    Serial.println("connect: wifiStatus(): CALL: Status End");
}

void udpPackets() {
    Serial.println("connect: udpPackets(): CALL: UDP Begin");
    int packetSize = Udp.parsePacket();
    while(packetSize) {
        Serial.print("connect: udpPackets(): PKTSZ: ");
        Serial.println(packetSize);
        Serial.print("connect: udpPackets(): REMOTEIP: ");
        IPAddress remote = Udp.remoteIP();
        for (int i =0; i < 4; i++) {
            Serial.print(remote[i], DEC);
            if (i < 3) {
                Serial.print(".");
            }
        }
        Serial.println("");
        Serial.print("connect: udpPackets(): REMOTEPORT: ");
        Serial.println(Udp.remotePort());

        // read the packet into packetBufffer
        Udp.read(packetBuffer,UDP_TX_PACKET_MAX_SIZE);
        Serial.print("connect: udpPackets(): RECV: ");
        Serial.println(packetBuffer);

        // send the same message back
        Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
        Udp.write(packetBuffer);
        Udp.endPacket();

        packetSize = Udp.parsePacket();
    }
    Serial.println("connect: udpPackets(): CALL: UDP End");
}
