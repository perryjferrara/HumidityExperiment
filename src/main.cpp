#include <Arduino.h>
#include "DHT.h"
#include <ArduinoJson.h>
#include <WiFi.h>          // Required for ESP32 Wi-Fi
#include <PubSubClient.h>  // Required for MQTT (Install via Library Manager)

// --- Pin Configurations ---
#define LED_PIN 32
#define VOUT_PIN 25
#define DHTPIN 22     
#define DHTTYPE DHT11   

// --- Wi-Fi & MQTT Configurations ---
const char* ssid     = "iPhone";     // Put your Wi-Fi name here
const char* password = "himomha123"; // Put your Wi-Fi password here

// The MQTT Broker address (Change this to your actual broker)
const char* mqtt_server = "broker.hivemq.com"; 
const int mqtt_port = 1883;

// Your specific MQTT Topic
const char* mqtt_topic = "csc113/ambientenv/";

// --- Object Initializations ---
DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

// Function to handle Wi-Fi connection
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// Function to handle MQTT reconnection
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() 
{
  Serial.begin(9600);
  while (!Serial) continue; 
  
  pinMode(LED_PIN, OUTPUT);
  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  Serial.println("before dht begin");
  dht.begin();
}

void measureTemp() {
  float h = dht.readHumidity();
  float t = dht.readTemperature(); 
  float f = dht.readTemperature(true); 

  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  float hif = dht.computeHeatIndex(f, h);
  float hic = dht.computeHeatIndex(t, h, false);

  // 1. Build your JSON structure
  JsonDocument doc; 
  doc["ID"] = "PF";  // Added to the very beginning of the JSON
  doc["humidity"] = h;
  
  JsonObject temperature = doc["temperature"].to<JsonObject>();
  temperature["celsius"] = t;
  temperature["fahrenheit"] = f;

  JsonObject gps = doc["gps"].to<JsonObject>();
  gps["lat"] = 48.8584;
  gps["lon"] = 2.2945;

  // 2. Convert JSON object into a string buffer for MQTT
  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);

  // 3. Publish the JSON string to your MQTT topic
  Serial.print("Publishing to topic [");
  Serial.print(mqtt_topic);
  Serial.print("]: ");
  Serial.println(jsonBuffer);
  
  if (client.publish(mqtt_topic, jsonBuffer)) {
    Serial.println("Publish successful!");
  } else {
    Serial.println("Publish failed. Is the payload too large or broker down?");
  }
}

void loop() 
{
  // Keep the MQTT client connection alive
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  delay(1000);
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);
  
  measureTemp();
}