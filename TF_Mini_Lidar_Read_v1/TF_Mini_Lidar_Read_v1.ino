HardwareSerial tfMini(2);

void setup() {
  Serial.begin(115200);
  tfMini.begin(115200, SERIAL_8N1, 7, 6);
  Serial.println("TF Mini Reader");
}

void loop() {
  int distance = readDistance();
  
  if (distance > 0) {
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  }
  
  // delay(100); // Don't spam readings
}

int readDistance() {
  uint8_t buffer[9];
  int index = 0;
  
  // Wait for data
  while (tfMini.available() < 9) {
    delay(1);
  }
  
  // Find frame start
  while (tfMini.available()) {
    if (tfMini.read() == 0x59) {
      buffer[0] = 0x59;
      if (tfMini.read() == 0x59) {
        buffer[1] = 0x59;
        break;
      }
    }
  }
  
  // Read remaining 7 bytes
  for (int i = 2; i < 9; i++) {
    buffer[i] = tfMini.read();
  }
  
  // Simple checksum
  uint8_t checksum = 0;
  for (int i = 0; i < 8; i++) {
    checksum += buffer[i];
  }
  
  if (checksum == buffer[8]) {
    return buffer[2] + (buffer[3] << 8);
  }
  
  return 0; // Bad checksum
}
