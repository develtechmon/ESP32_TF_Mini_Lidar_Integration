HardwareSerial tfMini(1); // Use Serial1 instead of Serial2

void setup() {
  Serial.begin(115200);
  // For ESP32-S3, Serial1 typically uses GPIO 17 (TX) and GPIO 18 (RX)
  tfMini.begin(115200, SERIAL_8N1, 44, 43); // RX=44, TX=43ddd
  Serial.println("TF Mini Reader");
}

void loop() {
  int distance = readDistance();
  
  if (distance > 0) {
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  }
  
  delay(5);
}

int readDistance() {
  uint8_t buffer[9];
  
  while (tfMini.available() < 9) {
    delay(1);
  }
  
  while (tfMini.available()) {
    if (tfMini.read() == 0x59) {
      buffer[0] = 0x59;
      if (tfMini.read() == 0x59) {
        buffer[1] = 0x59;
        break;
      }
    }
  }
  
  for (int i = 2; i < 9; i++) {
    buffer[i] = tfMini.read();
  }
  
  uint8_t checksum = 0;
  for (int i = 0; i < 8; i++) {
    checksum += buffer[i];
  }
  
  if (checksum == buffer[8]) {
    return buffer[2] + (buffer[3] << 8);
  }
  
  return 0;
}
