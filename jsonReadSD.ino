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
#include <WiFiMulti.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WebServer.h>
// Include generated files
#include "index_html.h"
#include "wifi_manager.h"

// Variables globales
String scanResults = "";

// Déclaration de la fonction stopAccessPoint
void stopAccessPoint() {
    WiFi.softAPdisconnect(true);
    Serial.println("Access Point stopped");
}

// Déclaration des tableaux de caractères pour les fichiers HTML
extern const char wifi_manager_html[];

extern const char index_html[];

// ======================== Constants ===================================

#define LED_PIN 2
#define SCL 22
#define SDA 21
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String hostName = "ESP32_Station";
JsonObject obj;
// ======================== Variables ===================================

bool isConnected = false;
unsigned long lastCheckTime = 0;
const long checkInterval = 2000;
const long scanInterval = 30000;

// ======================== Instances ===================================

WiFiMulti wifiMulti;
WebServer server(80);

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
    Serial.println("Card Mount Failed");
    return;
  }
#else
  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }
#endif
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
  Serial.println();
  Serial.println("SD Card mounted");
  printSDInfo();
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
  
  // Réinitialiser scanResults
  scanResults = "";
  
  for (int i = 0; i < n; ++i) {
    Serial.print(i + 1);
    Serial.print(". ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (RSSI: ");
    Serial.print(WiFi.RSSI(i));
    Serial.print(")");
    Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " [Open]" : "");
    
    // Pour l'interface web // TODO
    scanResults += "<li>";
    scanResults += WiFi.SSID(i);
    scanResults += " (RSSI: ";
    scanResults += WiFi.RSSI(i);
    scanResults += " dBm)";
    if (WiFi.encryptionType(i) != WIFI_AUTH_OPEN) {
      scanResults += " [Sécurisé]";
    }
    scanResults += "</li>";
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

void startAccessPoint() {
  Serial.println("Starting Access Point...");
  WiFi.softAP("ESP32-AccessPoint", "12345678");
  Serial.println("Access Point started");
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

StaticJsonDocument<2048> credentialsDoc;

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
    DeserializationError error = deserializeJson(credentialsDoc, file);
    file.close();

    if (error) {
        Serial.print("Error parsing JSON: ");
        Serial.println(error.c_str());
        return;
    }

    obj = credentialsDoc.as<JsonObject>();
    Serial.println("JSON parsed");

    for (JsonPair kv : obj) {
        const char* ssid = kv.key().c_str();
        const char* password = kv.value().as<const char*>();
        Serial.print("Adding SSID: ");
        Serial.println(ssid);
        wifiMulti.addAP(ssid, password);
    }
}

