#include <WiFi.h>
#include "ThingSpeak.h"

// WiFi credentials
const char* ssid = "wifiusername";
const char* password = "wifipassword";

// ThingSpeak settings
unsigned long myChannelNumber = 2958251;
const char* myWriteAPIKey = "apikey";

// GPIO setup
const int tempPin = 35;      // LM35 sensor (ADC1_7)
const int voltagePin = 34;   // Voltage divider from material wire (ADC1_6)
const int sampleDuration = 10000; // milliseconds
const int sampleInterval = 1;     // ms between samples
const int numSamples = sampleDuration / sampleInterval;

WiFiClient client;

// Material settings
String material = "Nichrome";  // Change to Titanium, Tungsten, Nichrome

float alpha, resistivity;

const float diameter = 0.0002;  // 0.2 mm
const float length = 0.1;       // 10 cm
const float Vin = 3.3;
const float Rfixed = 1000.0;
const float t1 = 20.0;

void setMaterialConstants(String mat) {
  if (mat == "Copper") {
    alpha = 0.00393;
    resistivity = 1.68e-8;
  } else if (mat == "Titanium") {
    alpha = 0.0035;
    resistivity = 4.20e-7;
  } else if (mat == "Tungsten") {
    alpha = 0.0045;
    resistivity = 5.60e-8;
  } else if (mat == "Nichrome") {
    alpha = 0.0004;
    resistivity = 1.10e-6;
  } else {
    alpha = 0.00393;
    resistivity = 1.68e-8;
  }
}

void setup() {
  Serial.begin(9600);
  delay(1000);

  setMaterialConstants(material);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP:");
  Serial.println(WiFi.localIP());

  ThingSpeak.begin(client);
}

void loop() {
  float sumTemp = 0, sumVoltage = 0;

  for (int i = 0; i < numSamples; i++) {
    // Read LM35 temp
    int tempADC = analogRead(tempPin);
    float tempVolt = tempADC * (3.3 / 4095.0);
    float temperatureC = tempVolt * 100;  // Calibrated factor
    sumTemp += temperatureC;

    // Read voltage from voltage divider
    int voltADC = analogRead(voltagePin);
    float voltage = voltADC * (3.3 / 4095.0);
    sumVoltage += voltage;

    delay(sampleInterval);
  }

  float avgTemp = sumTemp / numSamples;
  float avgVoltage = sumVoltage / numSamples;

  Serial.println("---------- Measurement ----------");
  Serial.print("Material: "); Serial.println(material);
  Serial.print("Avg Temperature (°C): "); Serial.println((int)avgTemp);
  Serial.print("Avg Vout (V): "); Serial.println(avgVoltage, 4);

  // Safety check: voltage must be less than Vin
  if (avgVoltage >= Vin || avgVoltage <= 0.01) {
    Serial.println("Invalid voltage: Vout >= Vin or too low. Check wiring!");
    Serial.println("----------------------------------");
    delay(200);
    return;
  }

  float Rsample = (avgVoltage * Rfixed) / (Vin - avgVoltage);
  float Rt = Rsample * (1 + alpha * (avgTemp - t1));
  float area = PI * pow(diameter / 2.0, 2);
  float rho = Rsample * area / length;
  float R0_est = resistivity * length / area;

  Serial.print("Avg Resistance R_t1 (Ω): "); Serial.println(Rsample, 4);
  Serial.print("Temperature corrected Resistance Rt (Ω): "); Serial.println(Rt, 4);
  Serial.print("Avg Resistivity (Ω·m): "); Serial.println(rho, 10);
  Serial.print("Estimated R0 from resistivity (Ω): "); Serial.println(R0_est, 4);
  Serial.println("----------------------------------");

  // Send to ThingSpeak
  ThingSpeak.setField(1, avgTemp);
  ThingSpeak.setField(2, Rsample);
  ThingSpeak.setField(3, Rt);
  ThingSpeak.setField(4, rho);
  ThingSpeak.setField(5, R0_est);
  ThingSpeak.setField(6, material);

  int responseCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (responseCode == 200) {
    Serial.println("Channel update successful.");
  } else {
    Serial.print("Problem updating channel. HTTP error code ");
    Serial.println(responseCode);
  }

  delay(200);
}


