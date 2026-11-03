#include <WiFi.h>
#include <WiFiManager.h>  // WiFi configuration portal
#include <ESPmDNS.h>      // mDNS for esp32-car.local
#include <WebSocketsServer.h>

// ---------- Hardware pins ----------
#define IN1 21  // Left motor direction A
#define IN2 19  // Left motor direction B
#define IN3 18  // Right motor direction A
#define IN4 23  // Right motor direction B

// PWM Enable pins (L298N speed control)
#define ENA 25  // Left motor speed (PWM)
#define ENB 26  // Right motor speed (PWM)

// Motor reversal flags - ADJUST THESE IF CAR MOVES WRONG DIRECTION
// If car vibrates or moves opposite direction, try changing these
const bool LEFT_MOTOR_REVERSED  = true;   // Set to false if left motor goes backwards when it should go forward
const bool RIGHT_MOTOR_REVERSED = false;  // Set to true if right motor goes backwards when it should go forward

// PWM settings
const int PWM_FREQ = 1000;      // 1 kHz
const int PWM_RESOLUTION = 8;   // 8-bit (0-255)

// Motor threshold settings - PROPORTIONAL CONTROL (Linear response)
const int MIN_SPEED_THRESHOLD = 5;    // Start motors almost immediately (5% joystick)
const int MIN_PWM_OUTPUT = 100;       // Very slow minimum speed
const int MAX_PWM_OUTPUT = 115;       // Very slow max speed
                                      // Adjust this value to change top speed

// ---------- WiFi Configuration ----------
const char* MDNS_HOSTNAME = "esp32-car";  // Access via esp32-car.local
const char* AP_CONFIG_NAME = "ESP32-Car-Setup";  // AP name for WiFi configuration

// ---------- WebSocket server ----------
WebSocketsServer webSocket = WebSocketsServer(81);  // WebSocket on port 81

// ---------- Motor state ----------
struct MotorCommand {
  int leftSpeed;   // -100 to +100 (percentage)
  int rightSpeed;  // -100 to +100 (percentage)
};

volatile MotorCommand currentCmd = {0, 0};

// ---------- Safety timeout ----------
const unsigned long SAFETY_TIMEOUT_MS = 300;  // Stop if no command for 300ms (fast but tolerant)
volatile unsigned long lastCommandTime = 0;

// ---------- Motor control functions ----------

