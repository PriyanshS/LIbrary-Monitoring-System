/*
 * ============================================================
 * LIBRARY MONITORING SYSTEM — Firebase Edition
 * Smart Environment & Occupancy Monitor
 *
 * Hardware: ESP32
 * Cloud: Firebase Realtime Database
 * ============================================================
 */

#include <DHT.h>
#include <Firebase_ESP_Client.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// ─── Credentials ─────────────────────────────────────────────
#define WIFI_SSID "Piyansh Sakxena"
#define WIFI_PASSWORD "piyu@1234"

// From Firebase Console -> Project Settings -> General
#define API_KEY "AIzaSyAGwbhq7KhbvuMlNGtrroVTOi9Oq4VOIh8"
// From Firebase Console -> Realtime Database
#define DATABASE_URL "https://library-monitor-65ad9-default-rtdb.asia-southeast1.firebasedatabase.app"

// ─── Pin Definitions ─────────────────────────────────────────
#define DHT_PIN 4
#define DHT_TYPE DHT11
#define PIR_PIN 14
#define MIC_PIN 34
#define GAS_PIN 35
#define TRIG1_PIN 26
#define ECHO1_PIN 27
#define TRIG2_PIN 32
#define ECHO2_PIN 33
#define LED_GREEN_PIN 18
#define LED_RED_PIN 19
#define BUZZER_PIN 23

// ─── Thresholds ──────────────────────────────────────────────
#define MAX_CAPACITY_DEFAULT 50
#define NOISE_THRESHOLD 2500
#define GAS_THRESHOLD 2000
#define TEMP_MAX 30.0
#define DISTANCE_TRIGGER_CM 80

// ─── Objects ──────────────────────────────────────────────────
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

FirebaseData fbdo;
FirebaseData stream;
FirebaseAuth auth;
FirebaseConfig config;

// ─── State Variables ──────────────────────────────────────────
int occupancyCount = 0;
int maxCapacity = MAX_CAPACITY_DEFAULT;
bool buzzerEnabled = true;
float temperature = 0;
float humidity = 0;
int noiseLevel = 0;
int airQuality = 0;
bool pirState = false;

bool entry1Triggered = false;
bool entry2Triggered = false;
unsigned long lastEntry1 = 0;
unsigned long lastEntry2 = 0;
unsigned long lastPush = 0;
unsigned long lastSample = 0;

// ─── Function Prototypes ──────────────────────────────────────
float readDistance(int trigPin, int echoPin);
int computeComfortScore();
void updateActuators();
void pushToFirebase();
void readAllSensors();
void handleOccupancy();
void updateLCD();
void streamCallback(FirebaseStream data);
void streamTimeoutCallback(bool timeout);

// ─── Setup ────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  pinMode(PIR_PIN, INPUT);
  pinMode(TRIG1_PIN, OUTPUT);
  pinMode(ECHO1_PIN, INPUT);
  pinMode(TRIG2_PIN, OUTPUT);
  pinMode(ECHO2_PIN, INPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("LibraryOS Boot");

  // WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");

  // Firebase Config
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Sign up/in anonymously for simplicity (ensure Auth is enabled in console)
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase Auth: Success");
  } else {
    Serial.printf("Firebase Auth Failed: %s\n",
                  config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Setup Stream (Receive commands from App)
  if (!Firebase.RTDB.beginStream(&stream, "/controls")) {
    Serial.printf("Stream begin error: %s\n", stream.errorReason().c_str());
  }
  Firebase.RTDB.setStreamCallback(&stream, streamCallback,
                                  streamTimeoutCallback);

  lcd.clear();
  lcd.print("System Ready");
  delay(1000);
}

// ─── Loop ─────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // Read sensors every 2s
  if (now - lastSample >= 2000) {
    readAllSensors();
    handleOccupancy();
    updateLCD();
    lastSample = now;
  }

  // Push to Firebase every 5s (to avoid flooding)
  if (now - lastPush >= 5000) {
    pushToFirebase();
    lastPush = now;
  }
}

// ─── Firebase Stream Callback ────────────────────────────────
void streamCallback(FirebaseStream data) {
  String path = data.dataPath();
  String val = data.stringData();

  if (path == "/maxCapacity") {
    maxCapacity = val.toInt();
    Serial.printf("[FIREBASE] Max Capacity set to %d\n", maxCapacity);
  } else if (path == "/buzzerEnabled") {
    buzzerEnabled = (val == "true");
    Serial.printf("[FIREBASE] Buzzer %s\n", buzzerEnabled ? "ON" : "OFF");
  }
}

