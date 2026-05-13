/*
 * KAI PROJECT - SINGLE SIDED SENSOR TEST
 * Pins clustered on the VIN side (Bottom of board)
 * Connections: GPIO 33 (DHT11), 34 (MQ-2), 35 (LDR)
 */

#include "DHT.h"

// --- Pin Re-Mapping (All on the same side) ---
#define DHTPIN      33   // Moved to 33
#define DHTTYPE     DHT11
#define GAS_PIN     34   // Analog MQ-2
#define LDR_PIN     35   // Analog LDR

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- KAI Single-Side Test Starting ---");
  
  dht.begin();
  
  // Set ADC resolution to 12-bit (0-4095) for ESP32
  analogReadResolution(12); 
  
  Serial.println("Wiring Check:");
  Serial.println("- DHT11 Data -> GPIO 33");
  Serial.println("- MQ-2 AO    -> GPIO 34");
  Serial.println("- LDR Output -> GPIO 35");
  Serial.println("-----------------------------\n");
}

void loop() {
  // Read Humidity and Temp
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Read Gas and Light
  int gasValue = analogRead(GAS_PIN);
  int lightValue = analogRead(LDR_PIN);

  // Serial Print
  Serial.print("TEMP: "); 
  if (isnan(t)) Serial.print("ERR"); else Serial.print(t); 
  Serial.print("C | HUM: "); 
  if (isnan(h)) Serial.print("ERR"); else Serial.print(h); 
  Serial.print("% | GAS: "); 
  Serial.print(gasValue); 
  Serial.print(" | LIGHT: "); 
  Serial.println(lightValue);

  delay(2000); // 2 second interval
}
