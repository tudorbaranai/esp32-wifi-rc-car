// ===========================================
// AUTONOMOUS DRIVING CAR - ESP32
// ===========================================
// Single ultrasonic sensor, obstacle avoidance
// Smart escape: alternates turn direction when stuck
// ===========================================

// ---------- Ultrasonic Sensor Pins ----------
#define TRIG_PIN 13
#define ECHO_PIN 14

// ---------- Motor Pins (L298N) ----------
#define IN1 21  // Left motor direction A
#define IN2 19  // Left motor direction B
#define IN3 18  // Right motor direction A
#define IN4 23  // Right motor direction B
#define ENA 25  // Left motor speed (PWM)
#define ENB 26  // Right motor speed (PWM)

// ---------- Motor Configuration ----------
const bool LEFT_MOTOR_REVERSED  = false;
const bool RIGHT_MOTOR_REVERSED = true;

// ---------- PWM Settings ----------
const int PWM_FREQ = 1000;
const int PWM_RESOLUTION = 8;

// ---------- Speed Settings ----------
const int AUTONOMOUS_SPEED = 85;      // Reduced from 80 to save battery
const int TURN_SPEED = 55;            // Reduced - slower turns save battery
const int REVERSE_SPEED = 50;         // Speed when reversing
const int MIN_PWM = 180;              // Minimum PWM to move motors

// ---------- Obstacle Avoidance Settings ----------
const int OBSTACLE_DISTANCE_CM = 40;  // React when obstacle closer than this
const int REVERSE_TIME_MS = 500;      // How long to reverse (increased)
const int TURN_TIME_MS = 400;         // How long to turn (reduced - was turning too much)
const int MAX_TURN_ATTEMPTS = 3;      // After this many turns, try opposite direction

// ---------- State Machine ----------
enum CarState {
  DRIVING_FORWARD,
  STOPPING,
  REVERSING,
  TURNING,
  RESUMING
};

CarState currentState = DRIVING_FORWARD;
unsigned long stateStartTime = 0;

// ---------- Escape Logic ----------
int turnAttempts = 0;           // How many times we've turned in current escape
bool turnDirectionRight = true; // Current turn direction

// ===========================================
// SENSOR FUNCTIONS
// ===========================================

float readSingleDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) {
    return 999;
  }

  return duration * 0.0343 / 2;
}

float getDistance() {
  return readSingleDistance();
}

// ===========================================
// MOTOR FUNCTIONS
// ===========================================

int speedToPWM(int speedPercent) {
  if (speedPercent == 0) return 0;
  int pwm = map(abs(speedPercent), 1, 100, MIN_PWM, 255);
  return constrain(pwm, 0, 255);
}

void setLeftMotor(int speedPercent) {
  int direction = speedPercent;
  if (LEFT_MOTOR_REVERSED) direction = -direction;

  int pwm = speedToPWM(speedPercent);

  if (direction > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else if (direction < 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    pwm = 0;
  }
  ledcWrite(ENA, pwm);
}

void setRightMotor(int speedPercent) {
  int direction = speedPercent;
  if (RIGHT_MOTOR_REVERSED) direction = -direction;

  int pwm = speedToPWM(speedPercent);

  if (direction > 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else if (direction < 0) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    pwm = 0;
  }
  ledcWrite(ENB, pwm);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);
}

void driveForward(int speed) {
  setLeftMotor(speed);
  setRightMotor(speed);
}

void driveBackward(int speed) {
  setLeftMotor(-speed);
  setRightMotor(-speed);
}

void turn(int speed, bool right) {
  int pwm = speedToPWM(speed);

  if (right) {
    // Turn RIGHT: left forward, right backward
    int leftDir = LEFT_MOTOR_REVERSED ? -1 : 1;
    if (leftDir > 0) {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
    } else {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
    }

    int rightDir = RIGHT_MOTOR_REVERSED ? 1 : -1;
    if (rightDir > 0) {
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
    } else {
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
    }
  } else {
    // Turn LEFT: left backward, right forward
    int leftDir = LEFT_MOTOR_REVERSED ? 1 : -1;
    if (leftDir > 0) {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
    } else {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
    }

    int rightDir = RIGHT_MOTOR_REVERSED ? -1 : 1;
    if (rightDir > 0) {
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
    } else {
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
    }
  }

  ledcWrite(ENA, pwm);
  ledcWrite(ENB, pwm);
}

