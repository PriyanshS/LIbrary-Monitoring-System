/*
 * ============================================================
 * PIR MOTION SENSOR TEST
 * PIR Output (GPIO 14)
 * Visual Feedback: LED Green (GPIO 18)
 * ============================================================
 */

#define PIR_PIN 14
#define LED_GREEN 18

void setup() {
  Serial.begin(115200);
  
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_GREEN, OUTPUT);
  
  Serial.println("\n--- PIR Motion Sensor Test ---");
  Serial.println("Monitoring GPIO 14...");
  Serial.println("The Green LED will light up when motion is detected.");
}

void loop() {
  int val = digitalRead(PIR_PIN);
  
  if (val == HIGH) {
    digitalWrite(LED_GREEN, HIGH);
    Serial.print(millis() / 1000);
    Serial.println("s: [!] MOTION DETECTED");
  } else {
    digitalWrite(LED_GREEN, LOW);
  }
  
  delay(100);
}