void streamTimeoutCallback(bool timeout) {
  if (timeout)
    Serial.println("[STREAM] Timeout, resuming...");
}

// ─── Push to Firebase ────────────────────────────────────────
void pushToFirebase() {
  if (Firebase.ready()) {
    FirebaseJson json;
    json.set("temperature", temperature);
    json.set("humidity", humidity);
    json.set("noise", map(noiseLevel, 0, 4095, 0, 100));
    json.set("airQuality", map(airQuality, 0, 4095, 0, 100));
    json.set("occupancy", occupancyCount);
    json.set("pir", pirState);
    json.set("comfort", computeComfortScore());
    json.set("timestamp/.sv", "timestamp"); // Server-side timestamp for connection monitoring

    // Build Status String
    int comfort = computeComfortScore();
    String status = "FAIR";
    if (occupancyCount >= maxCapacity)
      status = "FULL";
    else if (airQuality > GAS_THRESHOLD)
      status = "POOR AIR";
    else if (comfort > 75)
      status = "EXCELLENT";
    else if (comfort > 50)
      status = "GOOD";

    json.set("status", status);

    if (Firebase.RTDB.setJSON(&fbdo, "/sensor", &json)) {
      Serial.println("[FIREBASE] Data Sync Complete");
    } else {
      Serial.println("[FIREBASE] Sync Error: " + fbdo.errorReason());
    }
  }
}

// ─── Sensor Logic (Same as before) ───────────────────────────
void readAllSensors() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t))
    temperature = t;
  if (!isnan(h))
    humidity = h;
  pirState = digitalRead(PIR_PIN);
  noiseLevel = analogRead(MIC_PIN);
  airQuality = analogRead(GAS_PIN);
  updateActuators();
}

void handleOccupancy() {
  float d1 = readDistance(TRIG1_PIN, ECHO1_PIN);
  float d2 = readDistance(TRIG2_PIN, ECHO2_PIN);
  unsigned long now = millis();

  if (d1 < DISTANCE_TRIGGER_CM && !entry1Triggered) {
    entry1Triggered = true;
    lastEntry1 = now;
  }
  if (entry1Triggered && d2 < DISTANCE_TRIGGER_CM) {
    if ((now - lastEntry1) < 3000)
      occupancyCount++;
    entry1Triggered = false;
  }
  if (entry1Triggered && (now - lastEntry1) > 3000)
    entry1Triggered = false;

  if (d2 < DISTANCE_TRIGGER_CM && !entry2Triggered) {
    entry2Triggered = true;
    lastEntry2 = now;
  }
  if (entry2Triggered && d1 < DISTANCE_TRIGGER_CM) {
    if ((now - lastEntry2) < 3000 && occupancyCount > 0)
      occupancyCount--;
    entry2Triggered = false;
  }
  if (entry2Triggered && (now - lastEntry2) > 3000)
    entry2Triggered = false;
}

float readDistance(int trigPin, int echoPin) {
  float speedOfSound = 331.3 + (0.606 * temperature);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0)
    return 999.0;
  return (duration * speedOfSound / 10000.0) / 2.0;
}

int computeComfortScore() {
  int s = 100;
  if (temperature > TEMP_MAX)
    s -= (int)((temperature - TEMP_MAX) * 3);
  if (noiseLevel > NOISE_THRESHOLD)
    s -= 20;
  if (airQuality > GAS_THRESHOLD)
    s -= 25;
  if (occupancyCount >= maxCapacity)
    s -= 30;
  return constrain(s, 0, 100);
}

void updateActuators() {
  bool alert = (occupancyCount >= maxCapacity) || (airQuality > GAS_THRESHOLD);
  digitalWrite(LED_RED_PIN, alert ? HIGH : LOW);
  digitalWrite(LED_GREEN_PIN, alert ? LOW : HIGH);
  if (buzzerEnabled && alert) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.printf("OCC:%d/%d T:%.1f", occupancyCount, maxCapacity, temperature);
  lcd.setCursor(0, 1);
  if (occupancyCount >= maxCapacity)
    lcd.print("!! LIB FULL !!");
  else if (airQuality > GAS_THRESHOLD)
    lcd.print("!! POOR AIR !!");
  else
    lcd.printf("Status:%d%% OK", computeComfortScore());
}
