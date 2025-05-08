#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <EEPROM.h>
#include <Wire.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"


// === OLED Setup ===
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// === EEPROM Setup ===
WebServer server(80);
String ssid, pass, devid, content;
bool apMode = false;
bool scanComplete = false;
String scannedNetworks = "<p>Scanning...</p>";

// Firebase objects
#define API_KEY "YOUR_API_KEY"
#define DATABASE_URL "YOUR_DATABASE_URL"
String firebasePath = "YOUR_FIREBASE_PATH";
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// === Timer ===
unsigned long lastUpdate = 0;
unsigned long interval = 2000;


void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  pinMode(0, INPUT_PULLUP);
  // EEPROM.begin(EEPROM_SIZE);

  // Check for "clear" input
  Serial.println("Type 'clear' in Serial Monitor within 5 seconds to reset EEPROM...");
  unsigned long startTime = millis();
  while (millis() - startTime < 5000) {
    if (Serial.available()) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      if (input == "clear") {
        clearEEPROM();
        Serial.println("Please re-upload the sketch to write new credentials.");
        while (1);
      }
    }
  }

  //Write credentials (optional: comment out after first upload)
  // writeToEEPROM(0, "WIFI SSID");
  // writeToEEPROM(32, "WIFI PWD");
  // writeToEEPROM(64, "USERNAME");

  // Read from EEPROM
  // ssid = readFromEEPROM(0, 32);
  // password = readFromEEPROM(32, 32);
  // username = readFromEEPROM(64, 32);

  // Serial.println("Stored SSID: " + ssid);
  // Serial.println("Stored Password: " + password);
  // Serial.println("Stored Username: " + username);

  // === OLED Init ===
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED failed");
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Starting...");
  display.display();

  readEEPROM();

  // === Force to enter AP Mode by pressing BOOT button ===
  Serial.println("Waiting for 5 seconds!");
  delay(5000);

  if (ssid.length() == 0 || digitalRead(0) == 0) {
    Serial.println("Entering AP mode...");
    enterAPMode();
    return;
  }

  connectWiFi();
  initFirebase();
}

void loop() {
  if (apMode) {
    server.handleClient();
    if (!scanComplete) {
      int n = WiFi.scanComplete();
      if (n == -2) {
        WiFi.scanNetworks(true);
      } else if (n > 0) {
        scannedNetworks = "<table border='1'><tr><th>SSID</th><th>Signal</th></tr>";
        for (int i = 0; i < n; i++) {
          scannedNetworks += "<tr><td>" + WiFi.SSID(i) + "</td><td>" + String(WiFi.RSSI(i)) + " dBm</td></tr>";
        }
        scannedNetworks += "</table>";
        scanComplete = true;
        WiFi.scanDelete();
      }
    }
    return;
  }

  // === Refresh token if needed ===
  if (Firebase.ready() && Firebase.isTokenExpired()) {
    Serial.println("Token expired. Refreshing...");
    Firebase.begin(&config, &auth);
    Serial.println("✅ Token refreshed successfully.");
  }

  // === Skip if Firebase is not ready ===
  if (!Firebase.ready()) {
    Serial.println("Skipping loop: Firebase token ❌ not ready");
    delay(1000);
    return;
  }

  // === Firebase Read ===
  if (millis() - lastUpdate >= interval) {
    lastUpdate = millis();
    if (Firebase.RTDB.getString(&fbdo, firebasePath)) {
      String msg = fbdo.stringData();
      Serial.println("From Firebase: " + msg);

      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Device ID: " + devid);
      display.println("Message: ");
      display.print(msg);
      display.display();
    } else {
      String err = fbdo.errorReason();
      Serial.println("FB Error: " + err);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("FB Err:");
      display.println(err);
      display.display();
    }
  }
  delay(5000);
}

