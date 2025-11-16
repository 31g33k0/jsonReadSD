/*
 * pin 1 - not used          |  Micro SD card     |
 * pin 2 - CS (SS)           |                   /
 * pin 3 - DI (MOSI)         |                  |__
 * pin 4 - VDD (3.3V)        |                    |
 * pin 5 - SCK (SCLK)        | 8 7 6 5 4 3 2 1   /
 * pin 6 - VSS (GND)         | ▄ ▄ ▄ ▄ ▄ ▄ ▄ ▄  /
 * pin 7 - DO (MISO)         | ▀ ▀ █ ▀ █ ▀ ▀ ▀ |
 * pin 8 - not used          |_________________|
 *                             ║ ║ ║ ║ ║ ║ ║ ║
 *                     ╔═══════╝ ║ ║ ║ ║ ║ ║ ╚═════════╗
 *                     ║         ║ ║ ║ ║ ║ ╚══════╗    ║
 *                     ║   ╔═════╝ ║ ║ ║ ╚═════╗  ║    ║
 * Connections for     ║   ║   ╔═══╩═║═║═══╗   ║  ║    ║
 * full-sized          ║   ║   ║   ╔═╝ ║   ║   ║  ║    ║
 * SD card             ║   ║   ║   ║   ║   ║   ║  ║    ║
 * Pin name         |  -  DO  VSS SCK VDD VSS DI CS    -  |
 * SD pin number    |  8   7   6   5   4   3   2   1   9 /
 *                  |                                  █/
 *                  |__▍___▊___█___█___█___█___█___█___/
 *
 * Note:  The SPI pins can be manually configured by using `SPI.begin(sck, miso, mosi, cs).`
 *        Alternatively, you can change the CS pin and use the other default settings by using `SD.begin(cs)`.
 *
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * | SPI Pin Name | ESP8266 | ESP32 | ESP32‑S2 | ESP32‑S3 | ESP32‑C3 | ESP32‑C6 | ESP32‑H2 |
 * +==============+=========+=======+==========+==========+==========+==========+==========+
 * | CS (SS)      | GPIO15  | GPIO5 | GPIO34   | GPIO10   | GPIO7    | GPIO18   | GPIO0    |
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * | DI (MOSI)    | GPIO13  | GPIO23| GPIO35   | GPIO11   | GPIO6    | GPIO19   | GPIO25   |
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * | DO (MISO)    | GPIO12  | GPIO19| GPIO37   | GPIO13   | GPIO5    | GPIO20   | GPIO11   |
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * | SCK (SCLK)   | GPIO14  | GPIO18| GPIO36   | GPIO12   | GPIO4    | GPIO21   | GPIO10   |
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 *
 * For more info see file README.md in this library or on URL:
 * https://github.com/espressif/arduino-esp32/tree/master/libraries/SD
 */

#include "FS.h"
#include "SD.h"
#include "SPI.h"

/*
Uncomment and set up if you want to use custom pins for the SPI communication
#define REASSIGN_PINS
int sck = -1;
int miso = -1;
int mosi = -1;
int cs = -1;
*/

#include <WiFi.h>
// #include <WiFiClient.h>
#include <WiFiMulti.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ======================== Constants ===================================

#define LED_PIN 2
#define SCL 22
#define SDA 21
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

WiFiMulti wifiMulti;
String hostName = "ESP32 Station";
JsonObject obj; // Déclaration de l'objet JSON global

// ======================== Variables ===================================

bool isConnected = false;
unsigned long lastCheckTime = 0;
const long checkInterval = 1000;

