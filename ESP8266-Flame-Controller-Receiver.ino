#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "wifi_credentials.h"

WiFiClient espClient;
PubSubClient client(espClient);

#define HSI_INPUT D1
#define HSI_LED D5
#define POOF_INPUT D2
#define POOF_LED D6
#define WIFI_LED D7
#define MQTT_LED D8

void callback(char* topic, byte* payload, unsigned int length) {
 Serial.print("Message arrived [");
 Serial.print(topic);
 Serial.print("] ");
 for (int i=0;i<length;i++) {
  char receivedChar = (char)payload[i];
  Serial.print(receivedChar);
  // if (receivedChar == '0')
  // ESP8266 Huzzah outputs are "reversed"
  // digitalWrite(ledPin, HIGH);
  // if (receivedChar == '1')
  // digitalWrite(ledPin, LOW);
  }
  Serial.println();
}
 
 
void connectMqtt() {
  digitalWrite(MQTT_LED, LOW);
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266 Client")) {
      digitalWrite(MQTT_LED, HIGH);
      Serial.println("connected");
      // client.subscribe("ledStatus");
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
  pinMode(HSI_LED, OUTPUT);
  pinMode(POOF_INPUT, INPUT_PULLUP);
  pinMode(POOF_LED, OUTPUT);
  pinMode(WIFI_LED, OUTPUT);
  pinMode(MQTT_LED, OUTPUT);
  client.setServer(mqtt_server, 1883);
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
  if (digitalRead(HSI_INPUT) == digitalRead(HSI_LED)) {
    digitalWrite(HSI_LED, digitalRead(HSI_INPUT) ? LOW : HIGH);
    Serial.printf("HSI ignite %s\n", digitalRead(HSI_INPUT) ? "off" : "on");
  }
  if (digitalRead(POOF_INPUT) == digitalRead(POOF_LED)) {
    digitalWrite(POOF_LED, digitalRead(POOF_INPUT) ? LOW : HIGH);
    Serial.printf("Poof! %s\n", digitalRead(POOF_INPUT) ? "off" : "on");
  }
}
