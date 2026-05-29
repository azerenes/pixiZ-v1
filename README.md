# pixiZ v7 — Evrensel Multi-Tool

**ESP32 DevKit V4** tabanlı, **PolyCast5** + **Flipper Zero** + **Bruce** + **ESP32 Marauder** hibriti çok fonksiyonlu bir cihaz. IR öğrenme/gönderme, Bluetooth HID klavye, Wi-Fi pentest, BLE spam, BadUSB, Sub-GHz, NFC/RFID, LoRa ve daha fazlasını tek cihazda birleştirir.

![pixiZ](https://img.shields.io/badge/status-active-brightgreen)
![ESP32](https://img.shields.io/badge/ESP32-DevKit%20V4-blue)
![License](https://img.shields.io/badge/license-MIT-green)

---

## İçindekiler

- [Özellik Listesi (Tümü)](#özellik-listesi-tümü)
  - [📡 Wi-Fi (ESP32 Dahili — Ek Donanım Yok)](#-wi-fi-esp32-dahili--ek-donanım-yok)
  - [📶 Bluetooth / BLE (ESP32 Dahili — Ek Donanım Yok)](#-bluetooth--ble-esp32-dahili--ek-donanım-yok)
  - [📟 IR (VS1838B + IR LED — Mevcut Donanım)](#-ir-vs1838b--ir-led--mevcut-donanım)
  - [🎛️ Sub-GHz (CC1101 Modülü Gerekli)](#️-sub-ghz-cc1101-modülü-gerekli)
  - [📇 NFC/RFID (PN532 Modülü Gerekli)](#-nfcrfid-pn532-modülü-gerekli)
  - [📻 LoRa (SX1278 Modülü Gerekli)](#-lora-sx1278-modülü-gerekli)
  - [🖥️ ESP-NOW (ESP32 Dahili)](#️-esp-now-esp32-dahili)
  - [⌨️ BadUSB / Ducky Script](#️-badusb--ducky-script)
  - [🔐 Güvenlik / Diğer](#-güvenlik--diğer)
  - [🧩 NRF24 (NRF24 Modülü Gerekli)](#-nrf24-nrf24-modülü-gerekli)
  - [🎵 FM Radyo (TEA5767 Modülü Gerekli)](#-fm-radyo-tea5767-modülü-gerekli)
- [Devre Şeması](#devre-şeması)
- [Gerekli Kütüphaneler](#gerekli-kütüphaneler)
- [Yol Haritası](#yol-haritası)

---

## Özellik Listesi (Tümü)

Renk kodu:\
🟢 **Hazır** (şu an çalışıyor) · 🟡 **Hemen eklenir** (ek donanım yok) · 🔴 **Modül gelince** · ⚫ **İleride**

---

### 📡 Wi-Fi (ESP32 Dahili — Ek Donanım Yok)

| # | Özellik | Durum | Açıklama |
|---|---------|-------|----------|
| 1 | **Wi-Fi Ağ Tarama** | 🟢 Hazır | Çevredeki Wi-Fi ağlarını tara, SSID ve sinyal gücünü listele. |
| 2 | **Beacon Spam (Rastgele)** | 🟡 Hemen | 100+ sahte Wi-Fi ağı yayınla. Telefon/bilgisayarın Wi-Fi listesini spam'le, gerçek ağları bulmayı zorlaştır. Flipper Zero'nun en ünlü eğlence özelliği. Kafe, okul, toplu taşıma gibi kalabalık ortamlarda kullanılır. |
| 3 | **Beacon Spam (Listeden)** | 🟡 Hemen | Önceden belirlediğin bir listeden gerçekçi AP isimleri bas. "TurkTelekom_XXXX", "Turkcell_Superonline" gibi inandırıcı görünen sahte ağlar oluştur. |
| 4 | **AP Clone Spam** | 🟡 Hemen | Çevredeki gerçek Wi-Fi ağlarını tara, birebir aynı isimle sahte kopyalarını yayınla. 30 tane "Cafe_WiFi" görünür — hangisi gerçek bilinmez. Evil Twin saldırısının ön adımı. |
| 5 | **Deauth Flood** | 🟡 Hemen | Belirli bir Wi-Fi ağına sürekli "bağlantını kes" paketi gönder. O ağa bağlı herkesin interneti kopar. Evil Twin'den önce kurbanı düşürmek için kullanılır. ⚠️ Yasal uyarı: kendi ağın değilse izinsiz kullanma. |
| 6 | **Deauth Detector** | 🟡 Hemen | Çevrede birisi deauth paketi gönderiyorsa seni uyarır. Ekranda "Saldırı tespit edildi! Kaynak: XX:XX:XX → Hedef: YY:YY:YY" gösterir. Savunma amaçlı — "biri ağa saldırıyor" dersin. |
| 7 | **PMKID Capture** | 🟡 Hemen | WPA2/WPA3 korumalı bir ağın şifre hash'ini yakala. Bilgisayara aktarıp Hashcat ile kırarak Wi-Fi şifresini bul. Ağa bağlı olmana gerek yok, sadece bir cihazın bağlanmasını beklersin. En ciddi Wi-Fi pentest yöntemi. |
| 8 | **Probe Request Sniffer** | 🟡 Hemen | Telefonlar/bilgisayarlar "daha önce bağlandığım ağ var mı?" diye etrafa SSID adları yayınlar. Bunları toplayarak bir kişinin evindeki ağ adını, gittiği kafeyi, kaldığı oteli çıkarabilirsin. Gizlilik ihlali — insanlar farkında olmadan geçmişlerini etrafa yayınlar. |
| 9 | **Evil Twin** | 🟡 Hemen | Bir Wi-Fi ağının birebir kopyasını (aynı SSID + MAC) aç, orijinaline deauth bas, kurban otomatik senin AP'ne bağlansın. Trafiğini izleyebilir, girdiği siteleri görebilirsin. |
| 10 | **Evil Portal** | 🟡 Hemen | Evil Twin'in bir adım ötesi. Kurban sahte AP'ne bağlanınca karşısına "WiFi şifrenizi girin" veya "Facebook ile giriş" sayfası çıkar. Girilen bilgiler pixiZ'de görünür. İnsanların %90'ı hiç düşünmeden şifresini girer — sosyal mühendislik. |
| 11 | **Wardriving** | 🟡 Hemen | Gezerken çevredeki tüm Wi-Fi ağlarını topla, kaydet. WiGLE.net'e yükleyerek dünya Wi-Fi haritasına katkıda bulun. GPS modülü yoksa sadece AP listesi kaydedilir. |
| 12 | **Wi-Fi RAW Sniffer** | 🟡 Hemen | Tüm Wi-Fi paketlerini çiğ olarak yakala, Serial'dan Wireshark formatında dök. Bilgisayarda analiz edebilirsin. |
| 13 | **Scan Hosts + Port Tarama** | 🟡 Hemen | Ağdaki diğer cihazları tara, IP adreslerini ve açık portlarını (80=HTTP, 22=SSH, 443=HTTPS) listele. Güvensiz cihazları bulmak için. |
| 14 | **ARP Spoofing / Poisoning** | 🟡 Hemen | Ağdaki cihazların ARP tablosunu zehirle, trafiği kendi üzerinden geçir. Ortadaki adam (MITM) saldırısı. Kurbanın girdiği siteleri, ham HTTP isteklerini (şifre dahil) görürsün. |
| 15 | **Association Sleep Attack** | 🟡 Hemen | İstemciye sahte "ilişkilendirme yanıtı" göndererek bağlantısını boz. Cihaz "bağlıyım" sanır ama interneti çalışmaz. Deauth'tan farkı: cihaz tekrar bağlanmayı denemez. |
| 16 | **Beacon Sniff (AP Keşif)** | 🟡 Hemen | Çevredeki tüm AP'leri derinlemesine tara. Gizli SSID'leri bulma dahil. |
| 17 | **Telnet / SSH Client** | 🟡 Hemen | pixiZ üzerinden başka bir cihaza Telnet veya SSH ile bağlan, komut çalıştır. Acil durumda cebinde terminal. |
| 18 | **WireGuard Tünel** | 🟡 Hemen | WireGuard VPN ile güvenli tünel. pixiZ'i VPN istemcisi gibi kullan. |
| 19 | **OTA Güncelleme** | 🟡 Hemen | Wi-Fi üzerinden kablosuz firmware güncelleme. USB gerekmez. |

---

### 📶 Bluetooth / BLE (ESP32 Dahili — Ek Donanım Yok)

| # | Özellik | Durum | Açıklama |
|---|---------|-------|----------|
| 1 | **BLE Scanner** | 🟡 Hemen | Çevredeki tüm BLE cihazlarını (kulaklık, saat, telefon, araba, sensör) tara. MAC adresi, sinyal gücü (RSSI), cihaz adı, servisleri göster. |
| 2 | **BLE Spam — iOS (Apple Continuity/AirDrop)** | 🟡 Hemen | iPhone'ların AirDrop protokolünü taklit et. Tüm iPhone'larda "pixiZ bir şey paylaşmak istiyor" popup'ı çıkar. Flipper Zero'nun en sevilen özelliği. iOS 17+ kısmen engelledi ama hala çalışan yöntemler var. |
| 3 | **BLE Spam — Samsung Phone** | 🟡 Hemen | Samsung'un Quick Share protokolünü taklit et. "Yakında Samsung Phone var" bildirimi çıkar. |
| 4 | **BLE Spam — Windows** | 🟡 Hemen | Windows Swift Pair özelliğini tetikle. "Microsoft Surface" veya "Yakında Bluetooth cihazı" bildirimi. |
| 5 | **BLE Spam — Android** | 🟡 Hemen | Google Fast Pair protokolünü taklit et. "Yakında cihaz var" bildirimi. |
| 6 | **BLE Spam — All (Hepsi)** | 🟡 Hemen | iOS + Samsung + Windows + Android — hepsini aynı anda bas. Ortamdaki her telefona bildirim yağdır. |
| 7 | **BadBLE / BadUSB (Ducky Script over BT)** | 🟡 Hemen | Bluetooth klavye olarak bağlan, önceden yazılmış tuş vuruşlarını otomatik oynat. `Win+R → cmd → powershell -NoP -W Hidden -enc <base64>` ile reverse shell. |
| 8 | **Apple AirTag Monitor/Sniff** | 🟡 Hemen | Çevredeki AirTag'leri tespit et. Stalker takibi mi yapılıyor öğren. Veya AirTag sinyalini yakalayıp takip et. |
| 9 | **BT HID Klavye (Mevcut)** | 🟢 Hazır | Bilgisayar/telefona Bluetooth klavye olarak bağlan. BadUSB'in temel katmanı. |

---

### 📟 IR (VS1838B + IR LED — Mevcut Donanım)

| # | Özellik | Durum | Açıklama |
|---|---------|-------|----------|
| 1 | **IR Öğrenme + Saklama** | 🟢 Hazır | Herhangi bir IR kumandanın sinyalini öğren, NVS'ye kaydet. TV, klima, ses sistemi, uydu alıcısı — 38kHz taşıyıcı frekansında çalışan her şey. |
| 2 | **IR Gönderme (Multi-Button)** | 🟢 Hazır | Kayıtlı sinyalleri tekrarla. Her kumanda için 16'ya kadar buton. |
| 3 | **TV-B-Gone** | 🟡 Hemen | 200'den fazla TV markasının kapatma kodunu sırayla gönder — ortamdaki tüm TV'leri kapat. Toplantı odasında efsane espiri. |
| 4 | **IR RAW Capture** | 🟡 Hemen | Bilinmeyen protokollerin ham timing verilerini yakala. Protokol bilinmese bile aynen tekrarlanabilir. |
| 5 | **Custom IR Protokolleri** | 🟢 Hazır | NEC, NECext, SIRC (Sony 12/15/20), Samsung32, RC5, RC5X, RC6 desteği. |

---

### 🎛️ Sub-GHz (CC1101 Modülü Gerekli)

| # | Özellik | Durum | Açıklama |
|---|---------|-------|----------|
| 1 | **Sub-GHz Scan** | 🔴 Modül | 433/868/915 MHz bandında sinyal tara. Kapı zilleri, garaj kapıları, uzaktan kumandalı prizler, telsizler bu frekanslarda çalışır. |
| 2 | **Sub-GHz Replay** | 🔴 Modül | Sinyali yakala, aynen tekrarla. Flipper Zero'nun en popüler özelliği — bir düğmeye basar gibi kapı açma. |
| 3 | **Sub-GHz Spectrum** | 🔴 Modül | Frekans bandında canlı spektrum göster. Hangi frekansta sinyal var, gücü ne kadar grafik olarak gör. |
| 4 | **Sub-GHz Jammer** | 🔴 Modül | Belirli frekansta sürekli sinyal bas — tüm iletişimi bloke et. Kapı zillerini, garaj kapılarını geçici işlemez yap. |
| 5 | **Custom Frequency** | 🔴 Modül | 300-1000 MHz arası manuel frekans ayarı. Özel cihazlarla iletişim. |

---

### 📇 NFC/RFID (PN532 Modülü Gerekli)

| # | Özellik | Durum | Açıklama |
|---|---------|-------|----------|
| 1 | **NFC Kart Okuma** | 🔴 Modül | Mifare Classic, Ultralight, NTAG kartlarını oku. UID, blok verilerini göster. Otel kartı, okul kartı, metro kartı. |
| 2 | **NFC Kart Klonlama** | 🔴 Modül | Kartın tüm blok verilerini kopyala, boş karta yaz. Şifresiz kartlar birebir klonlanabilir. |
| 3 | **NFC Kart Yazma** | 🔴 Modül | Boş NFC etikete URL, metin, kartvizit yaz. |
| 4 | **NFC Taklit (Emulate)** | 🔴 Modül | pixiZ okuduğun kartı taklit etsin. Turnikede pixiZ'i okut, kartmış gibi davransın. |
| 5 | **NDEF Yazma** | 🔴 Modül | NFC etikete standart NDEF formatında veri yaz: URL, telefon, SMS, Wi-Fi şifresi. |
| 6 | **125kHz RFID** | ⚫ İleride | Düşük frekans RFID kartları. Ayrı devre gerek. |
| 7 | **Amiibolink** | 🔴 Modül | Nintendo Amiibo figürlerini taklit et. Oyunlarda nadir eşyaları aç. |

---

### 📻 LoRa (SX1278 Modülü Gerekli)

| # | Özellik | Durum | Açıklama |
|---|---------|-------|----------|
| 1 | **LoRa Mesajlaşma** | 🔴 Modül | 1km+ menzil, altyapısız haberleşme. Doğa, kamp, afet durumu. |
| 2 | **LoRa Remote Control** | 🔴 Modül | Uzaktaki bir cihazı LoRa ile kontrol et. Örn: 500m ötedeki robot kolunu aç/kapa. |

---

### 🖥️ ESP-NOW (ESP32 Dahili)

| # | Özellik | Durum | Açıklama |
|---|---------|-------|----------|
| 1 | **ESP-NOW Mesajlaşma** | 🟡 Hemen | İki ESP32 arasında router'sız direkt iletişim. ESP32'li robot koluna veya akıllı eve komut gönder. |
| 2 | **ESP-NOW Dosya Transferi** | 🟡 Hemen | ESP32'ler arası dosya gönderme (IR yedekleri, konfigürasyon). |
| 3 | **ESP-NOW Komut Gönderme** | 🟡 Hemen | Önceden tanımlı komutları gönder: "ışıkları aç", "motoru çalıştır". |

---

### ⌨️ BadUSB / Ducky Script

| # | Özellik | Durum | Açıklama |
|---|---------|-------|----------|
| 1 | **BT Klavye Bağlantısı** | 🟢 Hazır | Bluetooth üzerinden bilgisayara klavye olarak bağlan. |
| 2 | **Ducky Script Payload** | 🟡 Hemen | USB Rubber Ducky formatında tuş vuruşu betikleri yaz, NVS'ye kaydet, tek tıkla çalıştır. `Win+R → powershell → Enter` ile reverse shell al. |
| 3 | **Insta-Type** | 🟡 Hemen | Sık kullanılan metinleri (e-posta, paragraf) saniyeler içinde otomatik yaz. |
| 4 | **AI Destekli Payload** | ⚫ İleride | Mikrofona "bilgisayarı kapat" de, AI bunu tuş vuruşlarına çevirsin. İnternet gerek. |

---

### 🔐 Güvenlik / Diğer

| # | Özellik | Durum | Açıklama |
|---|---------|-------|----------|
| 1 | **Çevrimdışı Şifre Yöneticisi** | 🟡 Hemen | Tüm şifrelerini NVS'de sakla, BT klavye ile otomatik doldur. PoliCast5'in en sevilen özelliği. |
| 2 | **QR Kod Oluşturucu** | 🟡 Hemen | Ekranda QR kod göster. Wi-Fi şifresi paylaş, URL göster, metin paylaş. |
| 3 | **NTP Saat** | 🟡 Hemen | Wi-Fi üzerinden doğru saati al, ekranda göster. |
| 4 | **SD Kart Yöneticisi** | 🔴 Modül | SD karttaki dosyaları gez, oku, sil. IR yedekleri, wardriving verileri. |
| 5 | **WebUI (Tarayıcıdan Kontrol)** | 🟡 Hemen | pixiZ AP açar, tarayıcıdan tüm özellikleri kullan. |
| 6 | **JavaScript Script Motoru** | ⚫ İleride | pixiZ üzerinde JavaScript çalıştır. Oyun, hesaplama, otomasyon. |

---

### 🧩 NRF24 (NRF24 Modülü Gerekli)

| # | Özellik | Durum | Açıklama |
|---|---------|-------|----------|
| 1 | **NRF24 Jammer** | 🔴 Modül | 2.4GHz NRF24 cihazlarını (kablosuz klavye/fare, oyuncak, drone) bloke et. |
| 2 | **2.4GHz Spectrum** | 🔴 Modül | 2.4GHz bandında canlı spektrum analizi. Hangi kanal yoğun göster. |
| 3 | **Mousejack** | 🔴 Modül | Kablosuz fare/klavye sinyallerini yakala ve taklit et. Logitech alıcılara saldırı. |

---

### 🎵 FM Radyo (TEA5767 Modülü Gerekli)

| # | Özellik | Durum | Açıklama |
|---|---------|-------|----------|
| 1 | **FM Broadcast** | 🔴 Modül | FM radyo yayını yap. pixiZ'i mini radyo istasyonuna çevir. 200m çevre. |
| 2 | **FM Spectrum** | 🔴 Modül | 88-108 MHz bandında yayınları göster. |
| 3 | **Trafik Anonsu Hijack** | 🔴 Modül | Araba radyosunda RDS trafik anonsunu taklit et, kendi yayınını bastır. |

---

## Devre Şeması

```
  ESP32 DevKit V4
  ┌──────────────────────────────────────────┐
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
  │  GPIO 27 ─── IR LED (gönderme)           │
  │              │                           │
  │              +─/\/\/──>|── GND           │
  │               100Ω    IR LED             │
  │                                          │
  │  GPIO 21 ─── PN532 SDA (I2C)  [ileride] │
  │  GPIO 22 ─── PN532 SCL (I2C)  [ileride] │
  │  GPIO 15 ─── CC1101/SX1278 CS [ileride] │
  │  GPIO 4  ─── SD Card CS       [ileride] │
  └──────────────────────────────────────────┘
```

### ST7735 TFT (1.8" 160x128 SPI — Mevcut)

| TFT | ESP32 |
|-----|-------|
| CS  | GPIO 5 |
| RST | GPIO 16 |
| DC  | GPIO 17 |
| MOSI | GPIO 23 |
| SCLK | GPIO 18 |

### Butonlar (INPUT_PULLUP — Butonun Diğer Ucu GND)

| Buton | GPIO |
|-------|------|
| UP    | GPIO 32 |
| DOWN  | GPIO 33 |
| OK    | GPIO 14 |
| MENU  | GPIO 12 |

### IR

| Pin | GPIO |
|-----|------|
| Alıcı (VS1838B OUT) | GPIO 13 |
| LED (Gönderme) | GPIO 27 |

### Gelecek Modüller

| Modül | Bağlantı | Pinler |
|-------|----------|--------|
| PN532 NFC/RFID | I2C | GPIO 21 (SDA), GPIO 22 (SCL) |
| CC1101 Sub-GHz | SPI (CS) | GPIO 15 (CS) |
| SX1278 LoRa | SPI (CS) | GPIO 4 (CS) |
| SD Kart | SPI (CS) | GPIO 2 (CS) |

*TFT + CC1101 + SX1278 + SD Kart aynı SPI bus'ını paylaşır (MOSI/SCLK ortak), her birinin ayrı CS pini vardır.*

---

## Gerekli Kütüphaneler

Arduino IDE → Sketch → Include Library → Manage Libraries:

| Kütüphane | Arduino'daki Adı | Gerekli Mi? |
|-----------|-----------------|-------------|
| **IRremote** | `IRremote` by ArminJo | ✅ Evet |
| **HijelHID_BLEKeyboard** | `HijelHID_BLEKeyboard` by HijelHub | ✅ Evet |
| **NimBLE-Arduino** | `NimBLE-Arduino` by h2zero | ✅ Evet |
| **Adafruit GFX** | `Adafruit GFX Library` | ✅ Evet |
| **Adafruit ST7735** | `Adafruit ST7735 Library` | ✅ Evet |
| **QRCode** | `QRCode` by ricmoo | ✅ Evet |
| **PN532** | `Adafruit PN532` | 🔴 NFC için |
| **CC1101** | `ELECHOUSE_CC1101` | 🔴 Sub-GHz için |
| **LoRa** | `LoRa` by Sandeep Mistry | 🔴 LoRa için |
| **SD Card** | `SD` (ESP32 dahili) | 🔴 SD için |
| **NRF24** | `RF24` by TMRh20 | 🔴 NRF24 için |
| **TEA5767** | (manuel eklenti) | 🔴 FM için |

ESP32 board kurulumu:
- Arduino IDE: Dosya → Tercihler → "Ek Board Yöneticisi URL'leri"ne `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` ekle
- Araçlar → Board → Board Manager → "ESP32" ara ve kur
- Board seçimi: `ESP32 Dev Module`
- Partition Scheme: `No OTA (Large APP)` (2MB flash için önerilir)

---

## Proje Yapısı

```
pixiZ-v1/
├── pixiZ-v1/
│   └── pixiZ-v1.ino      # Tek dosya — tüm modüller içinde
├── docs/
│   └── WIRING.md          # Donanım bağlantı şemaları
├── LICENSE                # MIT Lisansı
└── README.md              # Bu dosya
```

Tüm modüller tek `.ino` dosyası içinde `#define ENABLE_xxx 0/1` ile yönetilir.
Yeni donanım eklendiğinde ilgili flag 1 yapmak yeterli.

---

## Devre Şeması

Tüm bağlantı detayları, pin tabloları ve uyarılar için:
➡️ **[docs/WIRING.md](docs/WIRING.md)**

### Mevcut Donanım

| Bileşen | ESP32 Pini |
|---------|-----------|
| ST7735 TFT | CS=5, RST=16, DC=17, MOSI=23, MISO=19, SCK=18 |
| Buton Yukarı | GPIO 32 (INPUT_PULLUP) |
| Buton Aşağı | GPIO 33 (INPUT_PULLUP) |
| Buton OK | GPIO 14 (INPUT_PULLUP) |
| Buton Menü | GPIO 12 (INPUT_PULLUP) |
| VS1838B IR Alıcı | GPIO 13 |
| IR LED (Gönderici) | GPIO 27 (100Ω ile) |

### Modül Pin Özeti (docs/WIRING.md'de detaylı)

| Modül | Arayüz | CS/Seçme | Diğer |
|-------|--------|----------|-------|
| CC1101 (Sub-GHz) | SPI (18/19/23) | GPIO 4 | GDO2=2 |
| PN532 (NFC/RFID) | I²C (21/22) | 0x24 | RST=15 |
| SX1278 (LoRa) | SPI (18/19/23) | GPIO 15 | RST=2, DIO0=4 |
| SD Kart | SPI (18/19/23) | GPIO 26 | — |
| NRF24L01+ | SPI (18/19/23) | GPIO 4 | CE=2 |
| TEA5767 (FM) | I²C (21/22) | 0x60 | — |

---

## Yol Haritası

### 🥇 Aşama 1 ✅ (v4)
- [x] BadUSB (Ducky Script payload + BT HID)
- [x] Beacon Spam (rastgele + listeden)
- [x] AP Clone Spam
- [x] Deauth Flood + Deauth Detector
- [x] BLE Spam (iOS + Samsung + Android + Windows + All)
- [x] BLE Scanner
- [x] TV-B-Gone
- [x] Probe Request Sniffer
- [x] PMKID Capture
- [x] IR RAW Capture

### 🥈 Aşama 2 ✅ (v5-v6)
- [x] Evil Twin + Evil Portal (4 şablon: WiFi/FB/IG/Twitter)
- [x] ARP Spoofing / Poisoning
- [x] Scan Hosts + Port Tarama
- [x] Wi-Fi RAW Sniffer
- [x] AirTag Monitor / Sniff
- [ ] Association Sleep Attack *(ileride)*
- [x] Beacon Sniff (detaylı AP listesi)
- [x] ESP-NOW Mesajlaşma + Komut Gönderme
- [x] Çevrimdışı Şifre Yöneticisi (CRUD + BT autotype)
- [x] QR Kod Oluşturucu

### 🥉 Aşama 3 ✅ (v7)
- [x] Wardriving (AP logger + NVS depolama)
- [ ] Telnet / SSH Client
- [ ] WireGuard Tünel
- [x] WebUI (Tarayıcıdan Kontrol)
- [x] Wi-Fi AP + İstemci Modu
- [ ] OTA Güncelleme
- [ ] NTP Saat
- [ ] ESP-NOW Dosya Transferi

### 🔌 Aşama 4 — Donanım Geldikçe
- [ ] **CC1101** → Sub-GHz Scan/Replay/Jammer/Spectrum
- [ ] **PN532** → NFC Okuma/Klonlama/Yazma/Emulate/Amiibo
- [ ] **SX1278** → LoRa Mesajlaşma
- [ ] **SD Kart** → Dosya Yöneticisi
- [ ] **NRF24** → Jammer/Spectrum/Mousejack
- [ ] **TEA5767** → FM Broadcast/Spectrum/Hijack

---

## Lisans

MIT License — Detaylar için [LICENSE](LICENSE) dosyasına bakın.

---

## İletişim

- GitHub: [@azerenes](https://github.com/azerenes)
- Proje: [github.com/azerenes/pixiZ-v1](https://github.com/azerenes/pixiZ-v1)
- İlham: [PolyCast5](https://polycast5.com), [Bruce](https://bruce.computer), [ESP32 Marauder](https://github.com/justcallmekoko/ESP32Marauder), [Flipper Zero](https://flipperzero.one)
