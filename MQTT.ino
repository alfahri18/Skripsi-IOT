#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define DHT_PIN 26 // Pin yang terhubung ke sensor DHT22
#define DHT_TYPE DHT22 // Tipe sensor (DHT22 atau DHT11)
#define SOIL_MOISTURE_PIN 36 // Pin yang terhubung ke sensor kelembaban tanah
#define PH_SENSOR_PIN 2
#define RELAY_PIN_1 13
#define RELAY_PIN_2 12

const char* ssid = "";
const char* password = "";
const char* mqtt_server = "";
const int mqtt_port = 1883;
const char* mqtt_client_id = "YourClientID11";
int sensorValue = 0;
float phValue = 0.0;

DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient espClient;
PubSubClient client(espClient);

char jsonMessage[256]; // Variabel jsonMessage hanya dideklarasikan sekali

void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  // Define relay pins as OUTPUT
  pinMode(RELAY_PIN_1, OUTPUT);
  pinMode(RELAY_PIN_2, OUTPUT);

  // Matikan awal relay untuk memastikan solenoid valve dalam kondisi mati.
  // digitalWrite(RELAY_PIN_1, LOW);
  // digitalWrite(RELAY_PIN_2, LOW);

  setupWifi();
  client.setServer(mqtt_server, mqtt_port);
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
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_client_id)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
void loop() {
    client.subscribe("sprinklerEnabled");

  // Periksa pesan yang diterima
  client.loop();
}

// Fungsi callback untuk menangani pesan yang diterima dari topik "sprinklerEnabled"
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received in topic [");
  Serial.print(topic);
  Serial.print("]: ");
  
  // Ubah payload menjadi string
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // Cek pesan yang diterima dan atur sprinkler berdasarkan pesan
  if (message == "{\"sprinklerEnabled\":true}") {
    digitalWrite(RELAY_PIN_1, HIGH);
    digitalWrite(RELAY_PIN_2, HIGH);
    Serial.println("Sprinkler diaktifkan");
    // Tambahkan logika Anda di sini untuk menyalakan sprinkler
  } else if (message == "{\"sprinklerEnabled\":false}") {
    digitalWrite(RELAY_PIN_1, LOW);
    digitalWrite(RELAY_PIN_2, LOW);
    Serial.println("Sprinkler dinonaktifkan");
    // Tambahkan logika Anda di sini untuk mematikan sprinkler
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  delay(2000); // Delay 2 detik antara pembacaan
  int value = analogRead(SOIL_MOISTURE_PIN);
  sensorValue = analogRead(PH_SENSOR_PIN);
  int humidity = dht.readHumidity();
  int temperature = dht.readTemperature();
  int soilMoisture =  value;
  phValue = (-0.0139 * sensorValue) + 7.7851;

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Gagal membaca data dari sensor DHT22");
    return;
  }

  Serial.print("Kelembaban: ");
  Serial.print(humidity);
  Serial.print("%\t");
  Serial.print("Suhu: ");
  Serial.print(temperature);
  Serial.println("°C");

  Serial.print("Kelembaban Tanah: ");
  Serial.println(soilMoisture);

  sprintf(jsonMessage, "{\"soilPh\": %.1f}", phValue);
  client.publish("soilPh", jsonMessage);
  Serial.print("pH Sensor: ");
  Serial.println(phValue, 1);

  // Kirim data kelembaban tanah ke topik MQTT
  sprintf(jsonMessage, "{\"soilHumidity\": %d}", soilMoisture);
  client.publish("soilHumidity", jsonMessage);
  Serial.print("Kelembaban Tanah: ");
  Serial.println(soilMoisture);

  sprintf(jsonMessage, "{\"airHumidity\": %d}", static_cast<int>(humidity));
  client.publish("airHumidity", jsonMessage);
  Serial.print("Kelembaban Udara: ");
  Serial.println(humidity);

  sprintf(jsonMessage, "{\"airTemperature\": %d}", static_cast<int>(temperature));
  client.publish("airTemperature", jsonMessage);
  Serial.print("Suhu Udara: ");
  Serial.println(temperature);

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

  // Kirim status kualitas ke topik MQTT
  sprintf(jsonMessage, "{\"farmStatus\": \"%s\"}", statusKualitas.c_str());
  client.publish("farmStatus", jsonMessage);
  Serial.print("Status Kualitas: ");
  Serial.print(jsonMessage);

  // Subscribe ke topik "sprinklerEnabled"
}
