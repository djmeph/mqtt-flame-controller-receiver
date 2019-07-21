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

uint8_t hsi_button_state;
int hsi_button_state_last = -1;
int hsi_debounce = 0;
const int hsi_debounce_time = 10;

uint8_t poof_button_state;
int poof_button_state_last = -1;
int poof_debounce = 0;
const int poof_debounce_time = 10;

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

void hsiButton() {
  hsi_button_state = digitalRead(HSI_INPUT);
  if (hsi_button_state != hsi_button_state_last && millis() - hsi_debounce > hsi_debounce_time) {
    hsi_button_state_last = hsi_button_state;
    hsi_debounce = millis();
    if (hsi_button_state == LOW) {
      digitalWrite(HSI_TRIGGER, HIGH);
      Serial.println("HSI ignite on");
    } else {
      digitalWrite(HSI_TRIGGER, LOW);
      Serial.println("HSI ignite off");
    }
  }
}

void poofButton() {
  if (poofState) return;
  poof_button_state = digitalRead(POOF_INPUT);
  if (poof_button_state != poof_button_state_last && millis() - poof_debounce > poof_debounce_time) {
    poof_button_state_last = poof_button_state;
    poof_debounce = millis();
    if (poof_button_state == LOW) {
      digitalWrite(POOF_TRIGGER, HIGH);
      Serial.println("POOF ignite on");
    } else {
      digitalWrite(POOF_TRIGGER, LOW);
      Serial.println("POOF ignite off");
    }
  }
}

void waxOn() {
  if (poofState && duration && !expiration) {
    expiration = millis() + duration;
    duration = 0;
    Serial.println("[REMOTE] Poof!");
    digitalWrite(POOF_TRIGGER, HIGH);
  }
}

void waxOff() {
  if (poofState && expiration <= millis()) {
    Serial.println("[REMOTE] fooP!");
    digitalWrite(POOF_TRIGGER, LOW);
    expiration = 0;    
    poofState = 0;
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) setupWifi();
  if (!client.connected()) connectMqtt();
  client.loop();
  hsiButton();
  poofButton();
  waxOn();
  waxOff();
}
