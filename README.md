# pixiZ v3 — Universal Remote

ESP32 tabanlı, **PolyCast5** + **Flipper Zero** hibriti evrensel kumanda. IR öğrenme/gönderme, Bluetooth HID klavye, Wi-Fi tarama, ESP-NOW, şifre yöneticisi ve gelecek modüller (NFC, Sub-GHz, LoRa, SD kart) için hazır altyapı.

![pixiZ](https://img.shields.io/badge/status-active-brightgreen)
![ESP32](https://img.shields.io/badge/ESP32-DevKit%20V4-blue)
![License](https://img.shields.io/badge/license-MIT-green)

---

## Özellikler

| Mod | Özellik | Durum |
|-----|---------|-------|
| **IR Remote** | Kumandaları öğren/sakla/gönder (NEC, Sony, RC5, RC6, Samsung) | ✅ Hazır |
| **Bluetooth KB** | Bluetooth HID klavye (NimBLE) | ✅ Hazır |
| **Wi-Fi Tools** | Ağ tarama, bilgi görüntüleme | ✅ Hazır |
| **ESP-NOW** | ESP-NOW cihazlararası iletişim | 🔄 Hazır (devre dışı) |
| **Passwords** | Çevrimdışı şifre yöneticisi | 🔄 Hazır (devre dışı) |
| **PN532 NFC/RFID** | NFC kart okuma/yazma | 🔄 Planlandı |
| **CC1101 Sub-GHz** | 433/868/915 MHz | 🔄 Planlandı |
| **SX1278 LoRa** | Uzun mesafe iletişim | 🔄 Planlandı |
| **SD Kart** | Veri depolama | 🔄 Planlandı |

---

## Devre Şeması

```
  ESP32 DevKit V4
  ┌──────────────────────────────────────────┐
  │                                          │
  │  3.3V ─── VCC (ST7735, IR-RX)           │
  │  GND  ─── GND (tüm bileşenler)           │
  │                                          │
  │  GPIO 5  ─── TFT CS                      │
  │  GPIO 16 ─── TFT RST                     │
  │  GPIO 17 ─── TFT DC                      │
  │  GPIO 23 ─── TFT MOSI                    │
  │  GPIO 18 ─── TFT SCLK                    │
  │                                          │
│  GPIO 32 ─── BTN_UP  ──/~~ GND          │
│  GPIO 33 ─── BTN_DOWN──/~~ GND          │
│  GPIO 14 ─── BTN_OK  ──/~~ GND          │
│  GPIO 12 ─── BTN_MENU──/~~ GND          │
│                                          │
│  GPIO 13 ─── IR Alıcı (VS1838B) OUT     │
│  GPIO 27 ─── IR LED (gönderme)          │
  │              │                           │
  │              +─/\/\/──>|── GND           │
  │               100Ω    IR LED             │
  │                                          │
  └──────────────────────────────────────────┘
```

### ST7735 TFT (1.8" 160x128 SPI)

| TFT | ESP32 |
|-----|-------|
| CS  | GPIO 5 |
| RST | GPIO 16 |
| DC  | GPIO 17 |
| MOSI | GPIO 23 |
| SCLK | GPIO 18 |
| VCC | 3.3V |
| GND | GND |
| LED | 3.3V (veya GPIO ile PWM) |

### Butonlar (INPUT_PULLUP — butonun diğer ucu GND)

| Buton | GPIO |
|-------|------|
| UP    | GPIO 32 |
| DOWN  | GPIO 33 |
| OK    | GPIO 14 |
| MENU  | GPIO 12 |

### IR Alıcı (VS1838B / TSOP38238)

| Pin | Bağlantı |
|-----|----------|
| OUT | GPIO 13 |
| VCC | 3.3V |
| GND | GND |

### IR LED (Gönderme)

IR LED'i GPIO 27 üzerinden 100Ω seri dirençle GND'ye bağlayın. Daha güçlü sinyal için:

```
GPIO 27 ── 1kΩ ──┐
                 Baz
              [2N2222]
                 Emiter ── GND
                 Kollektör ── IR LED ── 100Ω ── 3.3V
```

---

## Gerekli Kütüphaneler (v3)

Arduino IDE → Sketch → Include Library → Manage Libraries:

| Kütüphane | Arduino'daki Adı | Yazan | Zorunlu |
|-----------|-----------------|-------|---------|
| **IRremote** | `IRremote` | ArminJo | ✅ Evet |
| **HijelHID_BLEKeyboard** | `HijelHID_BLEKeyboard` | HijelHub | ✅ Evet |
| **NimBLE-Arduino** | `NimBLE-Arduino` | h2zero | ✅ Evet |
| **Adafruit GFX** | `Adafruit GFX Library` | Adafruit | ✅ Evet |
| **Adafruit ST7735** | `Adafruit ST7735 Library` | Adafruit | ✅ Evet |

ESP32 board desteği için:
- Arduino IDE: Dosya → Tercihler → "Ek Board Yöneticisi URL'leri"ne `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` ekle
- Araçlar → Board → Board Manager → "ESP32" ara ve kur
- Board seçimi: `ESP32 Dev Module`
- **Partition Scheme**: `No OTA (Large APP)` (2MB flash için önerilir)

---

## Kullanım

### Navigasyon

| Tuş | İşlev |
|-----|-------|
| **UP / DOWN** | Menüde gezin |
| **OK** | Seç / Onayla |
| **MENU** | Geri dön |

### IR Remote

1. Ana menüden **IR Remote** seç
2. **OK** ile yeni kumanda öğren
3. Kumandayı IR alıcıya tutup bir tuşa bas
4. Sinyal alındığında **OK** ile kaydet
5. Kayıtlı kumandaları listeden seçip **OK** ile gönder

### Bluetooth Klavye

1. Ana menüden **Bluetooth KB** seç
2. Bilgisayar/telefonun Bluetooth ayarlarından `pixiZ Remote` cihazını eşle
3. Bağlandığında ekranda "CONNECTED" yazar
4. *(Yakında: Bluetooth üzerinden tuş vuruşu gönderme)*

---

## Proje Yapısı (v3)

```
pixiZ-v1/
├── pixiZ-v1/
│   └── pixiZ-v1.ino      # Tek dosya — tüm modüller içinde
├── LICENSE                 # MIT Lisansı
└── README.md               # Bu dosya
```

Tüm modüller tek `.ino` dosyası içinde `#define ENABLE_xxx 0/1` ile yönetilir.
Yeni donanım eklendiğinde flag 1 yapmak yeterli.

---

## Yol Haritası (v3)

### Yazılım
- [x] IR öğrenme/saklama/gönderme (multi-button remotes)
- [x] Bluetooth HID klavye (NimBLE, Core 3.x)
- [x] Wi-Fi ağ tarama
- [x] Modüler yapı (`ENABLE_xxx` flagları)
- [ ] Bluetooth üzerinden makro/tuş vuruşu
- [ ] ESP-NOW peer-to-peer mesajlaşma
- [ ] Çevrimdışı şifre yöneticisi
- [ ] OTA güncelleme

### Donanım (sırayla eklenecek)
- [ ] **PN532** — NFC/RFID kart okuma/yazma (I2C)
- [ ] **CC1101** — Sub-GHz 433/868/915 MHz (SPI)
- [ ] **SX1278** — LoRa uzun mesafe (SPI)
- [ ] **SD Kart modülü** — SPI (TFT ile CS paylaşımı)

---

## Lisans

MIT License — Detaylar için [LICENSE](LICENSE) dosyasına bakın.

---

## İletişim

- GitHub: [@azerenes](https://github.com/azerenes)
- Proje: [github.com/azerenes/pixiZ-v1](https://github.com/azerenes/pixiZ-v1)
- İlham: [PolyCast5](https://polycast5.com) — Multi-Tool Everything Remote
