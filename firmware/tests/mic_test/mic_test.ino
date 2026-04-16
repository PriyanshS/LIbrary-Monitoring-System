/*
 * ============================================================
 * MICROPHONE CALIBRATION & TEST SCRIPT
 * For Library Monitoring System (ESP32)
 * ============================================================
 */

#define MIC_PIN 34  // Based on your latest wiring table

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- Mic Test Initiated ---");
  Serial.println("Watch the [P2P] values and the sound bar below.");
  Serial.println("Try snapping your fingers or clapping!");
}

void loop() {
  unsigned long startMillis = millis();
  unsigned int peakToPeak = 0;
  unsigned int signalMax = 0;
  unsigned int signalMin = 4095;

  // Sample window: 50ms
  while (millis() - startMillis < 50) {
    unsigned int sample = analogRead(MIC_PIN);
    if (sample < 4095) { // filter out occasional ADC noise spikes
      if (sample > signalMax) signalMax = sample;
      else if (sample < signalMin) signalMin = sample;
    }
  }

  peakToPeak = signalMax - signalMin;

  // VISUALIZER: Create a simple bar for the Serial Monitor
  int barLength = map(peakToPeak, 0, 1500, 0, 50);
  String bar = "";
  for (int i = 0; i < barLength; i++) {
    if (i < 10) bar += "-";
    else if (i < 25) bar += "=";
    else bar += "#";
  }

  Serial.print("P2P: ");
  if (peakToPeak < 100) Serial.print(" ");
  if (peakToPeak < 10) Serial.print(" ");
  Serial.print(peakToPeak);
  Serial.print(" | ");
  Serial.println(bar);

  delay(50);
}