void setupMotors() {
  // Direction pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  // PWM setup for enable pins (ESP32 Core 3.x API)
  ledcAttach(ENA, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(ENB, PWM_FREQ, PWM_RESOLUTION);
  
  stopMotors();
}

// Apply minimum threshold and map to PWM range
int applyMotorThreshold(int speedPercent) {
  // speedPercent: -100 to +100

  int absSpeed = abs(speedPercent);

  // Dead zone - stop if below threshold
  if (absSpeed < MIN_SPEED_THRESHOLD) {
    return 0;
  }

  // Map MIN_SPEED_THRESHOLD-100% to MIN_PWM_OUTPUT-MAX_PWM_OUTPUT
  // This gives smooth acceleration from very slow to limited max speed
  int pwmValue = map(absSpeed, MIN_SPEED_THRESHOLD, 100, MIN_PWM_OUTPUT, MAX_PWM_OUTPUT);

  // Restore sign
  return (speedPercent > 0) ? pwmValue : -pwmValue;
}

void setLeftMotor(int speedPercent) {
  // Apply threshold and convert to PWM
  int pwmSpeed = applyMotorThreshold(speedPercent);
  
  // Apply reversal flag
  if (LEFT_MOTOR_REVERSED) pwmSpeed = -pwmSpeed;
  
  if (pwmSpeed > 0) {
    // Forward
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    ledcWrite(ENA, pwmSpeed);
  } else if (pwmSpeed < 0) {
    // Backward
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    ledcWrite(ENA, -pwmSpeed);
  } else {
    // Stop
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    ledcWrite(ENA, 0);
  }
}

void setRightMotor(int speedPercent) {
  // Apply threshold and convert to PWM
  int pwmSpeed = applyMotorThreshold(speedPercent);
  
  // Apply reversal flag
  if (RIGHT_MOTOR_REVERSED) pwmSpeed = -pwmSpeed;
  
  if (pwmSpeed > 0) {
    // Forward
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    ledcWrite(ENB, pwmSpeed);
  } else if (pwmSpeed < 0) {
    // Backward
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    ledcWrite(ENB, -pwmSpeed);
  } else {
    // Stop
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    ledcWrite(ENB, 0);
  }
}

void stopMotors() {
  setLeftMotor(0);
  setRightMotor(0);
}

// ---------- WebSocket event handler ----------
void webSocketEvent(uint8_t clientNum, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] WebSocket disconnected\n", clientNum);
      // Safety: Stop motors when client disconnects
      currentCmd.leftSpeed = 0;
      currentCmd.rightSpeed = 0;
      stopMotors();
      break;

    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(clientNum);
        Serial.printf("[%u] WebSocket connected from %d.%d.%d.%d\n",
                      clientNum, ip[0], ip[1], ip[2], ip[3]);
      }
      break;

    case WStype_TEXT:
      {
        // Expected format: "left,right" (e.g., "75,-50")
        // Optimized parsing: Use char* instead of String to reduce heap fragmentation
        char* comma = strchr((char*)payload, ',');

        if (comma != NULL) {
          *comma = '\0';  // Split the string at comma
          int leftSpeed = atoi((char*)payload);
          int rightSpeed = atoi(comma + 1);

          // Constrain values
          leftSpeed = constrain(leftSpeed, -100, 100);
          rightSpeed = constrain(rightSpeed, -100, 100);

          // Store command
          currentCmd.leftSpeed = leftSpeed;
          currentCmd.rightSpeed = rightSpeed;

          // Update watchdog timer
          lastCommandTime = millis();

          // Optional: Send acknowledgment back to client
          // webSocket.sendTXT(clientNum, "OK");
        }
      }
      break;

    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      // Not used in this application
      break;
  }
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(100);
  
  // Setup motors
  setupMotors();
  
  Serial.println("\n=== ESP32 Car - Proportional Control ===");
  Serial.println("Motor threshold: 5%");
  Serial.print("PWM range: ");
  Serial.print(MIN_PWM_OUTPUT);
  Serial.print("-");
  Serial.println(MAX_PWM_OUTPUT);
  Serial.println("Safety timeout: 300ms | Heartbeat: 75ms");
  Serial.println("Control: Full 360-degree analog steering");
  Serial.println("Memory optimized: No HTTP server, WebSocket only\n");

  // WiFi Configuration using WiFiManager
  WiFiManager wifiManager;

  // Uncomment to reset saved WiFi credentials (for testing)
  // wifiManager.resetSettings();

  // Set timeout for configuration portal (3 minutes)
  wifiManager.setConfigPortalTimeout(180);

  Serial.println("Connecting to WiFi...");

  // Auto-connect to saved WiFi or start configuration portal
  if (!wifiManager.autoConnect(AP_CONFIG_NAME)) {
    Serial.println("Failed to connect and timeout reached");
    Serial.println("Restarting ESP32...");
    delay(3000);
    ESP.restart();
  }

  // Successfully connected to WiFi
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Start mDNS service
  if (MDNS.begin(MDNS_HOSTNAME)) {
    Serial.print("mDNS started: http://");
    Serial.print(MDNS_HOSTNAME);
    Serial.println(".local");
    Serial.print("WebSocket: ws://");
    Serial.print(MDNS_HOSTNAME);
    Serial.println(".local:81");

    // Add service to mDNS-SD
    MDNS.addService("ws", "tcp", 81);
  } else {
    Serial.println("Error starting mDNS");
  }

  // WebSocket Server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("\nWebSocket server started on port 81");

  Serial.println("\n=== Ready! ===");
  Serial.println("1. Double-click controller.html (or use full file path)");
  Serial.print("2. Enter: ");
  Serial.print(MDNS_HOSTNAME);
  Serial.println(".local");
  Serial.print("   Or IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("3. Click 'Connect'");
  Serial.println();
}

MotorCommand lastCmd = {0, 0};
bool safetyStopActive = false;

void loop() {
  webSocket.loop();  // Handle WebSocket events

  // CRITICAL SAFETY FEATURE: Auto-stop if no command received
  unsigned long currentTime = millis();
  unsigned long timeSinceLastCommand = currentTime - lastCommandTime;

  if (timeSinceLastCommand > SAFETY_TIMEOUT_MS) {
    // Timeout exceeded - force stop for safety
    if (!safetyStopActive) {
      currentCmd.leftSpeed = 0;
      currentCmd.rightSpeed = 0;
      setLeftMotor(0);
      setRightMotor(0);
      safetyStopActive = true;
      Serial.println("SAFETY STOP: No command timeout");
    }
  } else {
    safetyStopActive = false;
  }

  // Read volatile struct values safely
  int leftSpeed = currentCmd.leftSpeed;
  int rightSpeed = currentCmd.rightSpeed;

  // CRITICAL: If both speeds are zero, force immediate stop (no caching)
  if (leftSpeed == 0 && rightSpeed == 0) {
    if (lastCmd.leftSpeed != 0 || lastCmd.rightSpeed != 0) {
      stopMotors();  // Immediate hardware stop
      lastCmd.leftSpeed = 0;
      lastCmd.rightSpeed = 0;
      // Serial.println("STOP: Zero command received");
    }
  }
  // Apply motor commands only when changed
  else if (lastCmd.leftSpeed != leftSpeed || lastCmd.rightSpeed != rightSpeed) {
    lastCmd.leftSpeed = leftSpeed;
    lastCmd.rightSpeed = rightSpeed;

    setLeftMotor(leftSpeed);
    setRightMotor(rightSpeed);

    // DEBUG: Uncomment next line to see motor commands in Serial Monitor
    // This helps diagnose vibration or wrong direction issuesc
    // Serial.printf("Motors -> Left: %d%% | Right: %d%%\n", leftSpeed, rightSpeed);
  }

  delay(5);  // Faster loop for better responsiveness
}

// Anti-vibration + Auto-brake on direction reversal
// Features: Dead zone 25px, Min threshold 15%, 80ms brake on forward<->backward reversal
// Please work
