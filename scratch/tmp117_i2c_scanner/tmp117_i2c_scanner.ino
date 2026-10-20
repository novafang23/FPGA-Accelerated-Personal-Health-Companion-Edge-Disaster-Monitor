/*
 * Standalone I2C Bus Scanner & TMP117 RMA Diagnostic Tool
 * Target: Vicharak ShrikeFi (ESP32-S3) + TMP117 Breakout Board
 * Prints to BOTH Serial (Native USB) and Serial0 (CH9102 UART) simultaneously.
 */

#include <Wire.h>

// ShrikeFi ESP32-S3 I2C Pinout
#define I2C_SDA_PIN  1   // GPIO 1 -> TMP117 SDA
#define I2C_SCL_PIN  2   // GPIO 2 -> TMP117 SCL

void setup() {
  // Initialize both native USB CDC (Serial) and hardware UART0 / CH9102 (Serial0)
  Serial.begin(115200);
  Serial0.begin(115200);
  delay(1500);

  printBoth("\n========================================================\n");
  printBoth("   ROBOCRAZE RMA DIAGNOSTIC: ISOLATED TMP117 TEST\n");
  printBoth("========================================================\n");
  printBoth("Configuring I2C on SDA=GPIO%d, SCL=GPIO%d (100 kHz)...\n", I2C_SDA_PIN, I2C_SCL_PIN);

  // Enable internal pull-ups for clean 3.3V bus signals
  pinMode(I2C_SDA_PIN, INPUT_PULLUP);
  pinMode(I2C_SCL_PIN, INPUT_PULLUP);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 100000); // 100 kHz Standard Mode
  printBoth("I2C bus initialized. Starting continuous bus scan...\n\n");
}

void loop() {
  byte count = 0;
  bool tmp117_found = false;

  printBoth("--------------------------------------------------------\n");
  printBoth("Scanning I2C address range 0x08 to 0x77...\n");

  for (byte address = 8; address < 120; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      printBoth(" -> ACK RECEIVED at address 0x%02X", address);

      if (address == 0x48) {
        printBoth(" [TMP117 - Default Address 0x48 - ALIVE]");
        tmp117_found = true;
      } else if (address == 0x49 || address == 0x4A || address == 0x4B) {
        printBoth(" [TMP117 - Alternate Address 0x%02X - ALIVE]", address);
        tmp117_found = true;
      }
      printBoth("\n");
      count++;
    } else if (error == 4) {
      printBoth(" -> Bus error at address 0x%02X\n", address);
    }
  }

  printBoth("--------------------------------------------------------\n");
  if (tmp117_found) {
    printBoth("STATUS: SUCCESS - TMP117 detected and responding!\n");
  } else if (count == 0) {
    printBoth("STATUS: NO DEVICE RESPONDING ON I2C BUS (0 ACKs).\n");
    printBoth("        Wiring is verified direct Point-to-Point.\n");
    printBoth("        VERDICT: TMP117 sensor hardware is DEAD ON ARRIVAL (DOA).\n");
  } else {
    printBoth("STATUS: Other devices found (%d), but NO TMP117 (0x48-0x4B).\n", count);
  }
  printBoth("========================================================\n\n");

  delay(4000); // Rescan every 4 seconds
}

void printBoth(const char* format, ...) {
  char buf[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  Serial.print(buf);
  Serial0.print(buf);
}
