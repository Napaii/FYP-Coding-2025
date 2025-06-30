#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h> 
#include "ThingSpeak.h"  

#define ONE_WIRE_BUS D2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

const float Vin = 3.3; 
const float Rfixed = 330.0; 
const float analogPin = A0; 

// WiFi Setup
const char* ssid = "Pie";
const char* password = "Napie001";

// ThingSpeak Configuration
// Ensure your ThingSpeak channel has at least 2 fields enabled for this version:
// Field 1: Measured Temperature
// Field 2: Measured Sample Resistance
unsigned long myChannelNumber = 2958251;
const char* myWriteAPIKey = "JJLT5AF5B5CMCZ8C";
WiFiClient client;

void setup() {
  Serial.begin(9600);
  delay(1000); 

  sensors.begin();


  // Connect to Wi-Fi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Time (s) \tTemperature (C) \tResistance (Ohm) \tAnalog Value (ADC)");

  // Initialize ThingSpeak
  ThingSpeak.begin(client);
}

void loop() {
  unsigned long currentTime = millis();

  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0); 

  int adcVal = analogRead(analogPin);
  float Vout = (adcVal* Vin / 1023.0) ;
  float Rsample_measured = (Vout * Rfixed) / (Vin - Vout);

  Serial.print(currentTime / 1000.0, 2);
  Serial.print("\t \t \t"); 
  Serial.print(temperatureC, 2); 
  Serial.print("\t \t \t"); 
  Serial.print(Rsample_measured, 3); 
  Serial.print("\t \t \t"); 
  Serial.println(adcVal); 

  // Write to ThingSpeak
  // Set Field 1 to temperatureC
  ThingSpeak.setField(1, temperatureC); 
  // Set Field 2 to Rsample_measured
  ThingSpeak.setField(2, Rsample_measured); 

  // Write the fields to the ThingSpeak channel
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);


  delay(20000); 
}