/*
 * ============================================================
 * LIBRARY MONITORING SYSTEM — Final Production v3
 * Refined Occupancy, Multi-Screen LCD, and Pulsed Alerts
 * ============================================================
 */

#include <DHTesp.h>
#include <Firebase_ESP_Client.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <Wire.h>

// Firebase Helpers
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ─── Credentials ─────────────────────────────────────────────
#define WIFI_SSID "Piyansh Sakxena"
#define WIFI_PASSWORD "piyu@1234"
#define API_KEY "AIzaSyAGwbhq7KhbvuMlNGtrroVTOi9Oq4VOIh8"
#define DATABASE_URL "https://library-monitor-65ad9-default-rtdb.asia-southeast1.firebasedatabase.app"

// ─── Pin Definitions (Oct 2024 Wiring) ───────────────────────
#define DHT_PIN 14
#define PIR_PIN 36        // VP (Input Only)
#define MIC_PIN 34        // AO
#define GAS_PIN 35        // AO
#define TRIG1_PIN 26      // US Trigger
#define ECHO1_PIN 39      // VN (Input Only)
#define SDA_PIN 27        // LCD SDA
#define SCL_PIN 13        // LCD SCL
#define BUZZER_PIN 33
#define LED_RED_PIN 32
#define LED_GREEN_PIN 25

// ─── Thresholds & Baselines ──────────────────────────────────
#define MAX_CAPACITY_DEFAULT 50
#define NOISE_BASELINE 20
#define NOISE_THRESHOLD 100
#define GAS_BASELINE 900
#define GAS_THRESHOLD 2000
#define DISTANCE_TRIGGER_CM 10   // Refined as per user (> 10cm ignored)

// ─── Objects ──────────────────────────────────────────────────
DHTesp dht;
LiquidCrystal_I2C lcd(0x27, 16, 2);
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ─── State Variables ──────────────────────────────────────────
int occupancyCount = 0;
int maxCapacity = MAX_CAPACITY_DEFAULT;
float temperature = 0, humidity = 0;
int noiseLevel = 0, airQuality = 0;
bool isOnline = false;

// Remote Controls
bool buzzerEnabled = true;
bool ledRedOverride = false;
bool ledGreenOverride = false;
bool remoteTriggeredReset = false;

// Streaming Data
FirebaseData fbdoStream;

// Multi-Screen LCD
int currentLcdScreen = 0;
unsigned long lastScreenSwitch = 0;

// Non-blocking Pulse Alarm
unsigned long lastPulseTime = 0;
bool pulseState = false;

// Robust State Machine for Occupancy
enum OccState { IDLE, PIR_FIRST, US_FIRST, COOLDOWN };
OccState currentState = IDLE;
unsigned long stateStartTime = 0;
unsigned long lastCountTime = 0;

unsigned long lastCloudSync = 0;
unsigned long lastLocalSample = 0;
unsigned long lastBuzzerAction = 0; // For PIR lockout
unsigned long lastStreamCheck = 0;

// ─── Functions ────────────────────────────────────────────────

float readDistance() {
  digitalWrite(TRIG1_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG1_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG1_PIN, LOW);
  long duration = pulseIn(ECHO1_PIN, HIGH, 20000); // 20ms timeout
  if (duration == 0) return 999.0;
  return (duration * 0.0343) / 2.0;
}

void beep(int duration) {
  lastBuzzerAction = millis();
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}

