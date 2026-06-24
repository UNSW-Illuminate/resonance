# Resonance Simulator ESP32 Backend

This directory contains the embedded C++/Arduino prototype to run the coordinate-based graph and ripple simulation on the ESP32 hardware itself, driving a physical strip of WS2815 addressable LEDs using the FastLED library.

---

## 🛠️ Required Libraries
Ensure you have the following libraries installed in your Arduino IDE before uploading:
1. **FastLED** (by Daniel Garcia) - for WS2815 LED control.
2. **ArduinoJson** (by Benoit Blanchon) - version 6.x - for parsing map and config JSON files.

---

## 🔌 Hardware Connections (WS2815 LEDs)
WS2815 LEDs are 12V addressable LEDs with standard 3-pin control (12V, GND, Data, and Backup-Data).
- **12V**: Connect to a external 12V power supply.
- **GND**: Connect to the common ground of both the 12V power supply and the ESP32.
- **Data (DI)**: Connect to the ESP32 GPIO pin configured in `esp32-backend.ino` (default: **GPIO 13**).
- **Backup Data (BI)**: Connect to **GND** on the first LED of the strip (this prevents failures if the first LED data pin breaks, but grounding it works perfectly as standard).

*Note: Since ESP32 operates at 3.3V and WS2815 needs 5V logic high for data, you may need a 3.3V-to-5V level shifter (e.g., AHCT125) on the data line for long cables or stable operation.*

---

## 🌐 Connecting and Configuring
1. **WiFi Access Point**:
   On boot, the ESP32 will start its own WiFi network:
   - **SSID**: `Resonance-Simulator`
   - **Password**: `resonance-waves`
2. **Web Dashboard**:
   Connect your computer or smartphone to the WiFi network and open your web browser to:
   - **URL**: `http://192.168.4.1`

---

## 🕹️ Web Interface & Features
The web interface is hosted directly from ESP32 flash memory. Since you requested a simple, functional layout focused on usability, the page includes:
- **Control Settings**:
  - **Mode Selection**: Switch between **Ripple Simulation**, **Solid Color**, **Rainbow Demo**, and **Off**.
  - **Global Brightness**: Slider (0-255) controlling global strip brightness.
  - **Color Picker**: HTML5 pickers for Primary and Secondary colors.
  - **Wave Parameters**: Sliders for Wave Speed, Thickness, Max Radius, and number of wave bands.
- **Interactive SVG Map Visualizer**:
  - Automatically loads and plots node positions in an interactive grid.
  - Color-coded representation: **Green** circles represent **Bushes**, **Orange** circles represent **Reeds**.
  - **Clicking any node** on the SVG instantly sends an API call to the ESP32 to trigger a ripple starting from that node!
- **Map Layout Manager**:
  - Displays currently loaded statistics (node count, bushes vs reeds, total LED count calculated as `nodes * 9`).
  - Textarea to paste/edit raw map configurations (such as `map.json` or `map3.json`) directly on the device.

---

## 🛰️ API Endpoints
If you want to control the simulator programmatically or build another frontend:
- `GET /` - Serves the web control panel.
- `GET /api/status` - Returns a JSON payload showing current settings, active ripples, and node coordinates.
- `POST /api/settings` - Updates settings. Example payload:
  ```json
  {
    "mode": "ripple",
    "brightness": 128,
    "primaryR": 255, "primaryG": 0, "primaryB": 0,
    "secondaryR": 0, "secondaryG": 0, "secondaryB": 0,
    "waveSpeed": 1.2,
    "thickness": 4.5,
    "maxRadius": 25.0,
    "numBands": 3
  }
  ```
- `POST /api/trigger?row=R&col=C` - Triggers a ripple starting at coordinate `(R, C)`.
- `GET /api/map` - Returns the raw `/map.json` stored in LittleFS.
- `POST /api/map` - Uploads a new map layout and rebuilds the node graph links in real-time. Supports both standard `"nodes": [...]` grids and custom `"bushes": [...], "reeds": [...]` coordinate lists.
