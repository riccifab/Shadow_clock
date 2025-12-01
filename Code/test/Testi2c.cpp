#include <Wire.h>

void setup() {
  Wire.begin();             // Initialize I2C
  Serial.begin(115200);     // Start Serial communication
  while (!Serial);          // Wait for Serial to initialize (useful for some boards)
  Serial.println("\nI2C Scanner");
}

void loop() {
  byte error, address;
  int count = 0;
  
  Serial.println("Scanning...");
  for (address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      count++;
      delay(1); // Brief pause
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  
  if (count == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("Scan complete.\n");

  delay(5000); // Wait 5 seconds before scanning again
}