// ===========================================
// STATE MACHINE
// ===========================================

void changeState(CarState newState) {
  currentState = newState;
  stateStartTime = millis();

  switch (newState) {
    case DRIVING_FORWARD:
      Serial.println(">> FORWARD");
      break;
    case STOPPING:
      Serial.println(">> STOP");
      break;
    case REVERSING:
      Serial.println(">> REVERSE");
      break;
    case TURNING:
      Serial.print(">> TURN ");
      Serial.println(turnDirectionRight ? "RIGHT" : "LEFT");
      break;
    case RESUMING:
      Serial.println(">> CHECK");
      break;
  }
}

unsigned long getTimeInState() {
  return millis() - stateStartTime;
}

// ===========================================
// SETUP
// ===========================================

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  ledcAttach(ENA, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(ENB, PWM_FREQ, PWM_RESOLUTION);

  stopMotors();

  Serial.println("\n================================");
  Serial.println("  AUTONOMOUS CAR - SMART ESCAPE");
  Serial.println("================================");
  Serial.print("Obstacle: ");
  Serial.print(OBSTACLE_DISTANCE_CM);
  Serial.println(" cm");
  Serial.print("Speed: ");
  Serial.print(AUTONOMOUS_SPEED);
  Serial.println("%");
  Serial.println("================================\n");

  Serial.println("Starting in 3 seconds...");
  delay(3000);

  changeState(DRIVING_FORWARD);
}

// ===========================================
// MAIN LOOP
// ===========================================

void loop() {
  float distance = getDistance();

  switch (currentState) {

    case DRIVING_FORWARD:
      driveForward(AUTONOMOUS_SPEED);

      if (distance < OBSTACLE_DISTANCE_CM) {
        Serial.print("! OBSTACLE: ");
        Serial.print(distance, 0);
        Serial.println(" cm");
        turnAttempts = 0;  // Reset escape counter
        changeState(STOPPING);
      }
      break;

    case STOPPING:
      stopMotors();
      delay(150);  // Brief pause to let motors stop
      changeState(REVERSING);
      break;

    case REVERSING:
      driveBackward(REVERSE_SPEED);

      if (getTimeInState() >= REVERSE_TIME_MS) {
        stopMotors();
        delay(100);
        changeState(TURNING);
      }
      break;

    case TURNING:
      turn(TURN_SPEED, turnDirectionRight);

      if (getTimeInState() >= TURN_TIME_MS) {
        stopMotors();
        delay(100);
        turnAttempts++;
        changeState(RESUMING);
      }
      break;

    case RESUMING:
      stopMotors();
      delay(150);

      distance = getDistance();
      Serial.print("Check: ");
      Serial.print(distance, 0);
      Serial.println(" cm");

      if (distance >= OBSTACLE_DISTANCE_CM) {
        // Path clear!
        Serial.println("Path clear!");
        turnAttempts = 0;
        changeState(DRIVING_FORWARD);
      }
      else if (turnAttempts >= MAX_TURN_ATTEMPTS) {
        // Stuck! Switch direction and reverse more
        turnDirectionRight = !turnDirectionRight;  // Flip direction
        turnAttempts = 0;
        Serial.print("STUCK! Switching to ");
        Serial.println(turnDirectionRight ? "RIGHT" : "LEFT");
        changeState(REVERSING);  // Reverse again before turning other way
      }
      else {
        // Still blocked, turn more same direction
        Serial.println("Still blocked...");
        changeState(TURNING);
      }
      break;
  }

  delay(50);  // Small delay between loops
}
