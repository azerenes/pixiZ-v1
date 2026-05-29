# pixiZ v7 — Donanım Bağlantı Şeması

ESP32 DevKit V4 + ST7735 TFT + VS1838B IR Alıcı + IR LED + 4 Buton

---

## 📌 Kullanılan Pinler

| Pin | Bağlantı         | Açıklama              |
|-----|------------------|-----------------------|
| 5   | TFT_CS           | ST7735 Chip Select    |
| 16  | TFT_RST          | ST7735 Reset          |
| 17  | TFT_DC           | ST7735 Data/Command   |
| 23  | TFT_MOSI         | ST7735 MOSI (HW SPI)  |
| 19  | TFT_MISO         | ST7735 MISO (HW SPI)  |
| 18  | TFT_SCK          | ST7735 SCK (HW SPI)   |
| 32  | BUTON_UP         | Yukarı (INPUT_PULLUP) |
| 33  | BUTON_DOWN       | Aşağı  (INPUT_PULLUP) |
| 14  | BUTON_OK         | Onay   (INPUT_PULLUP) |
| 12  | BUTON_MENU       | Menü   (INPUT_PULLUP) |
| 13  | IR_RECV          | VS1838B IR Alıcı      |
| 27  | IR_SEND          | IR LED (Gönderici)    |
| 3V3 | VCC              | 3.3V güç              |
| GND | GND              | Ortak toprak          |

---

## 🔌 Modül Bağlantıları (Gelecek)

Aşağıdaki modüller henüz takılı değildir. Kod yapısı `#define ENABLE_xxx 1` ile hazırdır.
Modül geldiğinde aşağıdaki tabloya göre bağlayın ve `.ino` dosyasında ilgili tanımı `1` yapın.

---

### 1️⃣ CC1101 — Sub-GHz (433/868/915 MHz)

`#define ENABLE_SUBGHZ 1`

CC1101 SPI modülü. IRQ kesmesi için pin 4 kullanılır.

```
ESP32          CC1101
─────          ──────
3V3      ───→  VCC
GND      ───→  GND
GPIO 18  ───→  SCK
GPIO 19  ───→  MISO
GPIO 23  ───→  MOSI
GPIO 4   ───→  CS (veya GDO0)
GPIO 2   ───→  GDO2 (IRQ - opsiyonel)
```

> **Not:** CC1101 3.3V ile çalışır. 5V modül kullanıyorsanız seviye dönüştürücü takın.

---

### 2️⃣ PN532 — NFC/RFID (13.56 MHz)

`#define ENABLE_NFC 1`

PN532 I²C modu (önerilen). ADR pin GND = adres 0x24.

```
ESP32          PN532
─────          ──────
3V3      ───→  VCC
GND      ───→  GND
GPIO 21  ───→  SDA
GPIO 22  ───→  SCL
GPIO 15  ───→  RSTO (opsiyonel)
```

> **Alternatif SPI:** PN532 SPI modunda da çalışır. I²C daha az pin kullanır.
> PN532 üzerindeki switchleri doğru moda getirin: I²C = `01` (SEL=0, SCL=1).

---

### 3️⃣ SX1278 — LoRa (868/915 MHz)

`#define ENABLE_LORA 1`

SX1278 SPI modülü (RA-01/RA-02 shield).

```
ESP32          SX1278
─────          ──────
3V3      ───→  VCC
GND      ───→  GND
GPIO 18  ───→  SCK
GPIO 19  ───→  MISO
GPIO 23  ───→  MOSI
GPIO 15  ───→  NSS (CS)
GPIO 2   ───→  RST
GPIO 4   ───→  DIO0 (IRQ - opsiyonel)
```

> **Dikkat:** SX1278 3.3V mantık seviyesi ister. Anten bağlamadan vericiyi çalıştırmayın.

---

### 4️⃣ MicroSD Kart Modülü (SPI)

`#define ENABLE_SD 1`

SD kart modülü SPI üzerinden. **TFT ile aynı SPI hattını paylaşır**, farklı CS pini kullanır.

```
ESP32          SD Kart
─────          ───────
3V3/5V   ───→  VCC  (modüle göre)
GND      ───→  GND
GPIO 18  ───→  SCK
GPIO 19  ───→  MISO
GPIO 23  ───→  MOSI
GPIO 26  ───→  CS
```

> **Not:** TFT_CS = 5, SD_CS = 26. Aynı SPI veriyolunda iki cihaz farklı CS ile sorunsuz çalışır.
> ESP32'nin SDMMC driver'ı da kullanılabilir (`SD_MMC`), ancak SPI daha kolaydır.

---

### 5️⃣ NRF24L01+ — 2.4 GHz RF

`#define ENABLE_NRF24 1`

NRF24L01+ SPI modülü. 3.3V ile beslenir. **100µF kondansatör** VCC-GND arası şarttır!

```
ESP32          NRF24L01+
─────          ─────────
3V3      ───→  VCC
GND      ───→  GND
GPIO 18  ───→  SCK
GPIO 19  ───→  MISO
GPIO 23  ───→  MOSI
GPIO 4   ───→  CSN
GPIO 2   ───→  CE
```

> **Önemli:** NRF24 çok gürültülüdür. VCC-GND arasına 100µF + 0.1µF kondansatör takın.
> Modül 3.3V'dan fazla gerilime dayanamaz! 5V kullanırsanız yanar.

---

### 6️⃣ TEA5767 — FM Radyo

