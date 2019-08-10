#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <elapsedMillis.h>
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

typedef struct poofInfo_t {
  int state = 0;
  int expiration = 0;
  int duration = 0;
};

typedef struct buttonState_t {
  uint8_t state = 0;
  uint8_t stateLast = 0;
  uint32_t debounce = 0;
  const uint8_t debounceTime = 10;
  char* label;
};

poofInfo_t poofInfo;
buttonState_t hsiButton;
buttonState_t poofButton;

uint8_t wifiConnecting = 0;
uint8_t wifiFirstConnected = 0;

elapsedMillis mqttLoop = 5000;

void callback(char* topic, byte* payload, uint8_t length) {
  for (uint8_t i = 0; i < length; i++) Serial.print((char)payload[i]);
  Serial.println();
  DeserializationError error = deserializeJson(cmdJSON, payload, length);
  if (error) return;
  if (cmdJSON["cmd"] == "poof") {
    if (poofInfo.state != 1) {
      poofInfo.duration = cmdJSON["delay"];
      poofInfo.state = 1;
    }
  }
}

void networkSetup() {
  if (WiFi.status() != WL_CONNECTED) {
    if (!wifiConnecting) {
      wifiConnecting = 1;
      digitalWrite(WIFI_LED, LOW);
      Serial.println("");
      Serial.println("Connecting to WiFi ...");
      WiFi.begin(ssid, password);
    }
  } else {
    if (wifiConnecting && !wifiFirstConnected) {
        wifiFirstConnected = 1;
        wifiConnecting = 0;
        digitalWrite(WIFI_LED, HIGH);
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP()); 
    } else if (!client.connected()) {
      digitalWrite(MQTT_LED, LOW);
      if (mqttLoop > 5000) {
        mqttLoop = 0;
        Serial.print("Attempting MQTT connection...");
        if (client.connect(mqtt_clientId)) {
          digitalWrite(MQTT_LED, HIGH);
          Serial.println("connected");
          client.subscribe(mqtt_topic);
        } else {
          Serial.print("failed, rc=");
          Serial.print(client.state());
          Serial.println(" try again in 5 seconds");
        }
      }
    }
  }
}

void setup() {
  hsiButton.label = "HSI";
  poofButton.label = "Poofer";
  Serial.begin(115200);
  pinMode(HSI_INPUT, INPUT_PULLUP);
  pinMode(HSI_TRIGGER, OUTPUT);
  pinMode(POOF_INPUT, INPUT_PULLUP);
  pinMode(POOF_TRIGGER, OUTPUT);
  pinMode(WIFI_LED, OUTPUT);
  pinMode(MQTT_LED, OUTPUT);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  delay(10);
}

void buttonDebounce(buttonState_t *button, uint8_t trigger, uint8_t input) {
  button->state = digitalRead(input);
  if (button->state != button->stateLast && millis() - button->debounce > button->debounceTime) {
    button->stateLast = button->state;
    button->debounce = millis();
    if (button->state == LOW) {
      digitalWrite(trigger, HIGH);
      Serial.printf("%s ignite on \n", button->label);
    } else {
      digitalWrite(trigger, LOW);
      Serial.printf("%s ignite off \n", button->label);
    }
  }
}

void waxOn() {
  if (poofInfo.state && poofInfo.duration && !poofInfo.expiration) {
    poofInfo.expiration = millis() + poofInfo.duration;
    poofInfo.duration = 0;
    Serial.println("[REMOTE] Poof!");
    digitalWrite(POOF_TRIGGER, HIGH);
  }
}

void waxOff() {
  if (poofInfo.state && poofInfo.expiration <= millis()) {
    Serial.println("[REMOTE] fooP!");
    digitalWrite(POOF_TRIGGER, LOW);
    poofInfo.expiration = 0;    
    poofInfo.state = 0;
  }
}

void loop() {
  networkSetup();
  client.loop();
  buttonDebounce(&hsiButton, HSI_TRIGGER, HSI_INPUT);
  if (!poofInfo.state) buttonDebounce(&poofButton, POOF_TRIGGER, POOF_INPUT);
  waxOn();
  waxOff();
}
