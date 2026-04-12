/*
 * ============================================================
 * ACTUATOR TEST
 * Green LED (18), Red LED (19), and Buzzer (23)
 * ============================================================
 */

#define LED_GREEN 18
#define LED_RED 19
#define BUZZER 23

void setup() {
  Serial.begin(115200);
  
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  
  Serial.println("\n--- Actuator Test ---");
  Serial.println("Cycling: Green -> Red -> Buzzer");
}

void loop() {
  // 1. Green LED
  Serial.println("LED: GREEN");
  digitalWrite(LED_GREEN, HIGH);
  delay(1000);
  digitalWrite(LED_GREEN, LOW);
  
  // 2. Red LED
  Serial.println("LED: RED");
  digitalWrite(LED_RED, HIGH);
  delay(1000);
  digitalWrite(LED_RED, LOW);
  
  // 3. Buzzer
  Serial.println("BUZZER: BEEP");
  digitalWrite(BUZZER, HIGH);
  delay(200);
  digitalWrite(BUZZER, LOW);
  
  delay(2000); // 2-second pause between cycles
}
