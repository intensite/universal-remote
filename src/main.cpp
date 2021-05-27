#include "OTA.h"
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
const int MULTICODE_DELAY = 500;       // Delay in milliseconds between multicode commands

IRsend irsend(IR_LED);  // Set the GPIO to be used to sending the message.
void sendCode( int codeType, unsigned long codeValue, int repeat );

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

// Process the received MQTT Messages
void callback(char* topic, byte* payload, unsigned int length) {
  digitalWrite(STATUS_LED, HIGH);
  
    String IRcommand = "";
    int i = 0;
    while (payload[i] > 0) {
        IRcommand = IRcommand + (char)payload[i];
        i++;
    }
    DynamicJsonDocument json(300);
    auto error = deserializeJson(json, payload);

    // Test if parsing succeeds.
    if (error) {
        Serial.print(F("deserializeJson() failed with code "));
        Serial.println(error.c_str());
        return;
    }

    // Check if json contains an array
    JsonArray command_array = json.as<JsonArray>();

    if(command_array) {

      Serial.println("Seems to be an array of commands ");

      // using C++11 syntax (preferred):
      for (JsonVariant value : command_array) {
          JsonObject command = value.as<JsonObject>();
          unsigned long type = command["type"];
          unsigned long valu =  strtoul(command["value"], (char**)0, 0);  // Conver from string to ulong
          
          int repeat = command["repeat"];

          // Serial.print("type ");
          // Serial.println(type);
          // Serial.print("value ");
          // Serial.println(valu);
          // Serial.print("repeat ");
          // Serial.println(repeat);
          sendCode(type, valu, repeat);
          delay(MULTICODE_DELAY);

      }


    } else {

      int type = json["type"];
      // unsigned long valu = json["value"];
      unsigned long valu =  strtoul(json["value"], (char**)0, 0);  // Conver from string to ulong
      int repeat = json["repeat"];
      // Serial.print("Payload ");
      // Serial.println(IRcommand);
      // Serial.print("type ");
      // Serial.println(type);
      // Serial.print("value ");
      // Serial.println(valu);
      // Serial.print("repeat ");
      // Serial.println(repeat);
      sendCode(type, valu, repeat);
    }
    digitalWrite(STATUS_LED, LOW);
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

    if(USE_OTA_UPDATE)
        setupOTA("U-Remote", SSID, PASSWORD);


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

    // Staticjson<200> json;
    StaticJsonDocument<200> json;

    // MQTT
    if (client.connect("UniversalRemote")) {
      char str[40];
      strcpy(str,"IR Box live at: ");
      strcat(str,  WiFi.localIP().toString().c_str());
        client.publish(SERVICETOPIC, str);
        client.subscribe(COMMANDTOPIC);
    }
    Serial.println("Setup done");
}

// ================================================================
// ===               Main loop                                  ===
// ================================================================
void loop() {
  ArduinoOTA.handle();
  client.loop();
}


void sendCode( int codeType, unsigned long codeValue, int repeat ) {
  int codeLen = 32;

  if (codeType == SAMSUNG) {
    // Protocol Type 7
    irsend.sendSAMSUNG(codeValue, codeLen, repeat);
    Serial.print("Sent SAMSUNG ");
    Serial.println(codeValue, HEX);
  }
  else if (codeType == RCMM) {
    // Protocol Type 21
    irsend.sendRCMM(codeValue, codeLen, repeat);
    Serial.print("Sent RCMM ");
    Serial.println(codeValue, HEX);
  }
  
}