void connectToNetwork() {
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

void connectToSpecificNetwork(const char* ssid) {
    if (!obj.containsKey(ssid)) {
        Serial.println("Error: SSID not in credentials");
        return;
    }

    const char* password = obj[ssid].as<const char*>();
    Serial.print("Attempting to connect to: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    unsigned long startAttempt = millis();
    const unsigned long timeout = 10000; // 10 second timeout

    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < timeout) {
        delay(100);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("Successfully connected to target network");
        displayConnectionOnDisplay();
        displayConnectionOnSerial();
    } else {
        Serial.println("");
        Serial.println("Failed to connect to target network, falling back to WiFiMulti");
        connectToNetwork();
    }
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

void initDisplay() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.println("Starting");
    display.display();
}

/*
void startWebServer() { // TODO: remove
  WiFiServer server(80);
  server.begin();
}
*/


void displayWebPage() {
    // Rediriger vers la nouvelle interface
    server.sendHeader("Location", "/wifi_manager.html");
    server.send(302, "text/plain", "Redirecting to WiFi Manager...");
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
  initDisplay();
  handleSD();
  printSDInfo();
  populateWifiMulti();
  wlanScan();
  connectToNetwork();
  displayConnectionOnDisplay();
  displayConnectionOnSerial();
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_PIN, HIGH);
    // isConnected = true;
  }
  else {
    // isConnected = false;
    startAccessPoint();
  }
  server.on("/", HTTP_GET, displayWebPage);
  server.on("/update", HTTP_POST, []() {
    String newSSID = server.arg("ssid");
    String newPassword = server.arg("password");

    if (newSSID.length() == 0) {
      server.send(400, "text/plain", "Error: SSID cannot be empty");
      return;
    }

    Serial.println("Received new credentials:");
    Serial.print("  SSID: ");
    Serial.println(newSSID);
    Serial.println("  Password: [hidden]");

    // Read existing credentials
    File file = SD.open("/credentials.json");
    if (!file) {
      server.send(500, "text/plain", "Error: Failed to open credentials file");
      return;
    }

    DeserializationError error = deserializeJson(credentialsDoc, file);
    file.close();

    if (error) {
      server.send(500, "text/plain", "Error: Failed to parse credentials file");
      return;
    }

    // Add new credentials
    obj = credentialsDoc.as<JsonObject>();
    obj[newSSID] = newPassword;

    // Write back to file
    SD.remove("/credentials.json");
    file = SD.open("/credentials.json", FILE_WRITE);
    if (!file) {
      server.send(500, "text/plain", "Error: Failed to write credentials file");
      return;
    }

    serializeJson(credentialsDoc, file);
    file.close();

    // Add to WiFiMulti
    wifiMulti.addAP(newSSID.c_str(), newPassword.c_str());

    Serial.println("Credentials saved successfully");
    server.send(200, "text/html", "<html><body><h1>Success!</h1><p>Credentials added.</p><a href='/'>Back</a></body></html>");
  });
  server.on("/delete", HTTP_POST, []() {
    String ssid = server.arg("ssid");

    if (ssid.length() == 0) {
      server.send(400, "text/plain", "Error: SSID cannot be empty");
      return;
    }

    Serial.println("Received delete request for SSID: " + ssid);

    // Remove from WiFiMulti
    // wifiMulti.removeAP(ssid.c_str()); // TODO: remove
    // TODO : wifiMulti.APlistClean();
    // TODO repopulate wifiMulti with credentials from SD AFTER we remove the AP in credentials
    

    // Test credentials file
    File file = SD.open("/credentials.json");
    if (!file) {
      server.send(500, "text/plain", "Error: Failed to open credentials file");
      return;
    }

    DeserializationError error = deserializeJson(credentialsDoc, file);
    file.close();

    if (error) {
      server.send(500, "text/plain", "Error: Failed to parse credentials file");
      return;
    }

    // Remove from JSON object
    obj = credentialsDoc.as<JsonObject>();
    obj.remove(ssid);

    // Write back to file
    SD.remove("/credentials.json");
    file = SD.open("/credentials.json", FILE_WRITE);
    if (!file) {
      server.send(500, "text/plain", "Error: Failed to write credentials file");
      return;
    }

    serializeJson(credentialsDoc, file);
    file.close();

    Serial.println("Credentials deleted successfully");
    server.send(200, "text/html", "<html><body><h1>Success!</h1><p>Credentials deleted.</p><a href='/'>Back</a></body></html>");
  });
  server.on("/restart", HTTP_GET, []() {
    Serial.println("Received restart request");
    ESP.restart();
  });
  server.on("/scan", HTTP_GET, []() {
    Serial.println("Received scan request");
    wlanScan();
    server.send(200, "text/html", "<html><body><h1>Scan completed</h1><p>Scan results:</p><ul>" + scanResults + "</ul><a href='/'>Back</a></body></html>");
  });
  server.on("/connect", HTTP_GET, []() {
    Serial.println("Received connect request");
    wlanScan();
    WiFi.disconnect();
    delay(100);
    connectToNetwork();
    displayConnectionOnDisplay();
    displayConnectionOnSerial();
    server.send(200, "text/html", "<html><body><h1>Connected</h1><p>Connected to network.</p><a href='/'>Back</a></body></html>");
  });
  server.on("/disconnect", HTTP_GET, []() {
    Serial.println("Received disconnect request");
    WiFi.disconnect();
    server.send(200, "text/html", "<html><body><h1>Disconnected</h1><p>Disconnected from network.</p><a href='/'>Back</a></body></html>");
  });
  server.on("/startAP", HTTP_GET, []() {
    Serial.println("Received start AP request");
    startAccessPoint();
    server.send(200, "text/html", "<html><body><h1>AP started</h1><p>AP started.</p><a href='/'>Back</a></body></html>");
  });
  server.on("/stopAP", HTTP_GET, []() {
    Serial.println("Received stop AP request");
    stopAccessPoint();
    server.send(200, "text/html", "<html><body><h1>AP stopped</h1><p>AP stopped.</p><a href='/'>Back</a></body></html>");
  });
  // Servir la page wifi_manager.html
  server.on("/wifi_manager.html", HTTP_GET, []() {
    server.send(200, "text/html", String(wifi_manager_html));
  });
  server.on("/index.html", HTTP_GET, []() {
    server.send(200, "text/html", String(index_html));
  });
  
  // Servir la page d'accueil
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", "<html><body>"
                                 "<h1>ESP32 WiFi Manager</h1>"
                                 "<p><a href='/wifi_manager.html'>Accéder au gestionnaire WiFi</a></p>"
                                 "<p><a href='/index.html'>Page d'accueil</a></p>"
                                 "</body></html>");
  });
  server.begin();
}

