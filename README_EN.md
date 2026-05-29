<p align="center">
  <img src="https://img.shields.io/badge/pixiZ-v7.0-FF6B35?style=for-the-badge&logo=espressif&logoColor=white"/>
  <img src="https://img.shields.io/badge/ESP32-DevKit%20V4-1FA2F2?style=for-the-badge&logo=espressif&logoColor=white"/>
  <img src="https://img.shields.io/badge/License-MIT-00BFA5?style=for-the-badge"/>
</p>

<h1 align="center">⚡ pixiZ</h1>
<h3 align="center">Universal Multi-Tool Remote — ESP32-Powered</h3>

<p align="center">
  <i>A marriage of PolyCast5 · Flipper Zero · Bruce · ESP32 Marauder</i><br>
  <b>IR Remote · BT HID Keyboard · Wi-Fi Pentest · BLE Spam · BadUSB · Sub-GHz · NFC · LoRa</b>
</p>

<br>

---

## 🌐 Language

> **Türkçe için:** [`README.md`](README.md)

---

## 📋 Overview

**pixiZ** turns an ESP32 DevKit V4 into a versatile multi-tool remote. With the included hardware (ST7735 TFT, 4 buttons, IR receiver/LED) it already supports IR learning/transmission, Bluetooth HID keyboard, and a wide range of Wi-Fi/BLE attack tools. Add-on modules (CC1101, PN532, SX1278, etc.) unlock Sub-GHz, NFC, LoRa, and more.

> **65+ features** across 11 categories — all in a single `.ino` file.

---

## ✨ Feature Matrix

### 📡 Wi-Fi (Built-in — No Extra Hardware)

| # | Feature | Status | Description |
|---|---------|--------|-------------|
| 1 | **Network Scanner** | 🟢 Ready | Scan surrounding APs, display SSID, RSSI, channel, encryption |
| 2 | **Beacon Spam (Random)** | 🟢 Ready | Flood with 100+ fake APs — chaoify Wi-Fi lists |
| 3 | **Beacon Spam (From List)** | 🟢 Ready | Predefined realistic SSID list |
| 4 | **AP Clone Spam** | 🟢 Ready | Clone real APs to confuse targets |
| 5 | **Deauth Flood** | 🟢 Ready | Disconnect clients from any network |
| 6 | **Deauth Detector** | 🟢 Ready | Detect ongoing deauth attacks nearby |
| 7 | **PMKID Capture** | 🟢 Ready | Capture WPA2/WPA3 handshakes for offline cracking (Hashcat) |
| 8 | **Probe Request Sniffer** | 🟢 Ready | See what networks devices are looking for (privacy audit) |
| 9 | **Evil Twin** | 🟢 Ready | Clone AP SSID + BSSID, deauth original, hijack clients |
| 10 | **Evil Portal** | 🟢 Ready | 4 templates: WiFi Update, Facebook, Instagram, Twitter login pages |
| 11 | **Wardriving** | 🟢 Ready | Log APs (SSID/BSSID/RSSI/Ch/Enc) to NVS, view stats |
| 12 | **RAW Sniffer** | 🟢 Ready | Live 802.11 packet monitor (Beacon/Deauth/Probe/Data) |
| 13 | **Port Scanner** | 🟢 Ready | TCP connect scan of 19 common ports on target IP |
| 14 | **ARP Spoofing** | 🟢 Ready | Poison ARP tables, redirect traffic for MITM |
| 15 | **Beacon Sniff** | 🟢 Ready | Deep AP discovery with encryption, channel, signal % |
| 16 | **WebUI** | 🟢 Ready | Web dashboard: toggle Beacon/Deauth, scan, restart |

### 📶 BLE (Built-in)

| # | Feature | Status | Description |
|---|---------|--------|-------------|
| 1 | **BLE Scanner** | 🟢 Ready | Scan for all BLE devices, show address and RSSI |
| 2 | **BLE Spam — iOS (AirDrop)** | 🟢 Ready | Trigger AirDrop popups on nearby iPhones |
| 3 | **BLE Spam — Samsung** | 🟢 Ready | Trigger Quick Share popups on Samsung devices |
| 4 | **BLE Spam — Windows** | 🟢 Ready | Trigger Swift Pair popups on Windows |
| 5 | **BLE Spam — Android** | 🟢 Ready | Trigger Fast Pair popups on Android |
| 6 | **BLE Spam — All** | 🟢 Ready | All platforms simultaneously |
| 7 | **AirTag Sniff** | 🟢 Ready | Detect nearby Apple FindMy devices (0x4C00) |
| 8 | **BT HID Keyboard** | 🟢 Ready | Connect as Bluetooth keyboard to any device |

### 📟 IR (VS1838B + IR LED)

| # | Feature | Status | Description |
|---|---------|--------|-------------|
| 1 | **Learn + Save** | 🟢 Ready | Learn any 38kHz IR signal, store in NVS |
| 2 | **Send (Multi-Button)** | 🟢 Ready | Up to 16 buttons per remote, replay learned signals |
| 3 | **TV-B-Gone** | 🟢 Ready | 14+ power-off codes × 5 rounds — turn off TVs |
| 4 | **RAW Capture** | 🟢 Ready | Capture unknown protocols: Proto/Addr/Cmd display |
| 5 | **Custom Protocols** | 🟢 Ready | NEC, NECext, SIRC, Samsung32, RC5, RC6 |