// === Read EEPROM ===
void readEEPROM() {
  EEPROM.begin(512);
  Serial.println("Reading From EEPROM...");
  ssid = pass = devid = "";

  // Read SSID (0–19)
  for (int i = 0; i < 20; i++) {
    char ch = EEPROM.read(i);
    if (isPrintable(ch)) ssid += ch;
  }

  // Read Password (20–39)
  for (int i = 20; i < 40; i++) {
    char ch = EEPROM.read(i);
    if (isPrintable(ch)) pass += ch;
  }

  // Read Device ID (40–59)
  for (int i = 40; i < 60; i++) {
    char ch = EEPROM.read(i);
    if (isPrintable(ch)) devid += ch;
  }

  EEPROM.end();

  // Print to Serial
  Serial.println("Stored SSID: " + ssid);
  Serial.println("Stored Password: " + pass);
  Serial.println("Stored Device ID: " + devid);
}

// === Write EEPROM ===
void writeEEPROM(String a, String b, String c) {
  clearEEPROM();  // Always clear first to avoid leftover characters

  EEPROM.begin(512);
  Serial.println("Writing to EEPROM...");
  for (int i = 0; i < 20; i++) EEPROM.write(i, (i < a.length()) ? a[i] : 0);
  for (int i = 0; i < 20; i++) EEPROM.write(20 + i, (i < b.length()) ? b[i] : 0);
  for (int i = 0; i < 20; i++) EEPROM.write(40 + i, (i < c.length()) ? c[i] : 0);
  EEPROM.commit();
  EEPROM.end();
  Serial.println("Write Successful");
}

// === Clear EEPROM contents to default (0) ===
void clearEEPROM() {
  EEPROM.begin(512);
  Serial.println("✅ Clearing EEPROM (first 60 bytes only)...");
  for (int i = 0; i < 60; i++) EEPROM.write(i, 0);
  EEPROM.commit();
  EEPROM.end();
}

