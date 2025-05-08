# 📡 ESP32 Firebase OLED IoT Project

This project is an IoT credential and message display system built using **ESP32**, **Firebase Realtime Database**, **EEPROM**, and a **0.91" OLED screen**. It allows:

- WiFi credentials to be configured via **Access Point (AP) mode**
- Real-time text messages to be fetched from Firebase
- Display of messages on OLED
- Message updates via a **web-based control panel** (with login/signup)

---

## 🔧 Features

✅ Store WiFi SSID, password, and device ID using EEPROM  
✅ Reconfigure credentials via AP mode (no serial needed)  
✅ Connect to Firebase Realtime Database securely  
✅ Display connection status, device ID, and messages on OLED  
✅ Full login/signup UI to control messages  
✅ Web interface for easy management of message updates  

---

## 📦 Hardware Requirements

- [x] ESP32 Dev Module  
- [x] 0.91" I2C OLED Display (SSD1306)  
- [x] Push button (optional for manual reset)  
- [x] Breadboard + Jumper wires  

---

## 🖥️ Software Requirements

- Arduino IDE with the following libraries:
  - `Firebase_ESP_Client`
  - `Adafruit_SSD1306`
  - `Adafruit_GFX`
  - `EEPROM`
  - `Wire`
  - `WebServer`


- Firebase project (with Realtime DB and Authentication enabled)
- Browser with access to `index.html` (control panel)

---

## 📁 Project Structure

```
ESP32_Firebase_OLED_Project/
├── EEPROM.ino         # Arduino sketch (main logic)
├── index.html         # Web control panel
├── pic1.png           # Optional image for UI branding
├── README.md          # This file
```

---

## ⚙️ EEPROM Memory Map

| Address Range | Stored Value   |
|---------------|----------------|
| 0 – 31        | SSID           |
| 32 – 63       | Password       |
| 64 – 95       | Device ID      |

---

## 🔌 Wiring (OLED 0.91")

| OLED Pin | ESP32 Pin |
|----------|-----------|
| VCC      | 3.3V      |
| GND      | GND       |
| SDA      | GPIO 21   |
| SCL      | GPIO 22   |

---

## 🌐 How to Use

1. 🔌 **Upload `EEPROM.ino`** to your ESP32
2. 🚀 On first boot (or EEPROM cleared), ESP32 enters **AP Mode**
   - Connect to `ESP32_AP`
   - Open browser at `192.168.4.1`
   - Fill in SSID, Password, and Device ID
3. 🔁 ESP32 will reboot and connect to WiFi
4. ✅ Message from `/eeprom/message` in Firebase will show on OLED
5. 🧠 To update the message:
   - Open `index.html` in browser
   - Login/signup using Firebase Authentication
   - Update message and see it appear on the OLED

---

## 🔐 Firebase Setup

1. Go to [Firebase Console](https://console.firebase.google.com/)
2. Create a project
3. Enable:
   - Realtime Database (set rules to test mode for dev)
   - Email/Password Authentication
4. Add a Web App to get:
   - `apiKey`
   - `projectId`
   - `databaseURL`
5. Replace those values in both `EEPROM.ino` and `index.html`

---

## 💬 Example Firebase Data Path

```
/eeprom/message: "Hello from Firebase!"
```

---

## 🛠️ Credits

Developed for **STTHK3113 IoT Lab Assignment 2**  
Author: [Goh Hong Xuan]  
University: Universiti Utara Malaysia (UUM)  
