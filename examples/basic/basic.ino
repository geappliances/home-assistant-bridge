#include <Arduino.h>
#include <PubSubClient.h>
#include "HomeAssistantBridge.h"

static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);
static HomeAssistantBridge bridge;

static void configureWifi()
{
  Serial.println("Connecting to " + String(SSID));

  WiFi.begin(SSID, PASSWORD);

  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
}

static void configureMqtt()
{
  mqttClient.setServer(MQTT_SERVER, MQTT_SERVER_PORT);
}

static void connectToMqtt()
{
  while(!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if(mqttClient.connect("", MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
    }
    else {
      Serial.println("failed, rc=" + String(mqttClient.state()) + " will try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  configureWifi();
  configureMqtt();
  bridge.begin(mqttClient, Serial1, "some-device-id");
}

void loop()
{
  connectToMqtt();
  bridge.loop();
}