// === Enter AP Mode ===
void enterAPMode() {
  apMode = true;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESP32_AP", "");
  WiFi.scanNetworks(true);
  Serial.println("AP Mode: " + WiFi.softAPIP().toString());
  digitalWrite(2, HIGH);

  // === OLED Display ===
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("AP Mode");
  display.println("Connect to: ");
  display.print(devid);
  display.display();
  delay(2000);
  
  // === Web Server ===
  server.on("/", []() {
    content = R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          body {
            font-family: 'Segoe UI', sans-serif;
            background-color: #f5f7fa;
            text-align: center;
            margin: 0;
            padding: 20px;
          }

          .container {
            background-color: #ffffff;
            max-width: 400px;
            margin: auto;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
          }

          h2 {
            margin-bottom: 10px;
            color: #333;
          }

          label {
            display: block;
            text-align: left;
            margin: 10px 0 5px;
            color: #444;
          }

          input[type="text"] {
            width: 100%;
            padding: 10px;
            border: 1px solid #ccc;
            border-radius: 5px;
            box-sizing: border-box;
          }

          .button-group {
            margin-top: 15px;
            display: flex;
            justify-content: space-between;
          }

          .button {
            background-color: #007BFF;
            color: white;
            border: none;
            padding: 10px 20px;
            border-radius: 5px;
            font-size: 16px;
            cursor: pointer;
            width: 48%;
          }

          .button:hover {
            background-color: #0056b3;
          }

          .button-clear {
            background-color: #dc3545;
          }

          .button-clear:hover {
            background-color: #a71d2a;
          }

          table {
            margin-top: 20px;
            width: 100%;
            border-collapse: collapse;
          }

          th, td {
            border: 1px solid #ccc;
            padding: 8px;
            text-align: left;
          }

          th {
            background-color: #f0f0f0;
          }

          .section-title {
            margin-top: 30px;
            font-weight: bold;
            color: #444;
          }
        </style>
      </head>
      <body>
        <div class="container">
          <h2>WiFi Configuration</h2>
          <form method='get' action='setting'>
            <label>WiFi SSID:</label>
            <input type='text' name='ssid' value=')rawliteral" + ssid + R"rawliteral(' required>
            <label>WiFi Password:</label>
            <input type='text' name='password' value=')rawliteral" + pass + R"rawliteral('>
            <label>Device ID:</label>
            <input type='text' name='devid' value=')rawliteral" + devid + R"rawliteral(' required>
            <div class="button-group">
              <input class='button' type='submit' value='Submit'>
              <input class='button button-clear' type='button' value='Clear' onclick="window.location.href='/clear'">      
            </div>
          </form>

          <div class="section-title">Networks:</div>
          )rawliteral"
              + scannedNetworks + R"rawliteral(
        </div>
      </body>
      </html>
    )rawliteral"; 

    server.send(200, "text/html", content);
  });

  // === Handle New Settings to EEPROM ===
  server.on("/setting", []() {
    String ssid = server.arg("ssid");
    String pass = server.arg("password");
    String devid = server.arg("devid");
    String username = devid;

    // writeToEEPROM(0, ssid);
    // writeToEEPROM(32, pass);
    // writeToEEPROM(64, devid);
    writeEEPROM(ssid, pass, devid);
    server.send(200, "text/html", R"rawliteral(
      <html>
        <head>
          <meta name='viewport' content='width=device-width, initial-scale=1'>
          <style>
            body { font-family: Arial; background:#f8f8f8; text-align:center; padding:40px; }
            .msg { background:#ffffff; padding:20px; border-radius:10px; box-shadow:0 0 10px rgba(0,0,0,0.1); display:inline-block; }
            h2 { color:#28a745; }
            p { color:#333; }
          </style>
        </head>
        <body>
          <div class='msg'>
            <h2>Settings Saved!</h2>
            <p>ESP32 will now restart and connect with new credentials.</p>
          </div>
        </body>
      </html>
    )rawliteral");

    delay(2000);
    digitalWrite(2, LOW);
    ESP.restart();
  });

  // === Clear EEPROM ===
  server.on("/clear", []() {
    clearEEPROM();
    server.send(200, "text/html", R"rawliteral(
      <html>
        <head>
          <meta name='viewport' content='width=device-width, initial-scale=1'>
          <style>
            body { font-family: Arial; background:#fff5f5; text-align:center; padding:40px; }
            .msg { background:#ffffff; padding:20px; border-radius:10px; box-shadow:0 0 10px rgba(255,0,0,0.1); display:inline-block; }
            h2 { color:#dc3545; }
            p { color:#444; }
          </style>
        </head>
        <body>
          <div class='msg'>
            <h2>EEPROM Cleared!</h2>
            <p>ESP32 will restart now.<br>Please reconnect to configure WiFi again.</p>
          </div>
        </body>
      </html>
    )rawliteral");

    delay(2000);
    digitalWrite(2, LOW);
    ESP.restart();
  });
  server.begin();
}

// === Connect to WiFi ===
void connectWiFi() {
  WiFi.softAPdisconnect(true);  // Make sure AP mode is off
  WiFi.disconnect(true);        // Clear any old connection
  WiFi.mode(WIFI_STA);          // Station mode (client)
  WiFi.persistent(true);        // Store credentials in flash
  WiFi.setAutoReconnect(true);  // Enable auto-reconnect

  WiFi.begin(ssid.c_str(), pass.c_str());

  Serial.print("Connecting to WiFi: " + ssid + "...");

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 30) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi Connected!");
    display.println("IP: " + WiFi.localIP().toString());
    display.display();
  } else {
    Serial.println("\n❌ WiFi Failed. Entering AP mode...");
    display.println("WiFi Failed! Enter AP!");
    display.display();
    enterAPMode();
  }
}

// === Firebase Setup ===
void initFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  delay(3000); // wait for connection

  auth.user.email = "abc@gmail.com";
  auth.user.password = "pwd";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  while (auth.token.uid == "") {
    delay(3000);
    Serial.print(".");
  }

  Serial.println("✅ Firebase signUp OK");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Firebase OK >-<");
  display.display();
  delay(3000);
}

// // === EEPROM Functions ===
// void writeToEEPROM(int start, String data) {
//   for (int i = 0; i < data.length(); i++) {
//     EEPROM.write(start + i, data[i]);
//   }
//   EEPROM.write(start + data.length(), '\0');
//   EEPROM.commit();
// }

// String readFromEEPROM(int start, int length) {
//   char data[length + 1];
//   for (int i = 0; i < length; i++) {
//     data[i] = EEPROM.read(start + i);
//   }
//   data[length] = '\0';
//   return String(data);
// }