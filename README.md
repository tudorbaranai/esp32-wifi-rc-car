# ESP32 WiFi Controlled Car

A WiFi-controlled robot car using ESP32 with an intuitive web-based joystick interface.

## Features

- **WiFi Access Point** - ESP32 creates its own network
- **Web-Based Joystick Control** - Touch-friendly interface for mobile devices
- **Dual Motor Control** - L298N motor driver with PWM speed control
- **Safety Timeout** - Automatic stop if connection is lost (500ms)
- **Continuous Heartbeat** - Ensures reliable communication (100ms updates)
- **Responsive Controls** - Real-time direction and speed display

## Hardware Requirements

- ESP32 Development Board
- L298N Motor Driver Module
- 2x DC Motors
- Power supply (suitable for motors and ESP32)
- Robot car chassis

## Pin Configuration

| Pin | Function |
|-----|----------|
| 21  | Left motor direction A (IN1) |
| 19  | Left motor direction B (IN2) |
| 18  | Right motor direction A (IN3) |
| 23  | Right motor direction B (IN4) |
| 25  | Left motor PWM speed (ENA) |
| 26  | Right motor PWM speed (ENB) |

## Software Features

### Motor Control
- **Min Speed Threshold**: 40% (filters small joystick movements)
- **PWM Range**: 180-255 (optimized to overcome motor friction)
- **Dead Zone**: 15 pixels around joystick center
- **Motor Reversal Flags**: Easy correction for wiring differences

### Safety Features
- **Automatic Safety Timeout**: Motors stop if no command received for 500ms
- **Continuous Heartbeat System**: Browser sends updates every 100ms while joystick is active
- **Fail-Safe Design**: Car stops automatically on connection loss or browser crash

## Setup Instructions

1. **Upload Code**
   - Open `Larisuta_masina_v4.ino` in Arduino IDE
   - Select board: ESP32 Dev Module
   - Upload to your ESP32

2. **Connect to WiFi**
   - Network Name: `ESP32-Car`
   - Password: `car12345`

3. **Open Web Interface**
   - Navigate to: `http://192.168.4.1` (default ESP32 AP IP)
   - Use the joystick to control your car

## Usage

- **Forward**: Push joystick up
- **Backward**: Push joystick down
- **Left Turn**: Push joystick left (spin in place)
- **Right Turn**: Push joystick right (spin in place)
- **Stop**: Release joystick (automatic return to center)

## Configuration

You can adjust these parameters in the code:

```cpp
// WiFi credentials
const char* AP_SSID  = "ESP32-Car";
const char* AP_PASS  = "car12345";

// Motor thresholds
const int MIN_SPEED_THRESHOLD = 40;   // Minimum speed percentage
const int MIN_PWM_OUTPUT = 180;       // Minimum PWM value

// Safety timeout
const unsigned long SAFETY_TIMEOUT_MS = 500;  // Auto-stop timeout

// Motor reversal (if motors spin wrong direction)
const bool LEFT_MOTOR_REVERSED  = true;
const bool RIGHT_MOTOR_REVERSED = false;
```

## Troubleshooting

**Car doesn't move:**
- Check motor power supply
- Verify motor driver connections
- Check if PWM threshold is too high for your motors

**Car moves in wrong direction:**
- Adjust `LEFT_MOTOR_REVERSED` and `RIGHT_MOTOR_REVERSED` flags

**Connection issues:**
- Ensure you're connected to the ESP32-Car WiFi network
- Check if ESP32 IP is 192.168.4.1 (check Serial Monitor)

**Safety stop activating during normal use:**
- Check WiFi signal strength
- Adjust `SAFETY_TIMEOUT_MS` if needed (increase to 1000ms)

## Technical Details

- **PWM Frequency**: 1 kHz
- **PWM Resolution**: 8-bit (0-255)
- **Heartbeat Interval**: 100ms
- **Safety Timeout**: 500ms
- **Dead Zone**: 15 pixels

## License

Open source - feel free to modify and improve!

## Version History

- **v4** - Current version with safety timeout and heartbeat system
- Enhanced motor control with tuned thresholds
- Web-based joystick interface with touch support
