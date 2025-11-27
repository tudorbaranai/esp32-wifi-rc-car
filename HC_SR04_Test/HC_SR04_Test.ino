// HC-SR04 Ultrasonic Distance Sensor Test
// For ESP32-WROOM-32
// Wiring:
//   VCC  -> 5V
//   GND  -> GND
//   TRIG -> GPIO 13
//   ECHO -> GPIO 14 (through 2x 1kOhm voltage divider!)

#define TRIG_PIN 13
#define ECHO_PIN 14

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);

  Serial.println("\n=== HC-SR04 Distance Sensor Test ===");
  Serial.println("TRIG: GPIO 13");
  Serial.println("ECHO: GPIO 14 (with voltage divider)");
  Serial.println("Reading distance every 100ms...\n");
}

void loop() {
  // Send trigger pulse (10us HIGH)
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Read echo duration (timeout 30ms = ~5m max range)
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  // Calculate distance (speed of sound = 343 m/s = 0.0343 cm/us)
  // Distance = (duration * speed) / 2 (divide by 2 for round trip)
  float distance_cm = duration * 0.0343 / 2;

  // Display results
  if (duration == 0) {
    Serial.println("No echo received - check wiring!");
    Serial.println("  - Is ECHO connected to GPIO 14?");
    Serial.println("  - Is voltage divider correct? (2x 1kOhm)");
    Serial.println("  - Is sensor getting 5V power?");
  } else if (distance_cm < 2) {
    Serial.println("Too close! (< 2cm)");
  } else if (distance_cm > 400) {
    Serial.println("Out of range (> 400cm)");
  } else {
    Serial.print("Distance: ");
    Serial.print(distance_cm, 1);
    Serial.print(" cm  (");
    Serial.print(distance_cm / 100, 2);
    Serial.println(" m)");
  }

  delay(100);  // Read every 100ms
}
