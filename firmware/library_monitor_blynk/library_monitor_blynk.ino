/*
 * ============================================================
 * LIBRARY MONITORING SYSTEM — BLYNK IOT VERSION (Parallel)
 * Features: Native App Control, Push Alerts, Remote Overrides
 * ============================================================
 */

// ─── Blynk IoT Credentials (FILL THESE FROM CONSOLE) ────────
#define BLYNK_TEMPLATE_ID   "TMPL3T_CrZo81"
#define BLYNK_TEMPLATE_NAME "LibraryOS"
#define BLYNK_AUTH_TOKEN    "NEA4ftRroQrIY0c-N1ln-IhnlWeCCquh"

#include <DHTesp.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// ─── WiFi Credentials ────────────────────────────────────────
char ssid[] = "Piyansh Sakxena";
char pass[] = "piyu@1234";

// ─── Pin Definitions (Left Side Only) ───────────────────────
#define DHT_PIN 14
#define PIR_PIN 36        
#define MIC_PIN 34        
#define GAS_PIN 35        
#define TRIG1_PIN 26      
#define ECHO1_PIN 39      
#define SDA_PIN 27        
#define SCL_PIN 13        
#define BUZZER_PIN 33
#define LED_RED_PIN 32
#define LED_GREEN_PIN 25

// ─── Thresholds ──────────────────────────────────────────────
#define NOISE_BASELINE 20
#define GAS_THRESHOLD 2000
#define DISTANCE_TRIGGER_CM 10

// ─── State Variables ──────────────────────────────────────────
int occupancyCount = 0;
int maxCapacity = 50;
float temperature = 0, humidity = 0;
int noiseLevel = 0, airQuality = 0;
bool motionDetected = false;

// Manual Overrides (from Blynk)
bool ledRedOverride = false;
bool ledGreenOverride = false;
bool buzzerEnabled = true;

// Timing
BlynkTimer timer;
unsigned long lastBuzzerAction = 0;
unsigned long lastCountTime = 0;
bool pulseState = false;
unsigned long lastPulseTime = 0;

// LCD Setup
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHTesp dht;

// ─── Functions ────────────────────────────────────────────────

float readDistance() {
  digitalWrite(TRIG1_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG1_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG1_PIN, LOW);
  long duration = pulseIn(ECHO1_PIN, HIGH, 20000); 
  if (duration == 0) return 999.0;
  return (duration * 0.0343) / 2.0;
}

void beep(int duration) {
  lastBuzzerAction = millis();
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}

void handleAlerts() {
  bool sensorAlert = (occupancyCount >= maxCapacity) || (airQuality > GAS_THRESHOLD) || (noiseLevel > 100);
  
  // Master Priority Logic
  bool redState = ledRedOverride;
  if (!ledRedOverride && sensorAlert) redState = true; 
  
  bool greenState = ledGreenOverride;
  if (!ledRedOverride && !ledGreenOverride && !sensorAlert) greenState = true;

  digitalWrite(LED_RED_PIN, redState ? HIGH : LOW);
  digitalWrite(LED_GREEN_PIN, greenState ? HIGH : LOW);

  // Pulsed Buzzer (Manual Master Silence)
  if (buzzerEnabled && sensorAlert) {
    if (millis() - lastPulseTime > 1000) {
      pulseState = !pulseState;
      digitalWrite(BUZZER_PIN, pulseState ? HIGH : LOW);
      if (pulseState) lastBuzzerAction = millis();
      lastPulseTime = millis();
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.printf("C:%d/%d T:%.1f", occupancyCount, maxCapacity, temperature);
  lcd.setCursor(0, 1);
  lcd.printf("Noise:%d Gas:%d", noiseLevel, airQuality);
}

// ─── Periodic Timer Job ───────────────────────────────────────
void sendData() {
  // Read DHT
  TempAndHumidity data = dht.getTempAndHumidity();
  if (dht.getStatus() == 0) { temperature = data.temperature; humidity = data.humidity; }
  
  // Read Gas
  airQuality = analogRead(GAS_PIN);
  
  // Read Sound
  unsigned int sMax = 0, sMin = 4095;
  unsigned long start = millis();
  while(millis() - start < 50) {
    unsigned int s = analogRead(MIC_PIN);
    if (s > sMax) sMax = s;
    if (s < sMin) sMin = s;
  }
  noiseLevel = sMax - sMin;

  // Push to Blynk
  Blynk.virtualWrite(V0, temperature);
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V2, constrain(map(noiseLevel, NOISE_BASELINE, 200, 0, 100), 0, 100)); // Noise %
  Blynk.virtualWrite(V3, constrain(map(airQuality, 900, 2000, 0, 100), 0, 100)); // Gas %
  Blynk.virtualWrite(V4, occupancyCount);
  
  // Comfort Score logic
  int comfort = 100;
  if (temperature > 30) comfort -= 20;
  if (occupancyCount >= maxCapacity) comfort -= 30;
  Blynk.virtualWrite(V5, constrain(comfort, 0, 100));
  
  Blynk.virtualWrite(V6, digitalRead(PIR_PIN));
  
  updateLCD();
}

// ─── Occupancy Detection ──────────────────────────────────────
void checkOccupancy() {
  unsigned long now = millis();
  if (now - lastBuzzerAction < 1000) return; // Prevent buzzer loop

  if (now - lastCountTime > 3000) {
    if (digitalRead(PIR_PIN)) {
      int samples = 0;
      for(int i=0; i<3; i++) {
        if (readDistance() <= DISTANCE_TRIGGER_CM) samples++;
        delay(10);
      }
      if (samples >= 2) {
        occupancyCount++;
        beep(150);
        lastCountTime = now;
        Blynk.virtualWrite(V4, occupancyCount); // Immediate sync
      }
    }
  }
}

// ─── Blynk Handlers (Remote Controls) ───────────────────────

BLYNK_WRITE(V7) { maxCapacity = param.asInt(); }
BLYNK_WRITE(V8) { buzzerEnabled = param.asInt(); }
BLYNK_WRITE(V9) { ledRedOverride = param.asInt(); }
BLYNK_WRITE(V10) { ledGreenOverride = param.asInt(); }
BLYNK_WRITE(V11) { occupancyCount = 0; Blynk.virtualWrite(V4, 0); }
BLYNK_WRITE(V12) { occupancyCount++; Blynk.virtualWrite(V4, occupancyCount); }
BLYNK_WRITE(V13) { if(occupancyCount > 0) occupancyCount--; Blynk.virtualWrite(V4, occupancyCount); }

// Sync values from app on connect
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V7, V8, V9, V10);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init(); lcd.backlight();
  
  pinMode(PIR_PIN, INPUT);
  pinMode(TRIG1_PIN, OUTPUT);
  pinMode(ECHO1_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  
  dht.setup(DHT_PIN, DHTesp::DHT22);
  
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  
  timer.setInterval(2000L, sendData); 
}

void loop() {
  Blynk.run();
  timer.run();
  checkOccupancy();
  handleAlerts();
}
