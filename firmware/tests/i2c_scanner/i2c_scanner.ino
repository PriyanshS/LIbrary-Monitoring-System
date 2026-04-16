/*
 * ============================================================
 * I2C SCANNER (Custom Pins: 27 & 13)
 * For Library Monitoring System (ESP32)
 * ============================================================
 */

#include <Wire.h>

#define SDA_PIN 27
#define SCL_PIN 13

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\n--- I2C Scanner (Pins 27 & 13) ---");
  
  Wire.begin(SDA_PIN, SCL_PIN);
}

void loop() {
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");
      nDevices++;
    }
    else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");

  delay(5000); 
}