// ======================== Files Functions =============================

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char *path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char *path) {
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void testFileIO(fs::FS &fs, const char *path) {
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if (file) {
    len = file.size();
    size_t flen = len;
    start = millis();
    while (len) {
      size_t toRead = len;
      if (toRead > 512) {
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %lu ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }

  file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for (i = 0; i < 2048; i++) {
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %lu ms\n", 2048 * 512, end);
  file.close();
}
// ========================= SD Functions =============================

void handleSD() {
#ifdef REASSIGN_PINS
  SPI.begin(sck, miso, mosi, cs);
  if (!SD.begin(cs)) {
#else
  if (!SD.begin()) {
#endif
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
}


void printSDInfo() {
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}
// ========================= Wlan functions =============================

void wlanScan() {
  Serial.println("Scanning available networks...");
  int n = WiFi.scanNetworks();
  Serial.print("Number of networks found: ");
  Serial.println(n);
  for (int i = 0; i < n; ++i) {
    Serial.print(i + 1);
    Serial.print(". ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (RSSI: ");
    Serial.print(WiFi.RSSI(i));
    Serial.print(")");
    Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " [Open]" : "");
  }
}

void wlanConnect(const char *ssid, const char *password) {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");
}

// ========================= JSON functions =============================

void readJsonFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

// ========================= Other =============================

void printJsonFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}
/*
void connectToNetwork() {
    for(int i = 0; i < WiFi.scanNetworks(); i++) {
        Serial.print(i + 1);
        Serial.print(". ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (RSSI: ");
        Serial.print(WiFi.RSSI(i));
        Serial.print(")");
        Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " [Open]" : "");
        // Vérifier si le fichier existe
        if (!SD.exists("/credentials.json")) {
            Serial.println("Error: credentials.json not found");
            return;
        }

        // Ouvrir et lire le fichier
        File file = SD.open("/credentials.json");
        if (!file) {
            Serial.println("Error: could not open file");
            return;
        }

        // Parse JSON
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error) {
            Serial.print("Error: parsing JSON: ");
            Serial.println(error.c_str());
            return;
        }

        // Parcourir toutes les paires clé-valeur
        JsonObject obj = doc.as<JsonObject>();
        for (JsonPair kv : obj) {
            const char* ssid = kv.key().c_str();
            const char* password = kv.value().as<const char*>();
            
            Serial.print("SSID: ");
            Serial.println(ssid);
            
            // Vérifier si le SSID correspond à un réseau disponible
            if (WiFi.SSID(i) == ssid) {
                wlanConnect(ssid, password);
                return;
            }
        }
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Error: could not connect to any network");
            display.println("Error: could not connect to any network");
            display.display();
            return;
        }
    }
}
*/
void populateWifiMulti() {
    if (!SD.exists("/credentials.json")) {
        Serial.println("Error: credentials.json not found");
        return;
    }

    File file = SD.open("/credentials.json");
    if (!file) {
        Serial.println("Error: could not open file");
        return;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.print("Error parsing JSON: ");
        Serial.println(error.c_str());
        return;
    }

    obj = doc.as<JsonObject>();
    
    for (JsonPair kv : obj) {
        const char* ssid = kv.key().c_str();
        const char* password = kv.value().as<const char*>();
        Serial.print("Adding SSID: ");
        Serial.println(ssid);
        wifiMulti.addAP(ssid, password);
    }
}

void connectToNetwork() {
    populateWifiMulti();
    if(wifiMulti.run() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected");
        displayConnectionOnDisplay();
        displayConnectionOnSerial();
        return;
    }
    Serial.println("Error: could not connect to any network");
    display.println("Error: could not connect to any network");
    display.display();
    return;
}

void displayConnectionOnDisplay() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.println(WiFi.SSID());
    display.println("IP address");
    display.setTextSize(1);
    display.println(WiFi.localIP());
    display.println(hostName);
    display.display();
}

void displayConnectionOnSerial() {
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Host name: ");
    Serial.println(hostName);
}
// ========================= Setup =============================

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.begin(115200);
  Wire.begin(SDA, SCL);
// Initialize screen
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Try 0x3D if it doesn't work
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("Starting");
  display.display();
  handleSD();
  printSDInfo();
  // wlanScan();
  connectToNetwork();
  displayConnectionOnDisplay();
  displayConnectionOnSerial();
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_PIN, HIGH);
    // isConnected = true;
  }
}

void loop() {
    unsigned long currentTime = millis();

    if (WiFi.status() != WL_CONNECTED && currentTime - lastCheckTime >= checkInterval) {
        digitalWrite(LED_PIN, LOW);
        connectToNetwork();
        displayConnectionOnDisplay();
        displayConnectionOnSerial();
        lastCheckTime = currentTime;
    }
    else {
        digitalWrite(LED_PIN, HIGH);
    }
    yield();
    delay(1);
}
