#include "DHT.h"

#define DHTPIN 4        // Change this if you moved the wire to a different pin
#define DHTTYPE DHT22   // Use DHT22 if that's what you have

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println(F("--- Old DHT Test Script ---"));
  dht.begin();
}

void loop() {
  delay(2000); 

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.printf("Failed to read from DHT sensor on Pin %d!\n", DHTPIN);
    return;
  }

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.println(F("°C "));
}