void updateLCD() {
  lcd.clear();
  switch (currentLcdScreen) {
    case 0: // Main Occupancy & Climate
      lcd.setCursor(0, 0);
      lcd.printf("C:%d/%d T:%.1f", occupancyCount, maxCapacity, temperature);
      lcd.setCursor(0, 1);
      lcd.printf("HUMIDITY: %.0f%%", humidity);
      break;
    case 1: // Environment Raw + %
      lcd.setCursor(0, 0);
      lcd.printf("N:%d(%d%%)", noiseLevel, constrain(map(noiseLevel, NOISE_BASELINE, 200, 0, 100), 0, 100));
      lcd.setCursor(0, 1);
      lcd.printf("A:%d(%d%%)", airQuality, constrain(map(airQuality, GAS_BASELINE, 2000, 0, 100), 0, 100));
      break;
    case 2: // Network Status
      lcd.setCursor(0, 0);
      lcd.printf("NETWORK STATUS");
      lcd.setCursor(0, 1);
      lcd.print(isOnline ? "WiFi: CONNECTED" : "WiFi: DISCONNECTED");
      break;
  }
}

void handleAlerts() {
  // Master Logic: Dashboard controls take priority
  // If ledRedOverride is true, Red LED is ON. If false, it follows automated sensor logic ONLY if ledGreenOverride is also false.
  
  bool sensorAlert = (occupancyCount >= maxCapacity) || (airQuality > GAS_THRESHOLD) || (noiseLevel > NOISE_THRESHOLD);
  
  // Logic: Dashboard 'ON' always ON. 
  // If both are OFF, then use Auto logic.
  bool redState = ledRedOverride;
  bool greenState = ledGreenOverride;

  if (!ledRedOverride && !ledGreenOverride) {
     redState = sensorAlert;
     greenState = !sensorAlert;
  }

  digitalWrite(LED_RED_PIN, redState ? HIGH : LOW);
  digitalWrite(LED_GREEN_PIN, greenState ? HIGH : LOW);

  // Buzzer: Master Silence Path
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

void handleOccupancy() {
  unsigned long now = millis();
  
  // 1. Feedback Loop Protection: Ignore if buzzer recently active
  if (now - lastBuzzerAction < 1000) return;

  // 2. Cooldown between increments
  if (now - lastCountTime > 3000) { 
    bool pir = digitalRead(PIR_PIN);
    
    // 3. Robust distance sampling (require 3 consistent samples)
    int samplesBelow = 0;
    for(int i=0; i<3; i++) {
       if (readDistance() <= DISTANCE_TRIGGER_CM) samplesBelow++;
       delay(10);
    }
    bool ultra = (samplesBelow >= 2);

    if (pir && ultra) {
      occupancyCount++;
      beep(150); 
      lastCountTime = now;
      Serial.printf("[ENTRY] Count: %d\n", occupancyCount);
      
      // Immediate push to Firebase to prevent sync lag
      FirebaseJson json;
      json.set("occupancy", occupancyCount);
      json.set("timestamp", now);
      Firebase.RTDB.updateNodeAsync(&fbdo, "/sensor", &json);
    }
  }
}

// ─── Firebase Handlers ────────────────────────────────────────

void streamCallback(FirebaseStream data) {
  String path = data.dataPath();
  String val = data.payload();
  Serial.printf("[FIREBASE] Stream update on %s: %s\n", path.c_str(), val.c_str());
  
  if (path == "/") {
    FirebaseJson &json = data.jsonObject();
    FirebaseJsonData res;
    if (json.get(res, "maxCapacity")) maxCapacity = res.intValue;
    if (json.get(res, "buzzerEnabled")) buzzerEnabled = res.boolValue;
    if (json.get(res, "ledRed")) ledRedOverride = res.boolValue;
    if (json.get(res, "ledGreen")) ledGreenOverride = res.boolValue;
  } 
  else if (path == "/maxCapacity") maxCapacity = data.intData();
  else if (path == "/buzzerEnabled") buzzerEnabled = data.boolData();
  else if (path == "/ledRed") ledRedOverride = data.boolData();
  else if (path == "/ledGreen") ledGreenOverride = data.boolData();
  else if (path == "/resetOccupancy") {
    if (data.boolData()) {
      occupancyCount = 0;
      Serial.println("[COMMAND] Reset Occupancy");
      Firebase.RTDB.setBoolAsync(&fbdo, "/controls/resetOccupancy", false);
      Firebase.RTDB.setIntAsync(&fbdo, "/sensor/occupancy", 0);
    }
  } else if (path == "/remoteAdj") {
    int adj = data.intData();
    if (adj != 0) {
      occupancyCount += adj;
      if (occupancyCount < 0) occupancyCount = 0;
      Serial.printf("[COMMAND] Adjust Occupancy by %d (New: %d)\n", adj, occupancyCount);
      Firebase.RTDB.setIntAsync(&fbdo, "/controls/remoteAdj", 0); 
      Firebase.RTDB.setIntAsync(&fbdo, "/sensor/occupancy", occupancyCount);
    }
  }
  
  handleAlerts(); // Force update actuators immediately
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) Serial.println("Stream timed out, resuming...");
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- LibraryOS Production v3 ---");

  // I2C Init on Custom Pins
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();
  lcd.print("System Starting");

  pinMode(PIR_PIN, INPUT);
  pinMode(TRIG1_PIN, OUTPUT);
  pinMode(ECHO1_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);

  dht.setup(DHT_PIN, DHTesp::DHT22);

  // Offline-First connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 5000) {
    delay(500); Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    isOnline = true;
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    if (Firebase.signUp(&config, &auth, "", "")) {
      Serial.println("Firebase Auth: Success");
    } else {
      Serial.printf("Firebase Auth Failed: %s\n", config.signer.signupError.message.c_str());
    }
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    
    // Start Stream for Remote Controls
    if (!Firebase.RTDB.beginStream(&fbdoStream, "/controls")) {
      Serial.printf("Stream begin failed: %s\n", fbdoStream.errorReason().c_str());
    } else {
      Firebase.RTDB.setStreamCallback(&fbdoStream, streamCallback, streamTimeoutCallback);
    }
    
    Serial.println("\nWiFi Connected");
  } else {
    Serial.println("\nRunning Offline");
  }

  beep(300); 
}