void loop() {
    unsigned long currentTime = millis();
    Serial.println(WiFi.status()); //test
    if (WiFi.status() != WL_CONNECTED) {
        digitalWrite(LED_PIN, LOW);
        connectToNetwork();
        displayConnectionOnDisplay();
        displayConnectionOnSerial();
        lastCheckTime = currentTime;
    }
    if (WiFi.status() == WL_CONNECTED) {
        digitalWrite(LED_PIN, HIGH);
    }
    else {
        digitalWrite(LED_PIN, LOW);
    }
    if (currentTime - lastCheckTime >= checkInterval) {
        Serial.print("Current RSSI: ");
        int currentRSSI = WiFi.RSSI();
        Serial.println(currentRSSI);

        // Perform scan ONCE and store the result
        WiFi.scanDelete(); // Clear previous scan results
        int numNetworks = WiFi.scanNetworks();

        if (numNetworks > 0) {
            int bestRSSI = currentRSSI;
            int bestIndex = -1;

            // Iterate through cached results
            for (int i = 0; i < numNetworks; i++) {
                String scannedSSID = WiFi.SSID(i);
                int scannedRSSI = WiFi.RSSI(i);

                // Check if this network is in our credentials and is better
                if (scannedRSSI > bestRSSI && obj.containsKey(scannedSSID.c_str())) {
                    bestRSSI = scannedRSSI;
                    bestIndex = i;
                }
            }

            // Only switch if we found another significantly better network (10 dB threshold)
            String currentSSID = WiFi.SSID();
            if (bestIndex != -1 && bestRSSI > currentRSSI + 10 && currentSSID != WiFi.SSID(bestIndex)) {
                String betterSSID = WiFi.SSID(bestIndex);
                Serial.println("Found significantly better network");
                Serial.print("SSID: ");
                Serial.println(betterSSID);
                Serial.print("RSSI: ");
                Serial.println(bestRSSI);
                Serial.print("Improvement: +");
                Serial.print(bestRSSI - currentRSSI);
                Serial.println(" dB");

                display.clearDisplay();
                display.setCursor(0, 0);
                display.setTextSize(2);
                display.println("Switching to:");
                display.println(betterSSID);
                display.display();

                WiFi.disconnect();
                digitalWrite(LED_PIN, LOW);
                delay(100); // Brief delay to ensure clean disconnect
                connectToSpecificNetwork(betterSSID.c_str());
            }
        }

        lastCheckTime = currentTime;
    }
    server.handleClient();
    yield();
    delay(10);
}