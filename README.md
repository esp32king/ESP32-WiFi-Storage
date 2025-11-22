# ESP32-WiFi-Storage  OPEN SOURCE

ESP32 Media Storage WiFi Portal  
```
Screenshots are below.
```
A simple and powerful ESP32-based web media storage system with SD card + SPIFFS support.

## ⭐ Overview

This project turns your ESP32 into a WiFi-based media storage portal, allowing you to:

Upload, download, and delete files

Browse SD card or SPIFFS storage

Play audio/video files through the browser

Access the ESP32 as a standalone WiFi hotspot

Use a clean web UI optimized for mobile

The project automatically detects your SD card CS pin and mounts SD or SPIFFS based on the selected storage.

## 📌 Features

- ✔️ WiFi Access Point (no internet required)
- ✔️ SD + SPIFFS dual storage
- ✔️ Captive portal support
- ✔️ Auto SD-CS pin detection
- ✔️ Secure login system
- ✔️ Media preview (images, videos, audio)
- ✔️ Mobile-optimized UI
- ✔️ File manager (upload / delete / list / download)
- ✔️ Fast AJAX listing
- ✔️ Clean & modular code


## 📂 Project Structure

```
──────── PROJECT STRUCTURE ────────

project/
│
├── data/                 # HTML, CSS, JS files for SPIFFS (optional)
├── medi-storage-wifi.ino # Main ESP32 program
│
└── README.md             # You are here

───────────────────────────────────
```




🚀 How It Works
## 📡 1. Start WiFi Access Point

The ESP32 creates a hotspot:
```
SSID: WiFi Storage
Password: Open/none
```
## 🌐 2. Open the Web Portal

Visit:

http://192.168.4.1


You will see:

File upload

Current storage selection (SD or SPIFFS)

File browser with name, size, type

Delete / open / download buttons

## 💾 3. Storage Options
Storage	Description

SD	      External micro-SD card, large capacity
SPIFFS	  Internal flash storage

You
can switch storage anytime through UI.

## 🔧 Hardware Requirements

ESP32 Dev Module

Micro SD Card Module (not necessary)

FAT32 formatted SD card

Connecting wires (SPI interface)

## 📌 SD Card SPI Pins
SD Module	ESP32 Pin
```
───────────────────────────────────
Pin    | ESP32 GPIO / Notes
-------|------------------------------
CS     | Auto-detected (GPIO 4 / 5 / 13 by code)
MOSI   | GPIO 23
MISO   | GPIO 19
SCK    | GPIO 18
GND    | GND
VCC    | 3.3V
───────────────────────────────────
```

### 🔐 Admin Panel Login

```
⚙️  Username : pkmkb
🔑  Password : pkmkb1234
```


## 🛠️ How to Use
1. Install Required Libraries

All required libraries come with the Arduino ESP32 core:

FS

SPIFFS

SD

WebServer

DNSServer

## 2. Flash the code

Upload medi-storage-wifi.ino to your ESP32.

## 3. Insert SD Card

The ESP32 automatically detects the SD CS pin.

## 4. Connect to WiFi

Connect your phone or PC to:
```
SSID: WiFi Storage
Password: Open/none
```
## 5. Access the File Manager

Open:

```http://192.168.4.1```

## 📸 Screenshots

<img src="https://raw.githubusercontent.com/esp32king/ESP32-WiFi-Storage/refs/heads/main/Files/Esp-WiFi-Storage.jpg"></img>
<img src="https://raw.githubusercontent.com/esp32king/ESP32-WiFi-Storage/refs/heads/main/Files/wifi-storage-ss.jpg"></img>


## 🙌 Author / Credits

Created by Krishna (@krishna_upx61)

```
GitHub: https://github.com/esp32king
```

If you use this project, please give proper credit.
Support the creator — this project took time and effort to build!

## 📜 License

This project is open-source.
You may modify and redistribute it, but credit must be given to the original author.

## 💬 Need Help?

<br>Open an issue on GitHub or ask in discussions.</br>
────────────────────────────────
GitHub: https://github.com/esp32king ❤️  
────────────────────────────────
## More Speed For Movie streaming

If You want more speed for streaming movie so use SD/MMC module
<img src="https://raw.githubusercontent.com/esp32king/ESP32-WiFi-Storage/refs/heads/main/Files/MMC.jpg"></img>

### SD/MMC Diagram
```
microSD/MMC  | signal  | connect to (ESP32 DevKit)
-----------------------------------------------
[1] DAT2     | DAT2    | NC (not used in SPI)
[2] DAT3     | DAT3/CS | ESP32 GPIO 5   (Chip Select)  <-- or any free GPIO
[3] CMD      | CMD(MOSI)| ESP32 GPIO 23  (MOSI / SDI)
[4] VDD      | VCC     | 3.3V
[5] CLK      | CLK     | ESP32 GPIO 18  (SCK)
[6] VSS      | GND     | GND
[7] DAT0     | DAT0(MISO)| ESP32 GPIO 19 (MISO / SDO)
[8] DAT1     | DAT1    | NC (not used in SPI)

```
Thank You ❤️
