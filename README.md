# pixiZ Universal Remote

ESP32 tabanlı, **PolyCast5** benzeri çok fonksiyonlu evrensel kumanda. IR öğrenme/gönderme, Bluetooth HID klavye ve Wi-Fi kontrollerini tek cihazda birleştirir.

![pixiZ](https://img.shields.io/badge/status-active-brightgreen)
![ESP32](https://img.shields.io/badge/ESP32-DevKit%20V4-blue)
![License](https://img.shields.io/badge/license-MIT-green)

---

## Özellikler

| Mod | Özellik | Durum |
|-----|---------|-------|
| **IR Remote** | Kumandaları öğren ve sakla (NEC, Sony, RC5, RC6, Samsung) | ✅ Hazır |
| **IR Remote** | Öğrenilen kodları gönder | ✅ Hazır |
| **Bluetooth KB** | Bluetooth HID klavye olarak bağlan | ✅ Hazır |
| **WiFi Kontrol** | HTTP/ESP-NOW ile cihaz kontrolü | 🔄 Planlandı |
| **Ayarlar** | WiFi yapılandırma, cihaz bilgisi, IR temizleme, restart | ✅ Hazır |

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
  │  GPIO 25 ─── BTN_OK  ──/~~ GND          │
  │  GPIO 26 ─── BTN_MENU──/~~ GND          │
  │                                          │
  │  GPIO 13 ─── IR Alıcı (VS1838B) OUT     │
  │  GPIO 14 ─── IR LED (gönderme)          │
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
| OK    | GPIO 25 |
| MENU  | GPIO 26 |

### IR Alıcı (VS1838B / TSOP38238)

| Pin | Bağlantı |
|-----|----------|
| OUT | GPIO 13 |
| VCC | 3.3V |
| GND | GND |

### IR LED (Gönderme)

IR LED'i doğrudan GPIO 14 üzerinden 100Ω seri dirençle GND'ye bağlayın. Daha güçlü sinyal için:

```
GPIO 14 ── 1kΩ ──┐
                 Baz
              [2N2222]
                 Emiter ── GND
                 Kollektör ── IR LED ── 100Ω ── 3.3V
```

---

## Gerekli Kütüphaneler

Arduino IDE → Sketch → Include Library → Manage Libraries:

| Kütüphane | Yazan | Kurulum |
|-----------|-------|---------|
| **IRremote** | shirriff / z3t0 / ArminJo | `IRremote` ara ve kur |
| **Adafruit GFX** | Adafruit | `Adafruit GFX Library` |
| **Adafruit ST7735** | Adafruit | `Adafruit ST7735 Library` |
| **BleKeyboard** | T-vK | `ESP32 BLE Keyboard` |

ESP32 board desteği için:
- Arduino IDE: Dosya → Tercihler → "Ek Board Yöneticisi URL'leri"ne `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` ekle
- Araçlar → Board → Board Manager → "ESP32" ara ve kur
- Board seçimi: `ESP32 Dev Module`

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

## Proje Yapısı

```
pixiZ-v1/
├── pixiZ-v1/
│   └── pixiZ-v1.ino      # Ana kod
├── LICENSE                 # MIT Lisansı
└── README.md               # Bu dosya
```

---

## Yol Haritası

- [x] IR öğrenme ve saklama
- [x] IR kod gönderme
- [x] Bluetooth HID klavye
- [ ] Bluetooth üzerinden makro/tuş vuruşu gönderme
- [ ] Wi-Fi / ESP-NOW cihaz kontrolü
- [ ] IR kodlarını kategorilendirme
- [ ] Daha fazla IR protokol desteği
- [ ] OTA güncelleme

---

## Lisans

MIT License — Detaylar için [LICENSE](LICENSE) dosyasına bakın.

---

## İletişim

- GitHub: [@azerenes](https://github.com/azerenes)
- Proje: [github.com/azerenes/pixiZ-v1](https://github.com/azerenes/pixiZ-v1)
- İlham: [PolyCast5](https://polycast5.com) — Multi-Tool Everything Remote
