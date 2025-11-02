#include <WiFi.h>
#include <WebServer.h>

// ---------- Hardware pins ----------
#define IN1 21  // Left motor direction A
#define IN2 19  // Left motor direction B
#define IN3 18  // Right motor direction A
#define IN4 23  // Right motor direction B

// PWM Enable pins (L298N speed control)
#define ENA 25  // Left motor speed (PWM)
#define ENB 26  // Right motor speed (PWM)

// Motor reversal flags
const bool LEFT_MOTOR_REVERSED  = true;
const bool RIGHT_MOTOR_REVERSED = false;

// PWM settings
const int PWM_FREQ = 1000;      // 1 kHz
const int PWM_RESOLUTION = 8;   // 8-bit (0-255)

// Motor threshold settings - TUNED FOR YOUR SETUP
const int MIN_SPEED_THRESHOLD = 40;   // Ignore commands below 40%
const int MIN_PWM_OUTPUT = 180;       // Minimum PWM to overcome friction (70% of 255)

// ---------- WiFi Access Point ----------
const char* AP_SSID  = "ESP32-Car";
const char* AP_PASS  = "car12345";

// ---------- Web server ----------
WebServer server(80);

// ---------- Motor state ----------
struct MotorCommand {
  int leftSpeed;   // -100 to +100 (percentage)
  int rightSpeed;  // -100 to +100 (percentage)
};

volatile MotorCommand currentCmd = {0, 0};

// ---------- Safety timeout ----------
const unsigned long SAFETY_TIMEOUT_MS = 500;  // Stop if no command for 500ms
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
  
  // Map MIN_SPEED_THRESHOLD-100% to MIN_PWM_OUTPUT-255 PWM
  // This gives smooth acceleration from minimum working speed to full speed
  int pwmValue = map(absSpeed, MIN_SPEED_THRESHOLD, 100, MIN_PWM_OUTPUT, 255);
  
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

// ---------- HTTP handlers ----------
void handleRoot() {
  const char* html = R"rawliteral(
<!doctype html>
<html>
<head>
  <meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
  <title>ESP32 Car - Joystick</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      margin: 0;
      padding: 10px;
      background: #111;
      color: #fff;
      overflow: hidden;
    }
    h2 { margin: 10px 0; }
    #joystickContainer {
      position: relative;
      width: 300px;
      height: 300px;
      margin: 20px auto;
      background: #222;
      border-radius: 50%;
      border: 3px solid #444;
      touch-action: none;
    }
    #joystick {
      position: absolute;
      width: 100px;
      height: 100px;
      background: radial-gradient(circle, #0a84ff, #0056b3);
      border-radius: 50%;
      top: 100px;
      left: 100px;
      box-shadow: 0 4px 20px rgba(10, 132, 255, 0.5);
      transition: box-shadow 0.1s;
    }
    #joystick:active {
      box-shadow: 0 6px 30px rgba(10, 132, 255, 0.8);
    }
    #info {
      margin-top: 20px;
      font-size: 14px;
      color: #aaa;
    }
    .status {
      display: inline-block;
      margin: 5px 10px;
    }
    .active {
      color: #0a84ff;
      font-weight: bold;
    }
  </style>
</head>
<body>
  <h2>🎮 ESP32 Car</h2>
  <div id="joystickContainer">
    <div id="joystick"></div>
  </div>
  <div id="info">
    <div class="status">Direction: <span id="direction">Center</span></div>
    <div class="status">Speed: <span id="speed">0%</span></div>
  </div>

<script>
const container = document.getElementById('joystickContainer');
const joystick = document.getElementById('joystick');
const directionDisplay = document.getElementById('direction');
const speedDisplay = document.getElementById('speed');

const containerSize = 300;
const joystickSize = 100;
const maxDistance = (containerSize - joystickSize) / 2;
const centerX = maxDistance;
const centerY = maxDistance;

let isDragging = false;
let currentX = centerX;
let currentY = centerY;

