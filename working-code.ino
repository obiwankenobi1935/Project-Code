#include <TinyGPSPlus.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Arduino_LSM6DS3.h> // For accelerometer
#include <ArduinoJson.h> // Include ArduinoJson library

// WiFi credentials
const char* ssid = "HAALAND 8082";
const char* password = "00000000";

// MQTT Broker settings
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// MQTT topic
const char* topic = "transport/sensorData";

// Define DHT sensor settings
#define DHTPIN 3            // DHT22 data pin
#define DHTTYPE DHT22       // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

// Create instances for TinyGPSPlus, MQTT client, and accelerometer
TinyGPSPlus gps;
WiFiClient espClient;
PubSubClient client(espClient);

// Last message timestamp
unsigned long lastMsg = 0;
const long interval = 2000; // Interval for publishing data (2 seconds)

void setupWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void setupMQTT() {
  client.setServer(mqtt_server, mqtt_port);
  while (!client.connected()) {
    if (client.connect("ArduinoClient")) {
      Serial.println("Connected to MQTT Broker");
    } else {
      Serial.print("Failed to connect. Error code: ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600); // GPS module connected to Serial1
  dht.begin(); // Initialize DHT sensor

  // Initialize accelerometer
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  setupWiFi();
  setupMQTT();
}

void loop() {
  if (!client.connected()) {
    setupMQTT();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > interval) {
    lastMsg = now;
    publishSensorData();
  }

  // Process incoming GPS data
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }
}

void publishSensorData() {
  // Check if GPS data is available
  float latitude = gps.location.isValid() ? gps.location.lat() : 0.0;
  float longitude = gps.location.isValid() ? gps.location.lng() : 0.0;
  float altitude = gps.altitude.isValid() ? gps.altitude.meters() : 0.0;

  // Read temperature and humidity from DHT22
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Read accelerometer data to calculate velocity
  float ax, ay, az;
  float velocity = 0.0;
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);
    velocity = sqrt(ax * ax + ay * ay + az * az);
  }

  // Create a JSON object
  StaticJsonDocument<200> doc;
  doc["latitude"] = latitude;
  doc["longitude"] = longitude;
  doc["altitude"] = altitude;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["velocity"] = velocity;

  // Serialize the JSON object to a string
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  // Publish the JSON string to the MQTT topic
  client.publish(topic, jsonBuffer); 
  Serial.println("Data sent: " + String(jsonBuffer));
} 
