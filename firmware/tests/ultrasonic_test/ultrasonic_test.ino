/*
 * ============================================================
 * ULTRASONIC SENSOR TEST SCRIPT
 * For Library Monitoring System (ESP32)
 * ============================================================
 */

#define TRIG1_PIN 26
#define ECHO1_PIN 39 // ESP32 VN pin

void setup() {
  Serial.begin(115200);
  pinMode(TRIG1_PIN, OUTPUT);
  pinMode(ECHO1_PIN, INPUT);
  
  Serial.println("\n--- Ultrasonic Test Initiated ---");
  Serial.println("Point the sensor at an object and watch the distance.");
}

void loop() {
  // Clear the TRIG pin
  digitalWrite(TRIG1_PIN, LOW);
  delayMicroseconds(2);
  
  // Trigger the sensor
  digitalWrite(TRIG1_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG1_PIN, LOW);
  
  // Read the echo (30ms timeout = ~5 meters)
  long duration = pulseIn(ECHO1_PIN, HIGH, 30000);
  
  // Calculate distance in cm
  float distance = (duration * 0.0343) / 2.0;

  if (duration == 0) {
    Serial.println("No reading (Check wiring/power)");
  } else {
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.print(" cm | ");
    
    // Simple bar visualizer
    int dots = map(constrain(distance, 2, 80), 2, 80, 1, 40);
    for (int i=0; i<dots; i++) Serial.print(".");
    Serial.println("|");
  }

  delay(200);
}