`#define ENABLE_FM 1`

TEA5767 I²C FM radyo modülü.

```
ESP32          TEA5767
─────          ───────
3V3      ───→  VCC
GND      ───→  GND
GPIO 21  ───→  SDA
GPIO 22  ───→  SCL
```

> **Not:** TEA5767 adresi `0x60` (7-bit). Kulaklık çıkışı doğrudan TEA5767 üzerindedir.

---

## 🧮 Pin Çakışma Tablosu

Aşağıdaki tablo tüm modüller aynı anda bağlandığında oluşacak pin kullanımını gösterir.
Bir modülü kullanmıyorsanız o pinleri boş bırakabilirsiniz.

| GPIO | Kullanan            | Çakışma Riski |
|------|---------------------|---------------|
| 2    | CC1101 GDO2 / NRF24 CE / SX1278 RST | Bu modüller aynı anda kullanılamaz |
| 4    | CC1101 CS / SX1278 DIO0 / NRF24 CSN | Bu modüller aynı anda kullanılamaz |
| 5    | TFT_CS              | Sabit         |
| 12   | BTN_MENU            | Sabit (INPUT_PULLUP) |
| 13   | IR_RECV             | Sabit         |
| 14   | BTN_OK              | Sabit (INPUT_PULLUP) |
| 15   | PN532 RSTO / SX1278 NSS | Çakışma var — sadece biri |
| 16   | TFT_RST             | Sabit         |
| 17   | TFT_DC              | Sabit         |
| 18   | TFT_SCK / CC1101 SCK / SX1278 SCK / SD SCK / NRF24 SCK | **Hepsi aynı SPI** ✅ |
| 19   | TFT_MISO / CC1101 MISO / SX1278 MISO / SD MISO / NRF24 MISO | **Hepsi aynı SPI** ✅ |
| 21   | PN532 SDA / TEA5767 SDA | I²C hattı ✅ (sorunsuz) |
| 22   | PN532 SCL / TEA5767 SCL | I²C hattı ✅ (sorunsuz) |
| 23   | TFT_MOSI / CC1101 MOSI / SX1278 MOSI / SD MOSI / NRF24 MOSI | **Hepsi aynı SPI** ✅ |
| 26   | SD_CS               | Sadece SD     |
| 27   | IR_SEND             | Sabit         |
| 32   | BTN_UP              | Sabit (INPUT_PULLUP) |
| 33   | BTN_DOWN            | Sabit (INPUT_PULLUP) |

---

## ⚡ Ortak SPI Veriyolu

TFT, CC1101, SX1278, SD Kart ve NRF24L01 aynı SPI veriyolunu paylaşır.
Her cihazın ayrı bir **CS (Chip Select)** pini vardır. Bu sayede aynı anda sadece bir cihaz aktiftir.

```
ESP32 MOSI (23) ──┬── TFT
                  ├── CC1101
                  ├── SX1278
                  ├── SD Kart
                  └── NRF24L01

ESP32 MISO (19) ──┬── TFT
                  ├── ...
                  
ESP32 SCK  (18) ──┬── TFT
                  ├── ...
                  
CS Pinler:
  TFT    → GPIO 5
  CC1101 → GPIO 4
  SX1278 → GPIO 15
  SD     → GPIO 26
  NRF24  → GPIO 4  (CC1101 ile çakışır — aynı anda kullanmayın)
```

---

## 📐 Buton Bağlantısı

4 buton — hepsi `INPUT_PULLUP` (butona basınca LOW).

```
3V3 ──┬── [BUTON_UP] ──── GPIO 32
      ├── [BUTON_DOWN] ── GPIO 33
      ├── [BUTON_OK] ──── GPIO 14
      └── [BUTON_MENU] ── GPIO 12
```

> Her butonun bir ayağı GPIO'ya, diğer ayağı GND'ye veya 3V3'e (PULLUP moduna göre).
> Kod `INPUT_PULLUP` kullanır: buton basılı = LOW, basılı değil = HIGH.

---

## 📡 IR Bağlantısı

```
VS1838B IR Alıcı:
  Çıkış → GPIO 13
  VCC   → 3V3
  GND   → GND

IR LED (Gönderici):
  Anot → GPIO 27 (100Ω direnç ile)
  Katot → GND
```

> IR LED akım sınırlaması için 100Ω direnç kullanın.
> VS1838B taşıyıcı frekansı 38 kHz.

---

## 🔧 Güç Tüketimi

| Modül        | Çalışma Akımı | Tepe Akımı |
|--------------|--------------|------------|
| ESP32        | ~80 mA       | ~500 mA    |
| ST7735 TFT   | ~50 mA       | ~80 mA     |
| VS1838B      | ~1 mA        | ~3 mA      |
| IR LED       | ~20 mA       | ~100 mA    |
| CC1101       | ~15 mA       | ~30 mA     |
| PN532        | ~50 mA       | ~150 mA    |
| SX1278       | ~20 mA       | ~120 mA    |
| NRF24L01     | ~12 mA       | ~30 mA     |
| SD Kart      | ~30 mA       | ~100 mA    |
| TEA5767      | ~18 mA       | ~25 mA     |

> **Toplam (tüm modüller):** ~300 mA sürekli, ~1.1 A tepe.
> Güç kaynağınız en az **2A** olmalıdır.
> USB ile çalışırken bilgisayar USB portu genelde 500 mA sınırlıdır — harici güç önerilir.
