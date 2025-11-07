# ESP32 WiFi Controlled Car

A professional-grade WiFi-controlled robot car using ESP32 with **optimized WebSocket architecture** and web-based joystick interface.

## ✨ Key Features

### Network & Communication
- **WebSocket-Only Architecture** - No HTTP server overhead (~18KB saved)
- **Station Mode** - Joins your home WiFi (phone keeps internet!)
- **Auto WiFi Configuration** - WiFiManager portal for easy setup
- **mDNS Support** - Access via `esp32-car.local` (no IP needed)
- **Real-time Bidirectional Communication** - WebSocket for ultra-low latency

### Control & Safety
- **Web-Based Joystick** - Works on any device, no app needed
- **Differential Steering** - Independent left/right motor control
- **Safety Timeout** - Automatic stop if connection is lost (300ms)
- **Direction Reversal Braking** - Prevents motor fighting (80ms brake)
- **Continuous Heartbeat** - Keeps connection alive (75ms updates)
- **Dead Zone & Anti-Vibration** - Smooth, precise control

### Optimization
- **Memory Optimized** - Reduced flash usage (~42KB vs ~60KB)
- **Reduced Heap Fragmentation** - char[] parsing instead of String class
- **Lower Latency** - ~50ms WebSocket vs ~100ms HTTP
- **95% Bandwidth Reduction** - 9 bytes vs 500 bytes per command

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

## 🚀 Quick Setup

### Required Libraries

Install via Arduino IDE Library Manager:
- **WiFiManager** by tzapu
- **WebSockets** by Markus Sattler

### Step 1: Upload Code

1. Open `Larisuta_masina_v4.ino` in Arduino IDE
2. Select board: **ESP32 Dev Module**
3. Upload to your ESP32

### Step 2: First-Time WiFi Configuration

1. ESP32 creates hotspot: **`ESP32-Car-Setup`**
2. Connect your phone/laptop to this network
3. Configuration portal opens automatically (or go to `192.168.4.1`)
4. Select your home WiFi and enter password
5. Click Save - ESP32 restarts and connects

**WiFi credentials are saved!** ESP32 will auto-connect next time.

### Step 3: Open Controller

**Option A - Local File:**
- Open `controller.html` in your browser
- Enter `esp32-car.local` or the IP address shown in Serial Monitor
- Click "Connect"

**Option B - GitHub Pages (Recommended):**
- See `SETUP_GUIDE.md` for hosting instructions
- Access from any device with internet

📖 **Detailed setup guide:** See `SETUP_GUIDE.md`

## Usage

- **Forward**: Push joystick up
- **Backward**: Push joystick down
- **Left Turn**: Push joystick left (spin in place)
- **Right Turn**: Push joystick right (spin in place)
- **Stop**: Release joystick (automatic return to center)

## ⚙️ Configuration

Key parameters you can adjust in the code:

```cpp
// Network configuration
const char* MDNS_HOSTNAME = "esp32-car";           // Line 30: Access via esp32-car.local
const char* AP_CONFIG_NAME = "ESP32-Car-Setup";    // Line 31: WiFi setup portal name

// Motor thresholds
const int MIN_SPEED_THRESHOLD = 40;   // Line 26: Minimum speed percentage (40%)
const int MIN_PWM_OUTPUT = 180;       // Line 27: Minimum PWM value (180/255)

// Safety timeout
const unsigned long SAFETY_TIMEOUT_MS = 300;  // Line 43: Auto-stop timeout (300ms)

// Motor reversal (if motors spin wrong direction)
const bool LEFT_MOTOR_REVERSED  = true;   // Line 16
const bool RIGHT_MOTOR_REVERSED = false;  // Line 17

// Heartbeat (in controller.html JavaScript)
const HEARTBEAT_INTERVAL_MS = 75;         // WebSocket update rate (75ms)
const REVERSE_BRAKE_TIME_MS = 80;         // Direction reversal brake (80ms)
```

## 🐛 Troubleshooting

**Cannot connect to `esp32-car.local`:**
- Use IP address instead (check Serial Monitor at 115200 baud)
- Windows users: Install Bonjour Print Services for mDNS
- Android users: mDNS may not work - use IP address

**WebSocket connection fails:**
- Ensure phone/computer is on the **same WiFi network** as ESP32
- Check Serial Monitor shows "WebSocket server started on port 81"
- Try IP address instead of hostname
- Check firewall settings (allow port 81)

**Car doesn't move:**
- Check motor power supply (sufficient voltage/current)
- Verify motor driver connections (pins 21, 19, 18, 23, 25, 26)
- Test with wheels off ground first
- Check PWM threshold isn't too high for your motors

**Car moves in wrong direction:**
- Adjust `LEFT_MOTOR_REVERSED` and `RIGHT_MOTOR_REVERSED` flags (lines 16-17)
- Swap motor wires if needed

**WiFi configuration portal doesn't appear:**
- Uncomment `wifiManager.resetSettings();` in code (line 215)
- Upload, then comment it out again
- Connect to `ESP32-Car-Setup` network manually
- Go to `192.168.4.1` in browser

**Motors vibrate/oscillate:**
- Increase `MIN_SPEED_THRESHOLD` (currently 40%)
- Increase dead zone in controller.html
- Check motor power supply stability

📖 **More help:** See `SETUP_GUIDE.md` for detailed troubleshooting

## 📊 Technical Specifications

| Specification | Value |
|--------------|-------|
| **Communication Protocol** | WebSocket (RFC 6455) |
| **Message Format** | Text: "left,right" (e.g., "75,-50") |
| **Network Mode** | Station (WiFi client) |
| **WebSocket Port** | 81 |
| **mDNS Hostname** | esp32-car.local |
| **Update Rate** | ~13 Hz (75ms heartbeat) |
| **Average Latency** | ~50ms |
| **Safety Timeout** | 300ms |
| **Direction Reversal Brake** | 80ms |
| **PWM Frequency** | 1 kHz |
| **PWM Resolution** | 8-bit (0-255) |
| **PWM Range** | 180-255 (tuned for friction) |
| **Dead Zone** | 25 pixels |
| **Flash Memory** | ~42KB (optimized) |
| **Heap Memory** | ~10KB |

## 📁 Project Files

- `Larisuta_masina_v4.ino` - ESP32 WebSocket server code
- `controller.html` - Web-based joystick interface
- `SETUP_GUIDE.md` - Detailed setup and hosting instructions
- `README.md` - This file

## License

Open source - feel free to modify and improve!

## 📜 Version History

- **v5 (2025)** - **OPTIMIZED ARCHITECTURE**
  - WebSocket-only communication (removed HTTP server)
  - Station mode with WiFiManager auto-configuration
  - mDNS support (`esp32-car.local`)
  - External web controller (GitHub Pages compatible)
  - Memory optimizations (~18KB flash saved)
  - Reduced heap fragmentation (char[] instead of String)
  - Improved latency (~50ms vs ~100ms)

- **v4** - Safety timeout and heartbeat system
  - Enhanced motor control with tuned thresholds
  - Web-based joystick interface with touch support
  - Direction reversal braking

---

**Made with ❤️ for robotics enthusiasts**

For detailed setup instructions, see **`SETUP_GUIDE.md`**
