# WiFi Auto-Connect with ESP32 and SD Card

This project enables an ESP32 to automatically connect to WiFi networks using credentials stored in a JSON file on an SD card. The device scans for available networks and attempts to connect using the stored credentials.

## ✨ Features

- **Automatic WiFi Connection**: Connects to known networks from a credentials file
- **SD Card Support**: Stores WiFi credentials on a microSD card
- **JSON Configuration**: Easy-to-edit JSON format for network credentials
- **Serial Debugging**: Detailed output for troubleshooting
- **Flexible Pin Configuration**: Supports both VSPI and HSPI interfaces
- **Display Support**: Supports OLED display

## 📋 Prerequisites

- ESP32 development board
- MicroSD card module
- MicroSD card (formatted as FAT32)
- OLED display
- Jumper wires

## 🔌 Pin Connections

### Standard SD Card Connection
| SD Card Pin | ESP32 Pin | Description |
|-------------|-----------|-------------|
| CS          | GPIO5     | Chip Select |
| DI (MOSI)   | GPIO23    | Data In     |
| DO (MISO)   | GPIO19    | Data Out    |
| SCK         | GPIO18    | Clock       |
| VCC         | 3.3V      | Power       |
| GND         | GND       | Ground      |

### Alternative SPI Pins
| Interface | MOSI    | MISO    | CLK     | CS     |
|-----------|---------|---------|---------|--------|
| VSPI      | GPIO 23 | GPIO 19 | GPIO 18 | GPIO 5 |
| HSPI      | GPIO 13 | GPIO 12 | GPIO 14 | GPIO 15|

### Display I2C Connection
| ESP32 Pin | Display Pin | Description |
|-----------|-------------|-------------|
| GPIO 22   | SCL         | Clock       |
| GPIO 21   | SDA         | Data        |


## 🚀 Getting Started

### 1. Install Required Software
1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Add ESP32 board support:
   - Add `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` to Additional Board Manager URLs
   - Install "ESP32" from Boards Manager
3. Install required libraries:
   - ArduinoJson by Benoit Blanchon
   - SD (included with Arduino IDE)
   - SPI (included with Arduino IDE)
   - WiFi (included with Arduino IDE)

### 2. Prepare the SD Card
1. Format the SD card as FAT32
2. Create a file named `credentials.json` in the root directory
3. Add your network credentials in the following format:
   ```json
   {
     "YourNetworkSSID1": "YourPassword1",
     "YourNetworkSSID2": "YourPassword2"
   }
   ```

### 3. Upload and Run
1. Connect your ESP32 to your computer
2. Select the correct board and port in Arduino IDE
3. Set the following in `Tools`:
   - Board: "ESP32 Dev Module"
   - Upload Speed: "921600"
   - Flash Frequency: "80MHz"
   - Flash Mode: "QIO"
   - Flash Size: "4MB (32Mb)"
   - PSRAM: "Disabled"
4. Upload the sketch
5. Open Serial Monitor (115200 baud) to view connection status

## 🔧 Functions

- `connectToNetwork()`: Scans and connects to available networks
- `wlanConnect(ssid, password)`: Connects to a specific network
- `displayConnection()`: Shows current connection details
- `listDir()`: Lists files on the SD card
- `readFile()`: Reads file contents

## 🐛 Troubleshooting

### Common Issues
- **SD Card Not Detected**:
  - Check wiring connections
  - Ensure the card is properly formatted as FAT32
  - Try a different SD card

- **WiFi Connection Fails**:
  - Verify SSID and password in `credentials.json`
  - Check if the network is in range
  - Ensure the network is 2.4GHz (ESP32 doesn't support 5GHz)

- **Serial Monitor Shows Garbage**:
  - Ensure baud rate is set to 115200
  - Check for loose connections

## 📝 Notes

- The device will automatically attempt to connect to the accessible network with the best RSSI listed in `credentials.json`
- The SD card must remain inserted while the device is powered on (not sure)

## 📜 License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

### TODO
- [ ] Add support for multiple JSON configuration files
- [ ] Add web interface for configuration and addition of credentials
- [ ] Implement OTA (Over-The-Air) updates