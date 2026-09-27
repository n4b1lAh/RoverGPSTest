# Mars Rover GPS // Tactical Ground Station & Live Radar

Real-time satellite telemetry and 100% offline tactical navigation dashboard for Mars Rover testing using **ESP32** and **RadioLink SE100 (u-blox NEO-M8N)** GPS.

---

## 🛰️ Features

- **100% Offline Tactical Navigation Radar**: Runs entirely from ESP32 Flash memory with zero external dependencies (no internet, CDNs, or cellular connection required in the field).
- **1m / 2m Metric Precision Grid**: Concentric distance circles (1m, 2m, 5m, 10m) with high-precision local flat-earth equirectangular metric projection.
- **Relative Odometry**: Real-time displacement $\Delta X$ (East) and $\Delta Y$ (North) from origin $(0, 0)$.
- **Live Breadcrumb Trail**: Tracks and displays rover movement history.
- **HUD Telemetry**: Live Lat/Long, Satellites, HDOP precision rating, Speed, Altitude, and UTC Mission Time.
- **Direct Wi-Fi Access Point**: Connect any phone, laptop, or tablet to the rover's Wi-Fi network to open the dashboard.

---

## 🔌 Hardware Wiring

| SE100 Wire Color | Function | ESP32 Pin | Notes |
| :--- | :--- | :--- | :--- |
| **Red** | VCC | **VIN / 5V** | **Crucial:** Must connect to 5V (not 3.3V) |
| **Black** | GND | **GND** | Shared ground |
| **Orange** | GPS TX | **GPIO 16 (RX2)** | GPS transmitter connects to ESP32 RX |
| **White** | GPS RX | **GPIO 17 (TX2)** | GPS receiver connects to ESP32 TX |
| **Green** | I2C SCL | *Optional* | Onboard compass |
| **Yellow** | I2C SDA | *Optional* | Onboard compass |

---

## 🚀 Quick Start

1. Open this project in **VS Code** with **PlatformIO**.
2. Connect your ESP32 via USB and click **Upload (➔)**.
3. Once flashed, connect your laptop or phone to the rover's Wi-Fi:
   - **SSID:** `MarsRover-GroundStation`
   - **Password:** `rover1234`
4. Open your browser and navigate to:
   ```text
   http://192.168.4.1
   ```

---

## 🛠️ Tech Stack

- **Firmware:** C++ / Arduino Framework on ESP32 (`TinyGPSPlus`, `WiFi`, `WebServer`, `HardwareSerial`)
- **Dashboard:** HTML5 Canvas, Vanilla CSS3, WebSockets/REST API
- **Build System:** PlatformIO