// Increased dead zone for reliable stopping
const DEAD_ZONE = 15;  // Pixels from center before registering movement

// Heartbeat system for continuous safety updates
let heartbeatInterval = null;
let lastLeftSpeed = 0;
let lastRightSpeed = 0;
const HEARTBEAT_INTERVAL_MS = 100;  // Send updates every 100ms

function updateJoystick(x, y) {
  // Calculate distance from center
  const deltaX = x - centerX;
  const deltaY = y - centerY;
  const distance = Math.sqrt(deltaX * deltaX + deltaY * deltaY);
  
  // Limit to max distance
  let finalX, finalY;
  if (distance > maxDistance) {
    const angle = Math.atan2(deltaY, deltaX);
    finalX = centerX + maxDistance * Math.cos(angle);
    finalY = centerY + maxDistance * Math.sin(angle);
  } else {
    finalX = x;
    finalY = y;
  }
  
  // Update visual position
  joystick.style.left = finalX + 'px';
  joystick.style.top = finalY + 'px';
  
  currentX = finalX;
  currentY = finalY;
  
  // Calculate direction and speed
  const finalDeltaX = finalX - centerX;
  const finalDeltaY = finalY - centerY;
  const finalDistance = Math.sqrt(finalDeltaX * finalDeltaX + finalDeltaY * finalDeltaY);
  const speedPercent = Math.round((finalDistance / maxDistance) * 100);
  
  // Determine direction
  let direction = 'Center';
  let leftSpeed = 0;
  let rightSpeed = 0;
  
  // Apply dead zone - critical for stopping
  if (finalDistance > DEAD_ZONE) {
    const angle = Math.atan2(-finalDeltaY, finalDeltaX) * (180 / Math.PI);
    
    // Map joystick position to motor commands
    if (angle >= -45 && angle < 45) {
      // Right - spin in place
      direction = 'Right';
      leftSpeed = speedPercent;
      rightSpeed = -speedPercent;
    } else if (angle >= 45 && angle < 135) {
      // Forward
      direction = 'Forward';
      leftSpeed = speedPercent;
      rightSpeed = speedPercent;
    } else if (angle >= -135 && angle < -45) {
      // Backward
      direction = 'Backward';
      leftSpeed = -speedPercent;
      rightSpeed = -speedPercent;
    } else {
      // Left - spin in place
      direction = 'Left';
      leftSpeed = -speedPercent;
      rightSpeed = speedPercent;
    }
    
    // Visual feedback
    speedDisplay.className = 'active';
    directionDisplay.className = 'active';
  } else {
    // Inside dead zone - force stop
    leftSpeed = 0;
    rightSpeed = 0;
    speedDisplay.className = '';
    directionDisplay.className = '';
  }
  
  // Update display
  directionDisplay.textContent = direction;
  speedDisplay.textContent = speedPercent + '%';

  // Store current speeds (heartbeat will send them)
  lastLeftSpeed = leftSpeed;
  lastRightSpeed = rightSpeed;

  // Send immediately for responsiveness
  sendCommand(leftSpeed, rightSpeed);
}

function returnToCenter() {
  joystick.style.left = centerX + 'px';
  joystick.style.top = centerY + 'px';
  currentX = centerX;
  currentY = centerY;
  directionDisplay.textContent = 'Center';
  directionDisplay.className = '';
  speedDisplay.textContent = '0%';
  speedDisplay.className = '';

  // Stop heartbeat
  stopHeartbeat();

  // Store zero speeds
  lastLeftSpeed = 0;
  lastRightSpeed = 0;

  // Critical: Send stop command
  sendCommand(0, 0);
}

function sendCommand(leftSpeed, rightSpeed) {
  fetch(`/move?left=${leftSpeed}&right=${rightSpeed}`)
    .catch(e => console.log('Connection error'));
}

// Heartbeat functions - ensure continuous safety updates
function startHeartbeat() {
  if (heartbeatInterval === null) {
    heartbeatInterval = setInterval(() => {
      // Continuously send current command to keep ESP32 watchdog alive
      sendCommand(lastLeftSpeed, lastRightSpeed);
    }, HEARTBEAT_INTERVAL_MS);
  }
}

