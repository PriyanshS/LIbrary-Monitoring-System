/*
 * ============================================================
 * ULTRASONIC SENSOR TEST (Occupancy Logic)
 * HC-SR04 Entry (GPIO 26/27) and Exit (GPIO 32/33)
 * ============================================================
 */

#define TRIG1_PIN 26
#define ECHO1_PIN 27
#define TRIG2_PIN 32
#define ECHO2_PIN 33

void setup() {
  Serial.begin(115200);
  
  pinMode(TRIG1_PIN, OUTPUT);
  pinMode(ECHO1_PIN, INPUT);
  pinMode(TRIG2_PIN, OUTPUT);
  pinMode(ECHO2_PIN, INPUT);
  
  Serial.println("\n--- Ultrasonic & Occupancy Test ---");
  Serial.println("US#1 Entry (26/27) | US#2 Exit (32/33)");
}

float getDistance(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  
  long duration = pulseIn(echo, HIGH, 30000);
  if (duration == 0) return 999; // Timeout
  return (duration * 0.0343) / 2;
}

void loop() {
  float d1 = getDistance(TRIG1_PIN, ECHO1_PIN);
  float d2 = getDistance(TRIG2_PIN, ECHO2_PIN);
  
  Serial.print("Entry Sensor: ");
  if (d1 > 400) Serial.print("OUT OF RANGE");
  else { Serial.print(d1); Serial.print(" cm"); }
  
  Serial.print(" | Exit Sensor: ");
  if (d2 > 400) Serial.print("OUT OF RANGE");
  else { Serial.print(d2); Serial.print(" cm"); }
  
  // Trigger Logic Simulation
  if (d1 < 50) Serial.print("  [ ENTRY TRIGGERED ]");
  if (d2 < 50) Serial.print("  [ EXIT TRIGGERED ]");
  
  Serial.println();
  
  delay(500);
}
