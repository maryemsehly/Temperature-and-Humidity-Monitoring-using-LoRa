
#define HELTEC_POWER_BUTTON   
#include <heltec_unofficial.h>
#include <DHTesp.h>

// DHT11 sensor configuration
#define DHTPIN 42        
#define DHTTYPE DHT11   
DHTesp dht;

// LoRa parameters
#define PAUSE               5  
#define FREQUENCY           866.3       
#define BANDWIDTH           125.0       
#define SPREADING_FACTOR    7           
#define TRANSMIT_POWER      14          

long counter = 0;
uint64_t last_tx = 0;
uint64_t tx_time;
uint64_t minimum_pause;

void setup() {
  heltec_setup();
  both.println("LoRa Transmitter Init");

  // Initialize DHT sensor
  dht.setup(DHTPIN, DHTesp::DHT11);

  // Initialize LoRa radio
  RADIOLIB_OR_HALT(radio.begin());
  // Set radio parameters
  both.printf("Frequency: %.2f MHz\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  both.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  both.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  both.printf("TX Power: %i dBm\n", TRANSMIT_POWER);
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
}

void loop() {
  heltec_loop();

  // Read temperature and humidity from DHT11
  TempAndHumidity data = dht.getTempAndHumidity();

  // Check if the reading failed
  if (isnan(data.temperature) || isnan(data.humidity)) {
    both.println("Failed to read from DHT sensor!");
    delay(2000);
    return;
  }

  // Prepare the message in a structured format
  String txMessage = "Temp:" + String(data.temperature, 1) + "C Hum:" + String(data.humidity, 1) + "% [" + String(counter++) + "]";

  // Transmit the message every PAUSE seconds
  if (millis() - last_tx > (PAUSE * 1000)) {
    both.printf("Sending at %.2f MHz: %s\n", FREQUENCY, txMessage.c_str());
    heltec_led(50); // 50% brightness for LED
    tx_time = millis();
    RADIOLIB(radio.transmit(txMessage.c_str()));
    tx_time = millis() - tx_time;
    heltec_led(0);

    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("Transmission successful (%i ms)\n", (int)tx_time);
    } else {
      both.printf("Transmission failed (%i)\n", _radiolib_status);
    }

    // Maximum 1% duty cycle
    minimum_pause = tx_time * 100;
    last_tx = millis();
  }
}
