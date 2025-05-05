#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <EEPROM.h>
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
#define EEPROM_SIZE 100
String ssid, password, username;

#define API_KEY "YOUR_API_KEY"
#define DATABASE_URL "YOUR_DATABASE_URL"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

// === EEPROM Functions ===
void writeToEEPROM(int start, String data) {
  for (int i = 0; i < data.length(); i++) {
    EEPROM.write(start + i, data[i]);
  }
  EEPROM.write(start + data.length(), '\0');
  EEPROM.commit();
}

String readFromEEPROM(int start, int length) {
  char data[length + 1];
  for (int i = 0; i < length; i++) {
    data[i] = EEPROM.read(start + i);
  }
  data[length] = '\0';
  return String(data);
}

void clearEEPROM() {
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  Serial.println("✅ EEPROM cleared!");
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

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
  writeToEEPROM(0, "YOUR_SSID");
  writeToEEPROM(32, "YOUR_PASSWORD");
  writeToEEPROM(64, "YOUR_USERNAME");

  // Read from EEPROM
  ssid = readFromEEPROM(0, 32);
  password = readFromEEPROM(32, 32);
  username = readFromEEPROM(64, 32);

  Serial.println("Stored SSID: " + ssid);
  Serial.println("Stored Password: " + password);
  Serial.println("Stored Username: " + username);

  // === OLED Init ===
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED failed");
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Connecting WiFi...");
  display.display();

  // === WiFi Connect ===
  WiFi.begin(ssid.c_str(), password.c_str());
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi OK");
    display.println("IP: " + WiFi.localIP().toString());
    display.display();
  } else {
    Serial.println("\n❌ WiFi Failed");
    display.println("WiFi Failed");
    display.display();
    return;
  }

  // === Firebase Setup ===
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  //delay(2000); // wait for connection

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("✅ Firebase signUp OK");
    signupOK = true;
  } else {
    Serial.printf("❌ signUp failed: %s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}


void loop() {
  if (Firebase.ready() && signupOK) {
    if (Firebase.RTDB.getString(&fbdo, "/eeprom/message")) {
      String msg = fbdo.stringData();
      Serial.println("📥 Firebase Message: " + msg);

      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Hello, " + username);
      display.println("Message:");
      display.println(msg);
      display.display();
    } else {
      Serial.println("❌ Firebase read failed: " + fbdo.errorReason());
    }
  }

  delay(5000); // Update every 5s
}
