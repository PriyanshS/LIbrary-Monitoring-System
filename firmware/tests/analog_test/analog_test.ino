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
  // Monitor three potential ADC1 pins to help you find the right one
  int pins[] = {34, 32, 33};
  const char* labels[] = {"D34", "D32", "D33"};

  Serial.print("Time: "); Serial.print(millis()/1000); Serial.println("s");

  for (int i=0; i<3; i++) {
    int pin = pins[i];
    unsigned long startMillis = millis();
    unsigned int signalMax = 0;
    unsigned int signalMin = 4095;
    
    while (millis() - startMillis < 50) {
      unsigned int sample = analogRead(pin);
      if (sample > signalMax) signalMax = sample;
      if (sample < signalMin) signalMin = sample;
    }
    unsigned int p2p = signalMax - signalMin;

    Serial.print("  Pin "); Serial.print(labels[i]);
    Serial.print(" -> P2P: "); Serial.print(p2p);
    Serial.print(" | Min: "); Serial.print(signalMin);
    Serial.print(" | Max: "); Serial.print(signalMax);
    if (p2p > 200) Serial.print(" <--- [!] SIGNAL DETECTED");
    Serial.println();
  }

  // Gas Sensor on 35
  long gasSum = 0;
  for (int i=0; i<10; i++) gasSum += analogRead(GAS_PIN);
  Serial.print("  Pin D35 (GAS) -> Avg: "); Serial.println(gasSum / 10);
  
  Serial.println("----------------------------------------");
  delay(1000); 
}
