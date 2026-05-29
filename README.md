# pixiZ-v1 — AI Assistant

ESP32-S2 Mini tabanlı, ST7735 TFT ekran ve sesli AI asistan özellikli çok fonksiyonlu masaüstü cihaz.

![pixiZ-v1](https://img.shields.io/badge/status-active-brightgreen)
![ESP32](https://img.shields.io/badge/ESP32-S2%20Mini-blue)
![License](https://img.shields.io/badge/license-MIT-green)

---

## Özellikler

| # | Özellik | Açıklama |
|---|---------|----------|
| 1 | **Dijital Saat** | NTP ile otomatik zaman senkronizasyonu, Türkçe gün isimleri |
| 2 | **Dolar Kuru** | ExchangeRate API ile canlı USD/TRY |
| 3 | **Euro Kuru** | ExchangeRate API ile canlı EUR/TRY |
| 4 | **Bitcoin** | BTCTurk API ile anlık BTC/USDT |
| 5 | **Ethereum** | BTCTurk API ile anlık ETH/USDT |
| 6 | **Görev Listesi** | Google Apps Script ile hatırlatıcılar |
| 7 | **AI Sesli Asistan** | MAX4466 mikrofon + Gemini API ile sesli komut |
| 8 | **Slayt Yönetimi** | Gösterilecek slaytları aç/kapa |
| 9 | **Otomatik Geçiş** | 12 saniyede bir slayt değişimi |

---

## Devre Şeması

```
                        ┌─────────────────────────┐
                        │      ESP32-S2 Mini       │
                        │                         │
                        │  3V3 ─── VCC (ST7735)   │
                        │  3V3 ─── VCC (MAX4466)  │
                        │  GND ─── GND (tüm)      │
                        │                         │
                        │  GPIO 1  ─── TFT LED    │
                        │  GPIO 2  ─── TFT SCLK   │
                        │  GPIO 4  ─── TFT MOSI   │
                        │  GPIO 6  ─── TFT DC     │
                        │  GPIO 8  ─── TFT RST    │
                        │  GPIO 10 ─── TFT CS     │
                        │                         │
                        │  GPIO 7  ─── MAX4466 OUT│
                        │                         │
                        │  GPIO 33 ─── BTN_AI     │
                        │  GPIO 35 ─── BTN_UP     │
                        │  GPIO 37 ─── BTN_DOWN   │
                        │  GPIO 39 ─── BTN_MENU   │
                        │                         │
                        │  USB ─── 5V Güç         │
                        └─────────────────────────┘
```

### ST7735 TFT (1.8" 160x128 SPI)

| TFT Pin | Bağlantı |
|---------|----------|
| LED     | GPIO 1 (veya 3.3V) |
| SCLK    | GPIO 2 |
| MOSI    | GPIO 4 |
| DC      | GPIO 6 |
| RST     | GPIO 8 |
| CS      | GPIO 10 |
| VCC     | 3.3V |
| GND     | GND |

### MAX4466 Mikrofon

| MAX4466 | Bağlantı |
|---------|----------|
| OUT     | GPIO 7 (ADC) |
| VCC     | 3.3V |
| GND     | GND |

### Butonlar (INPUT_PULLUP)

| Buton | GPIO |
|-------|------|
| AI    | GPIO 33 |
| UP    | GPIO 35 |
| DOWN  | GPIO 37 |
| MENU  | GPIO 39 |

Tüm butonların diğer ucu **GND**'ye bağlanır.

---

## Kurulum

### 1. Gerekli Kütüphaneler

Arduino IDE'yi açıp **Sketch → Include Library → Manage Libraries**'den aşağıdakileri kur:

| Kütüphane | Sürüm | Yazan |
|-----------|-------|-------|
| Adafruit GFX | ≥1.11 | Adafruit |
| Adafruit ST7735 | ≥1.10 | Adafruit |
| ArduinoJson | ≥7.0 | Benoit Blanchon |

ESP32 board desteği için:
- **Arduino IDE**: Dosya → Tercihler → "Ek Board Yöneticisi URL'leri"ne `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` ekle
- Araçlar → Board → Board Manager → "ESP32" ara ve kur

### 2. WiFi ve API Anahtarları

Kodun başındaki şu değişkenleri kendine göre düzenle:

```cpp
const char* ssid     = "wifi_adiniz";
const char* password = "wifi_sifreniz";
const String apiKey    = "GEMINI_API_ANAHTARI";
const String exKey     = "EXCHANGERATE_API_ANAHTARI";
const String scriptURL = "GOOGLE_APPS_SCRIPT_URL";
```

### 3. API Anahtarları Nasıl Alınır

| Servis | Nasıl Alınır |
|--------|-------------|
| **Gemini API** | https://aistudio.google.com/app/apikey → "Create API Key" |
| **ExchangeRate API** | https://exchangerate-api.com → Ücretsiz kayıt, 1500 sorgu/ay |
| **Google Apps Script** | script.google.com → Yeni proje → Web uygulaması olarak yayınla |

---

## Kullanım

### Buton Kontrolleri

| Tuş | Fonksiyon |
|-----|-----------|
| **UP** | Bir önceki slayt |
| **DOWN** | Bir sonraki slayt |
| **MENU** | Ana menüyü aç |
| **AI** | Sesli asistanı başlat (basılı tut, bırakınca gönder) |

### Menü İşlemleri

| Menü | İşlem |
|------|-------|
| Canlı Piyasa | Tüm kur ve kriptolar tek ekranda |
| Slayt Yönetimi | Hangi slaytların gösterileceğini seç |
| Hatırlatıcı | Google Apps Script'ten görevleri getir |
| WiFi Bilgisi | Bağlı ağın SSID/IP/RSSI bilgisi |
| Yeniden Başlat | Cihazı resetle |

---

## Ekran Görüntüleri

| Slayt | Renk Teması |
|-------|-------------|
| Saat | Mavi/Mor |
| Dolar | Yeşil |
| Euro | Altın |
| Bitcoin | Turuncu |
| Ethereum | Mavi |
| Görevler | Kırmızı/Pembe |

Tema, kodun başındaki `MODERN_DESIGN` sabiti ile değiştirilebilir:
- `1` = Modern düz tasarım
- `0` = Retro piksel tasarım

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

## Lisans

MIT License — Detaylar için [LICENSE](LICENSE) dosyasına bakın.

---

## İletişim

- GitHub: [@azerenes](https://github.com/azerenes)
- Proje: [github.com/azerenes/pixiZ-v1](https://github.com/azerenes/pixiZ-v1)