void loop() {
  unsigned long now = millis();

  handleOccupancy();
  handleAlerts();

  // Every 1 second: Read sensors
  if (now - lastLocalSample >= 1000) {
    TempAndHumidity data = dht.getTempAndHumidity();
    if (dht.getStatus() == 0) { temperature = data.temperature; humidity = data.humidity; }
    airQuality = analogRead(GAS_PIN);
    
    // Sampling Mic
    unsigned int signalMax = 0, signalMin = 4095;
    unsigned long micStart = millis();
    while(millis() - micStart < 50) {
      unsigned int sample = analogRead(MIC_PIN);
      if (sample > signalMax) signalMax = sample;
      if (sample < signalMin) signalMin = sample;
    }
    noiseLevel = signalMax - signalMin;

    Serial.printf("[READ] T:%.1f Noise:%d Gas:%d Occ:%d\n", temperature, noiseLevel, airQuality, occupancyCount);
    updateLCD();
    lastLocalSample = now;
  }

  // Every 3 seconds: Cycle LCD Screen
  if (now - lastScreenSwitch > 3000) {
    currentLcdScreen = (currentLcdScreen + 1) % 3;
    updateLCD();
    lastScreenSwitch = now;
  }

  // Every 5 seconds: Firebase Sync
  if (isOnline && (now - lastCloudSync >= 5000)) {
    if (Firebase.ready()) {
      FirebaseJson json;
      json.set("temperature", temperature);
      json.set("humidity", humidity);
      json.set("noise", constrain(map(noiseLevel, NOISE_BASELINE, 200, 0, 100), 0, 100));
      json.set("airQuality", constrain(map(airQuality, GAS_BASELINE, 2000, 0, 100), 0, 100));
      json.set("occupancy", occupancyCount);
      json.set("timestamp", now); // Add timestamp for connection check
      Firebase.RTDB.setJSON(&fbdo, "/sensor", &json);
      lastCloudSync = now;
    }
  }
}
