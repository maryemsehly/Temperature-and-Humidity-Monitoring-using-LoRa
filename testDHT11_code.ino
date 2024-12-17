#include <DHTesp.h>

#define DHTPIN 42        // GPIO pin connected to DHT11 //GPIO 21  GPIO 22 GPIO 13  GPIO 27  GPIO 32  GPIO 33  GPIO 23  GPIO 25  GPIO 14  GPIO 12//
#define DHTTYPE DHT11    // DHT 11

DHTesp dht;

void setup() {
  Serial.begin(115200); // Initialize Serial Monitor
  delay(2000);          // Wait for the sensor to stabilize
  dht.setup(DHTPIN, DHTesp::DHT11); // Initialize DHT sensor
}

void loop() {
  // Read temperature and humidity
  TempAndHumidity data = dht.getTempAndHumidity();

  // Check if the reading failed
  if (isnan(data.temperature) || isnan(data.humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Display readings on Serial Monitor
  Serial.print("Temperature: ");
  Serial.print(data.temperature);
  Serial.print("°C  Humidity: ");
  Serial.print(data.humidity);
  Serial.println("%");

  // Wait a few seconds between measurements
  delay(2000);
}