#include "DHT.h"

#define DHTPIN 4     // Pin 15 is best for ESP32
#define DHTTYPE DHT22   // Specifically for DHT22

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println(F("DHT22 Test on GPIO 15"));
  dht.begin();
}

void loop() {
  delay(2000); 

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // If both are 0 or NaN, there is still a signal issue
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT22 on Pin 15!"));
    return;
  }

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.println(F("°C "));
}
