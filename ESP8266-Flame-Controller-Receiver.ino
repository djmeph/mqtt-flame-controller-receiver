#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "wifi_credentials.h"

WiFiClient espClient;
PubSubClient client(espClient);
StaticJsonDocument<128> cmdJSON;

#define HSI_INPUT D1
#define HSI_TRIGGER D5
#define POOF_INPUT D2
#define POOF_TRIGGER D6
#define WIFI_LED D7
#define MQTT_LED D8

int poofState = 0;
int expiration = 0;
int duration = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  String command = String((char*)payload);
  Serial.printf("Message arrived [%s] %s\n", topic, command.c_str());
  DeserializationError error = deserializeJson(cmdJSON, command);
  if (error) {
    return;
  }
  String cmd = cmdJSON["cmd"];
  if (cmd == "poof") {    
    if (poofState != 1) {
      duration = cmdJSON["delay"];
      poofState = 1;      
    }
  }
}
 
 
void connectMqtt() {
  digitalWrite(MQTT_LED, LOW);
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_clientId)) {
      digitalWrite(MQTT_LED, HIGH);
      Serial.println("connected");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setupWifi() {
  digitalWrite(WIFI_LED, LOW);
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid); 
  WiFi.begin(ssid, password);
   
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  digitalWrite(WIFI_LED, HIGH);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  pinMode(HSI_INPUT, INPUT_PULLUP);
  pinMode(HSI_TRIGGER, OUTPUT);
  pinMode(POOF_INPUT, INPUT_PULLUP);
  pinMode(POOF_TRIGGER, OUTPUT);
  pinMode(WIFI_LED, OUTPUT);
  pinMode(MQTT_LED, OUTPUT);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    setupWifi();
  }
  if (!client.connected()) {
    connectMqtt();
  }
  client.loop();
  if (digitalRead(HSI_INPUT) == digitalRead(HSI_TRIGGER)) {
    digitalWrite(HSI_TRIGGER, digitalRead(HSI_INPUT) ? LOW : HIGH);
    Serial.printf("HSI ignite %s\n", digitalRead(HSI_INPUT) ? "off" : "on");
  }
  if (!poofState && digitalRead(POOF_INPUT) == digitalRead(POOF_TRIGGER)) {
    digitalWrite(POOF_TRIGGER, digitalRead(POOF_INPUT) ? LOW : HIGH);
    Serial.printf("Poof! %s\n", digitalRead(POOF_INPUT) ? "off" : "on");
  }
  if (poofState && duration && !expiration) {
    expiration = millis() + duration;
    duration = 0;
    Serial.println("[REMOTE] Poof!");
    digitalWrite(POOF_TRIGGER, HIGH);
  }
  if (poofState && expiration <= millis()) {
    Serial.println("[REMOTE] fooP!");
    digitalWrite(POOF_TRIGGER, LOW);
    expiration = 0;    
    poofState = 0;
  }
}
