

#define HELTEC_POWER_BUTTON   
#include <heltec_unofficial.h>
#include <WiFi.h>
#include <WebServer.h>

// LoRa parameters
#define FREQUENCY           866.3      
#define BANDWIDTH           125.0
#define SPREADING_FACTOR    7

String rxdata;
volatile bool rxFlag = false;

// Wi-Fi credentials
const char* ssid = "StarLink";
const char* password = "00001111";

// Create a web server on port 8080
WebServer server(8080);

// Variables to store received temperature and humidity
String temperature = "N/A";
String humidity = "N/A";

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
  server.on("/DATA", handleData);

  // Start the server on port 8080
  server.begin();
  both.println("Web server started on port 8080.");
  both.printf("Access the data at: http://%s:8080/DATA\n", WiFi.localIP().toString().c_str());
}

void loop() {
  heltec_loop();

  // Handle client requests
  server.handleClient();

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

// Handler for /DATA
void handleData() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Data Received</title></head><body>";
  html += "<h1>Received Data</h1>";
  html += "<p>Temperature: " + temperature + " &deg;C</p>";
  html += "<p>Humidity: " + humidity + " %</p>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}
