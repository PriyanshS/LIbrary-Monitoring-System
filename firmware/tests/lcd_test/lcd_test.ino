/*
 * ============================================================
 * LCD TEST V2 - Active Refresh
 * ============================================================
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// If your Serial Monitor found a different address (e.g., 0x3F), change it here:
LiquidCrystal_I2C lcd(0x27, 16, 2); 

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  
  Serial.println("\n--- LCD Debugging Session V2 ---");
  lcd.init();
  lcd.backlight();
}

void loop() {
  // Test Backlight
  lcd.backlight();
  
  // Clear and Write Text every loop
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Testing LCD...");
  lcd.setCursor(0, 1);
  lcd.print("Millis: ");
  lcd.print(millis() / 1000);
  
  Serial.println("Writing to LCD...");
  
  delay(1000);
  
  // Optional: flash backlight to show loop is running
  lcd.noBacklight();
  delay(200);
}
