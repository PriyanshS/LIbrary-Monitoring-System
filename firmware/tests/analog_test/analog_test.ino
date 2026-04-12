/*
 * ============================================================
 * ANALOG SENSOR TEST
 * Microphone (GPIO 34) and MQ-2 Gas (GPIO 35)
 * ============================================================
 */

#define MIC_PIN 34
#define GAS_PIN 35

void setup() {
  Serial.begin(115200);
  
  // Note: GPIO 34 and 35 are input-only pins on ESP32, 
  // perfect for analog sensors.
  
  Serial.println("\n--- Analog Sensors Test ---");
  Serial.println("Microphone (D34) | Gas MQ-2 (D35)");
}

void loop() {
  // Read raw 12-bit ADC values (0 - 4095)
  int micValue = analogRead(MIC_PIN);
  int gasValue = analogRead(GAS_PIN);
  
  Serial.print("Time: "); Serial.print(millis()/1000); Serial.print("s | ");
  
  Serial.print("Mic Level: ");
  Serial.print(micValue);
  
  // Simple visualizer for Mic
  if (micValue > 2500) Serial.print(" [ LOUD ]");
  else if (micValue < 500) Serial.print(" [ SILENT ]");
  else Serial.print(" [ NORMAL ]");

  Serial.print(" | Gas/Smoke: ");
  Serial.print(gasValue);
  
  if (gasValue > 2000) Serial.print(" [ ALERT: GAS DETECTED ]");
  
  Serial.println();
  
  delay(100); // Faster sampling for Mic to catch noise
}
