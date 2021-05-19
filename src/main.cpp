#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
// #include <WiFi.h>
#include <ESP8266WiFi.h>
#include "CREDENTIALS"

#define SENDTOPIC "IR/key"
#define COMMANDTOPIC "IR/command"
#define SERVICETOPIC "IR/service"

const uint16_t IR_LED = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
const uint16_t STATUS_LED = 16;  // ESP8266 GPIO pin to use. Recommended: 4 (D0).

IRsend irsend(IR_LED);  // Set the GPIO to be used to sending the message.

// WiFi
WiFiClient wifiClient;
#ifdef __credentials__
    char *ssid      = SSID;               // Set you WiFi SSID
    char *password  = PASSWORD;           // Set you WiFi password
#else
    char *ssid      = "";               // Set you WiFi SSID
    char *password  = "";               // Set you WiFi password
#endif

//MQTT
IPAddress server(192, 168, 70, 233);

void callback(char* topic, byte* payload, unsigned int length) {
    String IRcommand = "";
    int i = 0;
    while (payload[i] > 0) {
        IRcommand = IRcommand + (char)payload[i];
        i++;
    }
    DynamicJsonDocument jsonBuffer(200);
    // JsonObject& root = jsonBuffer.parseObject(payload);
    auto error = deserializeJson(jsonBuffer, payload);

    // Test if parsing succeeds.
    if (error) {
        Serial.print(F("deserializeJson() failed with code "));
        Serial.println(error.c_str());
        return;
    }

    // int type = root["type"];
    // unsigned long valu = root["value"];
    // int len = root["length"];
    // Serial.println("");
    Serial.print("Payload ");
    Serial.println(IRcommand);
    // Serial.print("type ");
    // Serial.println(type);
    // Serial.print("value ");
    // Serial.println(valu);
    // Serial.print("length ");
    // Serial.println(len);
    // sendCode(type, valu, len);
}

PubSubClient client(server, 1883, callback, wifiClient);


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("UniversalRemote")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(SERVICETOPIC, "I am live again");
      // ... and resubscribe
      //  client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc = ");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void publishMQTT(String topic, String message) {
  if (!client.connected()) {
    reconnect();
  }
  client.publish(topic.c_str(), message.c_str());
}


void setup() {
    #if ESP8266
        Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
    #else  // ESP8266
        Serial.begin(115200, SERIAL_8N1);
    #endif  // ESP8266
    pinMode(STATUS_LED, OUTPUT);

    Serial.println("Connecting to WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin (ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println();
    Serial.println("WiFi connected");
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());  

    irsend.begin();

    // StaticJsonBuffer<200> jsonBuffer;
    StaticJsonDocument<200> jsonBuffer;

    // MQTT
    if (client.connect("UniversalRemote")) {
        client.publish(SERVICETOPIC, "IR Box live");
        client.subscribe(COMMANDTOPIC);
    }
    Serial.println("Setup done");
}

// ================================================================
// ===               Main loop                                  ===
// ================================================================
void loop() {
  client.loop();
}