### ⌨️ BadUSB / Ducky Script

| # | Feature | Status | Description |
|---|---------|--------|-------------|
| 1 | **BT Keyboard Connection** | 🟢 Ready | Pair via Bluetooth as HID keyboard |
| 2 | **Ducky Script Engine** | 🟢 Ready | DELAY, STRING, ENTER, GUI, CTRL-ALT-DEL, ALT-F4, etc. |
| 3 | **NVS Payload Storage** | 🟢 Ready | Save/load 10 payloads up to 256 bytes each |

### 🛡️ Security / Tools

| # | Feature | Status | Description |
|---|---------|--------|-------------|
| 1 | **Password Manager** | 🟢 Ready | CRUD for service/username/password, BT autotype |
| 2 | **QR Code Generator** | 🟢 Ready | Generate WiFi QR or custom text, display on TFT |
| 3 | **ESP-NOW Messaging** | 🟢 Ready | Send/receive messages between two ESP32 devices |

### 🔌 External Modules (Hardware Required)

| Module | Interface | Features | Status |
|--------|-----------|----------|--------|
| **CC1101** (Sub-GHz) | SPI | Scan, Replay, Jammer, Spectrum | 🔴 Module needed |
| **PN532** (NFC/RFID) | I²C | Read, Clone, Write, Emulate, Amiibo | 🔴 Module needed |
| **SX1278** (LoRa) | SPI | Long-range messaging, Remote control | 🔴 Module needed |
| **SD Card** | SPI | File browser, IR backups, wardriving logs | 🔴 Module needed |
| **NRF24L01+** | SPI | Jammer, Spectrum, Mousejack | 🔴 Module needed |
| **TEA5767** (FM) | I²C | Broadcast, Spectrum, RDS hijack | 🔴 Module needed |

---

## 🔧 Hardware

### Current Pinout

| Component | ESP32 Pin |
|-----------|-----------|
| ST7735 TFT | CS=5, RST=16, DC=17, MOSI=23, MISO=19, SCK=18 |
| Button UP | GPIO 32 (INPUT_PULLUP) |
| Button DOWN | GPIO 33 (INPUT_PULLUP) |
| Button OK | GPIO 14 (INPUT_PULLUP) |
| Button MENU | GPIO 12 (INPUT_PULLUP) |
| VS1838B IR Receiver | GPIO 13 |
| IR LED (Transmitter) | GPIO 27 (100Ω resistor) |

For full wiring diagrams of all modules → **[docs/WIRING.md](docs/WIRING.md)**

---

## 📦 Libraries (Arduino IDE)

Sketch → Include Library → Manage Libraries:

| Library | Arduino Name | Required |
|---------|-------------|----------|
| **IRremote** | `IRremote` by ArminJo | ✅ Yes |
| **BLEKeyboard** | `HijelHID_BLEKeyboard` by HijelHub | ✅ Yes |
| **NimBLE-Arduino** | `NimBLE-Arduino` by h2zero | ✅ Yes |
| **Adafruit GFX** | `Adafruit GFX Library` | ✅ Yes |
| **Adafruit ST7735** | `Adafruit ST7735 Library` | ✅ Yes |
| **QRCode** | `QRCode` by ricmoo | ✅ Yes |
| **PN532** | `Adafruit PN532` | 🔴 For NFC |
| **CC1101** | `ELECHOUSE_CC1101` | 🔴 For Sub-GHz |
| **LoRa** | `LoRa` by Sandeep Mistry | 🔴 For LoRa |
| **SD Card** | `SD` (Built-in) | 🔴 For SD |
| **NRF24** | `RF24` by TMRh20 | 🔴 For NRF24 |

**Board Setup:**
- Add URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
- Board: `ESP32 Dev Module`
- Partition: `No OTA (Large APP)`

---

## 📁 Project Structure

```
pixiZ-v1/
├── pixiZ-v1/
│   └── pixiZ-v1.ino        # Single-file firmware (~2800 lines)
├── docs/
│   └── WIRING.md            # Hardware connection diagrams
├── README.md                # Turkish documentation
├── README_EN.md             # English documentation (this file)
└── LICENSE                  # MIT License
```

All modules controlled via `#define ENABLE_xxx 0/1` — flip to activate.

---

## 🛤️ Roadmap

| Phase | Version | Status |
|-------|---------|--------|
| **🥇 Phase 1** — Wi-Fi/BLE/IR attacks | v4 | ✅ Complete |
| **🥈 Phase 2** — Evil Twin, Port Scanner, QR, Passwords | v5-v6 | ✅ Complete |
| **🥉 Phase 3** — Wardriving, WebUI, Portal presets | v7 | ✅ Complete |
| **🔌 Phase 4** — Hardware modules | v8+ | 📅 When modules arrive |

---

## 📜 License

MIT License — see [LICENSE](LICENSE).

---

<p align="center">
  <a href="https://github.com/azerenes/pixiZ-v1">
    <img src="https://img.shields.io/badge/github-azerenes/pixiZ--v1-181717?style=for-the-badge&logo=github"/>
  </a>
</p>
