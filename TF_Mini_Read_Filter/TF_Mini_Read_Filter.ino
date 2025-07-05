HardwareSerial tfMini(2);

// ===== FILTERING SYSTEM =====

// YMFC-style circular buffer variables (renamed for clarity)
#define BUFFER_SIZE 21  // Same as YMFC BARO_TAB_SIZE
int lidarHistTab[BUFFER_SIZE];  // Lidar history table
int lidarHistIdx = 0;           // Lidar history index  
int lidarSum = 0;               // Running sum of lidar readings

// Complementary filter variables
float complementaryFiltered = 0;
// const float COMP_FILTER_ALPHA = 0.85;  // Adjust between 0-1 (higher = more responsive)

// Data storage for comparison
int rawData = 0;
int circularFiltered = 0;
int complementaryData = 0;

void setup() {
  Serial.begin(115200);
  tfMini.begin(115200, SERIAL_8N1, 7, 6);
  
  // Initialize YMFC-style circular buffer
  for (int i = 0; i < BUFFER_SIZE; i++) {
    lidarHistTab[i] = 0;
  }
  lidarSum = 0;
  lidarHistIdx = 0;
  
  Serial.println("TF Mini Filtering Comparison - YMFC Style");
  Serial.println("Format: Raw,Circular,Complementary");
  delay(1000);
}

void loop() {
  int distance = readDistance();
  
  if (distance > 0) {
    // 1. Store raw data
    rawData = distance;
    
    // 2. Apply circular buffer (moving average)
    circularFiltered = applyCircularBuffer(distance);
    
    // 3. Apply complementary filter
    complementaryData = applyComplementaryFilter(distance);
    
    // Output for Serial Plotter (CSV format)
    Serial.print("Raw_Data:");
    Serial.print(rawData);
    Serial.print(",");

    Serial.print("Circular_Filtered_Data:");
    Serial.print(circularFiltered);
    Serial.print(",");

    Serial.print("Complementary_Data:");
    Serial.println(complementaryData);
  }
  
  delay(50); // 20Hz update rate
}

// ===== ORIGINAL TF MINI READING FUNCTION =====
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

// ===== CIRCULAR BUFFER FILTER (YMFC Style) =====
int applyCircularBuffer(int newValue) {
  // YMFC approach: Add new value, subtract value that will be overwritten
  lidarHistTab[lidarHistIdx] = newValue;                    // Store new value
  lidarSum += lidarHistTab[lidarHistIdx];                   // Add new value to sum
  lidarSum -= lidarHistTab[(lidarHistIdx + 1) % BUFFER_SIZE]; // Subtract value that will be overwritten next
  
  // Move index forward
  lidarHistIdx++;
  if (lidarHistIdx == BUFFER_SIZE) lidarHistIdx = 0;
  
  // Calculate filtered value
  // Note: YMFC divides by (BUFFER_SIZE-1) instead of BUFFER_SIZE
  return lidarSum / (BUFFER_SIZE - 1);
}

// ===== COMPLEMENTARY FILTER =====
int applyComplementaryFilter(int newValue) {
  // Initialize filter on first reading
  static bool firstReading = true;
  if (firstReading) {
    complementaryFiltered = newValue;
    firstReading = false;
    return (int)complementaryFiltered;
  }
  
  // Complementary filter formula:
  // output = alpha * new_value + (1 - alpha) * previous_output
  // Higher alpha = more responsive to changes
  // Lower alpha = more smoothing
  const float COMP_FILTER_ALPHA = 0.75;  // Adjust between 0-1 (higher = more responsive)

  complementaryFiltered = COMP_FILTER_ALPHA * newValue + 
                         (1.0 - COMP_FILTER_ALPHA) * complementaryFiltered;
  
  return (int)complementaryFiltered;
}

// ===== OPTIONAL: ADVANCED OUTLIER REJECTION FILTER =====
int applyOutlierRejection(int newValue) {
  static int lastGoodValue = 0;
  static int outlierCount = 0;
  const int MAX_CHANGE = 100;  // Maximum allowed change in cm
  const int MAX_OUTLIERS = 3;  // Maximum consecutive outliers
  
  // Initialize on first reading
  if (lastGoodValue == 0) {
    lastGoodValue = newValue;
    return newValue;
  }
  
  // Check if value is an outlier
  int change = abs(newValue - lastGoodValue);
  
  if (change > MAX_CHANGE) {
    outlierCount++;
    if (outlierCount < MAX_OUTLIERS) {
      // Return last good value
      return lastGoodValue;
    } else {
      // Too many outliers, accept the new value
      lastGoodValue = newValue;
      outlierCount = 0;
      return newValue;
    }
  } else {
    // Good value
    lastGoodValue = newValue;
    outlierCount = 0;
    return newValue;
  }
}

/*
=============================================================================
USAGE INSTRUCTIONS FOR SERIAL PLOTTER:

1. Upload this code to your ESP32
2. Open Arduino IDE Serial Plotter (Tools > Serial Plotter)
3. Set baud rate to 115200
4. You'll see 3 lines:
   - Blue line: Raw TF Mini data (noisy, spiky)
   - Red line: Circular buffer filtered (smooth, delayed)
   - Green line: Complementary filtered (balanced)

UNDERSTANDING THE FILTERS:

CIRCULAR BUFFER (Moving Average):
- Pros: Removes noise very effectively, stable output
- Cons: Adds delay, slow to respond to real changes
- Best for: Stable altitude hold, reducing sensor noise
- Used in: ArduPilot, commercial flight controllers

COMPLEMENTARY FILTER:
- Pros: Responsive to changes, configurable balance
- Cons: May still pass some noise through
- Best for: Real-time applications, dynamic flight
- Tuning: Increase COMP_FILTER_ALPHA for more responsiveness
          Decrease for more smoothing

WHEN TO USE EACH:
- For altitude hold: Circular buffer (stability is key)
- For obstacle avoidance: Complementary filter (speed is key)
- For landing: Combination of both (stable but responsive)

EXPECTED BEHAVIOR:
- Raw data: Jumpy, noisy readings
- Circular: Smooth but delayed response
- Complementary: Balanced between noise and responsiveness

=============================================================================
*/
