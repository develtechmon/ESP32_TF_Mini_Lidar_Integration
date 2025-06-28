/*
 * Simple Fast ESP32 TF Mini Reader
 * 
 * Wiring:
 * TF Mini VCC -> ESP32 5V
 * TF Mini GND -> ESP32 GND
 * TF Mini TX  -> ESP32 GPIO16
 * TF Mini RX  -> ESP32 GPIO17
 */

HardwareSerial tfMini(2);

void setup() {
  Serial.begin(115200);
  tfMini.begin(115200, SERIAL_8N1, 13, 12); // RX and TX
  
  Serial.println("TF Mini Simple Fast Reader");
}

void loop() {
  int distance = readDistance();
  
  if (distance > 0) {
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  }
  
  // No delay for maximum speed
}

int readDistance() {
  // Read immediately when data is available
  while (tfMini.available() >= 9) {
    
    // Look for header bytes 0x59 0x59
    if (tfMini.read() == 0x59 && tfMini.read() == 0x59) {
      
      // Get distance from next 2 bytes
      int low = tfMini.read();
      int high = tfMini.read();
      int distance = low + (high << 8);
      
      // Clear remaining bytes
      tfMini.read(); tfMini.read(); tfMini.read(); 
      tfMini.read(); tfMini.read();
      
      return distance;
    }
  }
  
  return 0; // No valid reading
}