function stopHeartbeat() {
  if (heartbeatInterval !== null) {
    clearInterval(heartbeatInterval);
    heartbeatInterval = null;
  }
}

// Mouse events
joystick.addEventListener('mousedown', (e) => {
  isDragging = true;
  startHeartbeat();  // Start continuous updates
  e.preventDefault();
});

container.addEventListener('mousemove', (e) => {
  if (!isDragging) return;
  const rect = container.getBoundingClientRect();
  const x = e.clientX - rect.left - joystickSize / 2;
  const y = e.clientY - rect.top - joystickSize / 2;
  updateJoystick(x, y);
});

document.addEventListener('mouseup', () => {
  if (isDragging) {
    isDragging = false;
    returnToCenter();
  }
});

// Touch events
joystick.addEventListener('touchstart', (e) => {
  isDragging = true;
  startHeartbeat();  // Start continuous updates
  e.preventDefault();
});

container.addEventListener('touchmove', (e) => {
  if (!isDragging) return;
  e.preventDefault();
  const touch = e.touches[0];
  const rect = container.getBoundingClientRect();
  const x = touch.clientX - rect.left - joystickSize / 2;
  const y = touch.clientY - rect.top - joystickSize / 2;
  updateJoystick(x, y);
});

document.addEventListener('touchend', () => {
  if (isDragging) {
    isDragging = false;
    returnToCenter();
  }
});

document.addEventListener('touchcancel', () => {
  if (isDragging) {
    isDragging = false;
    returnToCenter();
  }
});

// Initialize
returnToCenter();
</script>
</body>
</html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleMove() {
  if (!server.hasArg("left") || !server.hasArg("right")) {
    server.send(400, "text/plain", "Missing parameters");
    return;
  }

  int leftSpeed = server.arg("left").toInt();
  int rightSpeed = server.arg("right").toInt();

  // Constrain values
  leftSpeed = constrain(leftSpeed, -100, 100);
  rightSpeed = constrain(rightSpeed, -100, 100);

  // Store as percentage (threshold applied in motor functions)
  currentCmd.leftSpeed = leftSpeed;
  currentCmd.rightSpeed = rightSpeed;

  // Update watchdog timer - critical for safety timeout
  lastCommandTime = millis();

  server.send(200, "text/plain", "OK");
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(100);
  
  // Setup motors
  setupMotors();
  
  Serial.println("\n=== ESP32 Car with Tuned Joystick ===");
  Serial.println("Motor threshold: 40%");
  Serial.println("PWM range: 180-255");
  Serial.println("Safety timeout: 500ms (auto-stop enabled)");
  
  // WiFi Access Point
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  
  bool apStarted = WiFi.softAP(AP_SSID, AP_PASS);
  
  if (apStarted) {
    Serial.print("WiFi: ");
    Serial.println(AP_SSID);
    Serial.print("Password: ");
    Serial.println(AP_PASS);
    Serial.print("URL: http://");
    Serial.println(WiFi.softAPIP());
  }
  
  // HTTP Server
  server.on("/", handleRoot);
  server.on("/move", handleMove);
  server.onNotFound([](){ server.send(404, "text/plain", "Not found"); });
  
  server.begin();
  Serial.println("Ready!\n");
}

MotorCommand lastCmd = {0, 0};
bool safetyStopActive = false;

void loop() {
  server.handleClient();

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

  // Apply motor commands only when changed
  if (lastCmd.leftSpeed != leftSpeed || lastCmd.rightSpeed != rightSpeed) {

    lastCmd.leftSpeed = leftSpeed;
    lastCmd.rightSpeed = rightSpeed;

    setLeftMotor(leftSpeed);
    setRightMotor(rightSpeed);

    // Debug output (uncomment to see values)
    // Serial.printf("Command: L=%d%% R=%d%%\n", leftSpeed, rightSpeed);
  }

  delay(10);
}
