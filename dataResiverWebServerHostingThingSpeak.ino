/**
 * Receiver code for Heltec ESP32 LoRa V3 Module.
 * Receives temperature and humidity data via LoRa, hosts a web server, and sends data to ThingSpeak.
 */

#define HELTEC_POWER_BUTTON   // Must be before "#include <heltec_unofficial.h>"
#include <heltec_unofficial.h>
#include <WiFi.h>
#include <WebServer.h>

// LoRa parameters
#define FREQUENCY           866.3       // Must match transmitter
#define BANDWIDTH           125.0
#define SPREADING_FACTOR    7

String rxdata;
volatile bool rxFlag = false;

// Wi-Fi credentials
const char* ssid = "StarLink";          // Replace with your Wi-Fi SSID
const char* password = "00001111";      // Replace with your Wi-Fi password

// ThingSpeak API configuration
String apiKey = "DQEH8GZ8UAV15SR1";     // Your ThingSpeak Write API Key
const char* server = "api.thingspeak.com";

// Create a web server on port 8080
WebServer serverWeb(8080);

// Variables to store received temperature and humidity
String temperature = "N/A";
String humidity = "N/A";

// WiFi client for ThingSpeak
WiFiClient client;

void setup() {
  heltec_setup();
  both.println("LoRa Receiver Init");

  // Initialize LoRa radio
  RADIOLIB_OR_HALT(radio.begin());
  // Set the callback function for received packets
  radio.setDio1Action(rx);
  // Set radio parameters
  both.printf("Frequency: %.2f MHz\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  both.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  both.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  // Start receiving
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  both.println("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    both.print(".");
  }
  both.println();
  both.println("Wi-Fi connected.");

  both.print("IP address: ");
  both.println(WiFi.localIP());

  // Set up server routes
  serverWeb.on("/DATA", handleData);

  // Start the server on port 8080
  serverWeb.begin();
  both.println("Web server started on port 8080.");
  both.printf("Access the data at: http://%s:8080/DATA\n", WiFi.localIP().toString().c_str());
}

void loop() {
  heltec_loop();

  // Handle client requests
  serverWeb.handleClient();

  // If a packet was received, process it
  if (rxFlag) {
    rxFlag = false;
    radio.readData(rxdata);
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      // Parse the received data
      parseData(rxdata);

      // Display the formatted message
      both.println("Data Received Successfully");
      both.println("Temp: " + temperature + " C");
      both.println("Humd: " + humidity + " %");

      // Send data to ThingSpeak
      sendToThingSpeak(temperature, humidity);

      heltec_led(50); // Indicate reception
      delay(100);
      heltec_led(0);
    } else {
      both.printf("Receive failed (%i)\n", _radiolib_status);
    }
    // Restart the receiver
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }
}

// Interrupt callback for received packets
void rx() {
  rxFlag = true;
}

// Function to parse received data
void parseData(String data) {
  // Expected format: "Temp:xx.xC Hum:yy.y%"
  int tempIndex = data.indexOf("Temp:");
  int humIndex = data.indexOf("Hum:");

  if (tempIndex != -1 && humIndex != -1) {
    temperature = data.substring(tempIndex + 5, data.indexOf("C", tempIndex));
    humidity = data.substring(humIndex + 4, data.indexOf("%", humIndex));
  } else {
    both.println("Failed to parse data");
  }
}

// Function to send data to ThingSpeak
void sendToThingSpeak(String temp, String hum) {
  if (client.connect(server, 80)) {  // Connect to ThingSpeak server
    String postStr = apiKey;
    postStr += "&field1=" + temp;    // Field 1: Temperature
    postStr += "&field2=" + hum;    // Field 2: Humidity
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);

    both.println("Data sent to ThingSpeak:");
    both.println("  Temp: " + temp + " C");
    both.println("  Humd: " + hum + " %");
  } else {
    both.println("Failed to connect to ThingSpeak");
  }
  client.stop();
}

// Handler for /DATA
void handleData() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Data Received</title></head><body>";
  html += "<h1>Received Data</h1>";
  html += "<p>Temperature: " + temperature + " &deg;C</p>";
  html += "<p>Humidity: " + humidity + " %</p>";
  html += "</body></html>";

  serverWeb.send(200, "text/html", html);
}