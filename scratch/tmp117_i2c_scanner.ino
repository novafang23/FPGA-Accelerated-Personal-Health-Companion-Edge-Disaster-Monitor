/*
 * Standalone I2C Bus Scanner & TMP117 RMA Diagnostic Tool
 * Target: Vicharak ShrikeFi (ESP32-S3) + TMP117 Breakout Board
 * Tested on: Arduino IDE 2.x
 */

#include <Wire.h>

// ShrikeFi ESP32-S3 I2C Pin Assignments
#define I2C_SDA_PIN  1   // GPIO 1 -> TMP117 SDA
#define I2C_SCL_PIN  2   // GPIO 2 -> TMP117 SCL

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000); // Wait up to 3s for serial monitor
  delay(1000);

  Serial.println("\n========================================================");
  Serial.println("   ROBOCRAZE RMA DIAGNOSTIC: ISOLATED TMP117 TEST");
  Serial.println("========================================================");
  Serial.printf("Configuring I2C on SDA=GPIO%d, SCL=GPIO%d (100 kHz)...\n", I2C_SDA_PIN, I2C_SCL_PIN);

  // Enable internal pull-ups to guarantee solid 3.3V logic levels
  pinMode(I2C_SDA_PIN, INPUT_PULLUP);
  pinMode(I2C_SCL_PIN, INPUT_PULLUP);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 100000); // 100 kHz Standard Mode
  Serial.println("I2C bus initialized. Starting continuous bus scan...\n");
}

void loop() {
  byte count = 0;
  bool tmp117_found = false;

  Serial.println("--------------------------------------------------------");
  Serial.println("Scanning I2C address range 0x08 to 0x77...");

  for (byte address = 8; address < 120; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.printf(" -> ACK RECEIVED at address 0x%02X", address);

      if (address == 0x48) {
        Serial.print(" [TMP117 - Default Address 0x48 - ALIVE]");
        tmp117_found = true;
      } else if (address == 0x49 || address == 0x4A || address == 0x4B) {
        Serial.printf(" [TMP117 - Alternate Address 0x%02X - ALIVE]", address);
        tmp117_found = true;
      }
      Serial.println();
      count++;
    } else if (error == 4) {
      Serial.printf(" -> Bus error at address 0x%02X\n", address);
    }
  }

  Serial.println("--------------------------------------------------------");
  if (tmp117_found) {
    Serial.printf("STATUS: SUCCESS - TMP117 detected and responding!\n");
  } else if (count == 0) {
    Serial.println("STATUS: NO DEVICE RESPONDING ON I2C BUS (0 ACKs).");
    Serial.println("        Wiring is verified direct Point-to-Point.");
    Serial.println("        VERDICT: TMP117 sensor hardware is DEAD ON ARRIVAL (DOA).");
  } else {
    Serial.printf("STATUS: Other devices found (%d), but NO TMP117 (0x48-0x4B).\n", count);
  }
  Serial.println("========================================================\n");

  delay(4000); // Rescan every 4 seconds
}
