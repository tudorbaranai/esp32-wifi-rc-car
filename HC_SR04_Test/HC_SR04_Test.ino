// HC-SR04 Ultrasonic Distance Sensor Test
// For ESP32-WROOM-32

#define TRIG_PIN 13
#define ECHO_PIN 14

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);

  Serial.println("HC-SR04 Ready");
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

  // Calculate distance
  float distance_cm = duration * 0.0343 / 2;

  // Display only distance
  if (duration == 0) {
    Serial.println("No echo");
  } else {
    Serial.print(distance_cm, 1);
    Serial.println(" cm");
  }

  delay(2000);  // Read every 2 seconds
}
