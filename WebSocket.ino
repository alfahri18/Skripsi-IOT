#include <DHT.h>
#include <WiFi.h>
#include <ArduinoWebsockets.h>

#define DHT_PIN 26 // Pin yang terhubung ke sensor DHT22
#define DHT_TYPE DHT22 // Tipe sensor (DHT22 atau DHT11)
#define SOIL_MOISTURE_PIN 36 // Pin yang terhubung ke sensor kelembaban tanah
#define PH_SENSOR_PIN 2
#define RELAY_PIN_1 13
#define RELAY_PIN_2 12

const char* ssid = "KazuyoSan";
const char* password = "akuajayangtau";
const char* websocket_server_host = "";
const uint16_t websocket_server_port = 80;

int sensorValue = 0;
float phValue = 0.0;

DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient wifiClient;
WebsocketsClient websocketsClient;

char jsonMessage[256]; // Variabel jsonMessage hanya dideklarasikan sekali

void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  // Define relay pins as OUTPUT
  pinMode(RELAY_PIN_1, OUTPUT);
  pinMode(RELAY_PIN_2, OUTPUT);

  // Matikan awal relay untuk memastikan solenoid valve dalam kondisi mati.
  digitalWrite(RELAY_PIN_1, LOW);
  digitalWrite(RELAY_PIN_2, LOW);

  setupWifi();
  websocketsClient.onMessage(onMessage);
}

void setupWifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");

  // Connect to WebSocket server
  websocketsClient.connect(websocket_server_host, websocket_server_port);
}

void onMessage(WebsocketsMessage message) {
  Serial.print("Got Message: ");
  Serial.println(message.data());

  // Handle messages here if needed
}

void loop() {
  websocketsClient.loop();
  delay(2000); // Delay 2 detik antara pembacaan
  int value = analogRead(SOIL_MOISTURE_PIN);
  sensorValue = analogRead(PH_SENSOR_PIN);
  int humidity = dht.readHumidity();
  int temperature = dht.readTemperature();
  int soilMoisture =  value;
  phValue = (-0.0139 * sensorValue) + 7.7851;

  // Send sensor data to WebSocket server
  if (websocketsClient.available()) {
    sprintf(jsonMessage, "{\"soilPh\": %.1f}", phValue);
    websocketsClient.send(jsonMessage);

    sprintf(jsonMessage, "{\"soilHumidity\": %d}", soilMoisture);
    websocketsClient.send(jsonMessage);

    sprintf(jsonMessage, "{\"airHumidity\": %d}", humidity);
    websocketsClient.send(jsonMessage);

    sprintf(jsonMessage, "{\"airTemperature\": %d}", temperature);
    websocketsClient.send(jsonMessage);

    // Logika untuk menentukan status kualitas
    String statusKualitas;
    if (humidity >= 40 && humidity <= 60 && temperature >= 20 && temperature <= 30 &&
        soilMoisture >= 200 && soilMoisture <= 800 && phValue >= 6 && phValue <= 7) {
      statusKualitas = "Sangat Baik";
    } else if (humidity >= 30 && humidity <= 70 && temperature >= 15 && temperature <= 35 &&
               soilMoisture >= 100 && soilMoisture <= 1000 && phValue >= 5 && phValue <= 8) {
      statusKualitas = "Normal";
    } else {
      statusKualitas = "Buruk";
    }

    sprintf(jsonMessage, "{\"farmStatus\": \"%s\"}", statusKualitas.c_str());
    websocketsClient.send(jsonMessage);
  }
}
