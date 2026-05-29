/*
 * pixiZ v3 — Universal Remote
 * PolyCast5 + Flipper Zero hybrid
 * ESP32 DevKit V4
 * github.com/azerenes/pixiZ-v1
 *
 * Modül ekleyince:
 *   config.h'de ENABLE_xxx = 1 yap, gerekli kütüphaneyi kur, pini bağla.
 */

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <IRremote.h>
#include <Preferences.h>
#include <HijelHID_BLEKeyboard.h>
#include <WiFi.h>

// ═══════════════════════════════════════════════════
//  KONFİGÜRASYON
// ═══════════════════════════════════════════════════

// --- Ekran (ST7735) ---
#define TFT_CS    5
#define TFT_RST   16
#define TFT_DC    17

// --- Butonlar (INPUT_PULLUP, diğer uç GND) ---
#define BTN_UP    32
#define BTN_DOWN  33
#define BTN_OK    14
#define BTN_MENU  12

// --- IR ---
#define IR_RECV   13
#define IR_SEND   27

// --- Modül aktif/pasif (1 = açık) ---
#define ENABLE_IR       1   // IR öğrenme/gönderme (mevcut)
#define ENABLE_BT       1   // Bluetooth HID klavye (dahili)
#define ENABLE_WIFI     1   // Wi-Fi tarama (dahili)
#define ENABLE_ESPNOW   0   // ESP-NOW (dahili) — config.h düzenle
#define ENABLE_PASS     0   // Şifre yöneticisi (yazılım)
#define ENABLE_OTA      0   // OTA güncelleme
// Modül gelince 1 yap:
#define ENABLE_NFC      0   // PN532 NFC/RFID (I2C)
#define ENABLE_SUBGHZ   0   // CC1101 Sub-GHz (SPI)
#define ENABLE_LORA     0   // SX1278 LoRa (SPI)
#define ENABLE_SD       0   // SD Kart (SPI)

// --- Eksik tanımlar (bazı Adafruit_ST7735 sürümlerinde yok) ---
#ifndef ST77XX_GREY
#define ST77XX_GREY  0x7BEF
#endif
#ifndef ST77XX_CYAN
#define ST77XX_CYAN  0x07FF
#endif

// --- Sınırlar ---
#define MAX_REMOTES   20
#define MAX_BTNS      16
#define MAX_PASS      20
#define MAX_NAME      20
#define MAX_BNAME     14
#define MAX_PLEN      32

// ═══════════════════════════════════════════════════
//  GLOBALS
// ═══════════════════════════════════════════════════

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Preferences prefs;

#if ENABLE_BT
HijelHID_BLEKeyboard bleKeyboard("pixiZ Remote", "pixiZ", 100);
#endif

struct IRBtn { char name[MAX_BNAME]; uint16_t addr, cmd; uint8_t proto; };
struct Remote { char name[MAX_NAME]; int bc; IRBtn btns[MAX_BTNS]; };
Remote rmt[MAX_REMOTES];
int rmtN = 0;

#if ENABLE_PASS
struct PEntry { char svc[MAX_NAME], user[MAX_NAME], pass[MAX_PLEN]; };
PEntry passes[MAX_PASS];
int passN = 0;
#endif

// UI state
int pg = 0; // 0=main 1=ir_list 2=ir_btns 3=learn 4=send
            // 5=bt 6=wifi 7=espnow 8=pass 9=about 10=tools
int sel = 0, sel2 = 0, sel3 = 0;
bool lrn = false;
int lrnSt = 0, lrnRmt = -1;
IRData irRes;
char tName[MAX_BNAME];

unsigned long lastBtn = 0;
const int DB = 200;

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
      sprintf(k, "r%dbn", r, b); String bn = prefs.getString(k, "Btn");
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

#if ENABLE_PASS
void ldPass() {
  passN = prefs.getInt("pc", 0);
  if (passN > MAX_PASS) passN = MAX_PASS;
  for (int i = 0; i < passN; i++) {
    char k[16];
    sprintf(k, "ps%d", i); prefs.getString(k, "", passes[i].svc, MAX_NAME);
    sprintf(k, "pu%d", i); prefs.getString(k, "", passes[i].user, MAX_NAME);
    sprintf(k, "pp%d", i); prefs.getString(k, "", passes[i].pass, MAX_PLEN);
  }
}
void svPass() {
  prefs.putInt("pc", passN);
  for (int i = 0; i < passN; i++) {
    char k[16];
    sprintf(k, "ps%d", i); prefs.putString(k, passes[i].svc);
    sprintf(k, "pu%d", i); prefs.putString(k, passes[i].user);
    sprintf(k, "pp%d", i); prefs.putString(k, passes[i].pass);
  }
}
void clrPass() { passN = 0; prefs.putInt("pc", 0); }
#endif

// ═══════════════════════════════════════════════════
//  SPLASH
// ═══════════════════════════════════════════════════

void splash() {
  tft.fillScreen(ST77XX_BLACK);
  tft.fillRoundRect(20, 16, 120, 96, 8, ST77XX_BLUE);
  tft.setTextSize(2); tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(32, 30); tft.print("pixiZ");
  tft.setTextSize(1); tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(28, 56); tft.print("Universal");
  tft.setCursor(24, 72); tft.print("Remote v3");
  tft.setTextColor(ST77XX_GREY);
  tft.setCursor(12, 100); tft.print("ESP32 DevKit V4");
}

// ═══════════════════════════════════════════════════
//  SETUP
// ═══════════════════════════════════════════════════

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
#if ENABLE_PASS
  ldPass();
#endif
#if ENABLE_BT
  bleKeyboard.begin();
#endif

  splash(); delay(1500);
  pg = 0; sel = 0;
  drawMain();
}

// ═══════════════════════════════════════════════════
//  MAIN MENU
// ═══════════════════════════════════════════════════

const char* mn[] = {
  "IR Remote",
#if ENABLE_BT
  "Bluetooth KB",
#endif
#if ENABLE_WIFI
  "WiFi Tools",
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
  pg = 0; hdr("pixiZ v3", ST77XX_BLUE);
  for (int i = 0; i < MNC; i++) mi(i, MNC, 28, 28, ST77XX_CYAN, sel, mn[i]);
  ftr("UP/DOWN  OK=Sel  MENU=Tools");
}

void mainUp() { int o=sel; sel=(sel-1+MNC)%MNC; mi(o,MNC,28,28,ST77XX_CYAN,sel,mn[o]); mi(sel,MNC,28,28,ST77XX_CYAN,sel,mn[sel]); }
void mainDown() { int o=sel; sel=(sel+1)%MNC; mi(o,MNC,28,28,ST77XX_CYAN,sel,mn[o]); mi(sel,MNC,28,28,ST77XX_CYAN,sel,mn[sel]); }
void mainOk() {
  int i=0;
  if (sel==i) { sel2=0; pg=1; drawIRList(); return; }
#if ENABLE_BT
  if (sel==++i) { pg=5; drawBT(); return; }
#endif
#if ENABLE_WIFI
  if (sel==++i) { pg=6; drawWiFi(); return; }
#endif
#if ENABLE_ESPNOW
  if (sel==++i) { pg=7; drawESPNOW(); return; }
#endif
#if ENABLE_PASS
  if (sel==++i) { pg=8; drawPassList(); return; }
#endif
  if (sel==++i) { sel3=0; pg=10; drawTools(); }
}

// ═══════════════════════════════════════════════════
//  IR MODULE
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
      default: tft.print(irRes.protocol);
    }
    tft.setCursor(8, 86); tft.print("Addr:0x"); tft.print(irRes.address, HEX);
    tft.setCursor(8, 100); tft.print("Cmd: 0x"); tft.print(irRes.command, HEX);
  }
  ftr("OK=Learn/Save  MENU=Cancel");
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
    char buf[MAX_NAME]; sprintf(buf, "Rmt_%d", n);
    bool f = false;
    for (int i = 0; i < rmtN; i++) if (strcmp(rmt[i].name, buf) == 0) { f=true; n++; break; }
    if (!f) {
      strcpy(rmt[rmtN].name, buf); rmt[rmtN].bc = 0;
      rmtN++; svIR(); sel2 = rmtN-1; sel3 = 0;
      lrnRmt = sel2; lrn = true; lrnSt = 0;
      int m = 1;
      while (true) {
        char b2[MAX_BNAME]; sprintf(b2, "Btn_%d", m);
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
    char buf[MAX_BNAME]; sprintf(buf, "Btn_%d", n);
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
  rmt[r].btns[b].proto = (uint8_t)irRes.protocol;
  rmt[r].bc++; svIR(); sel3 = b; lrn = false;
  tft.fillRect(0, 50, 160, 30, ST77XX_BLACK);
  tft.setTextColor(ST77XX_GREEN); tft.setCursor(8, 64); tft.print("Saved!");
  delay(600);
}

void IRloop() {
  if (lrn && lrnSt == 0 && IrReceiver.decode()) {
    irRes = IrReceiver.decodedIRData; IrReceiver.resume();
    lrnSt = 1; drawLearn();
  }
}

void actIR() {
  if (pg == 1) {
    if (rmtN == 0) { if (ok()) irAdd(); else if (menu()) { pg=0; drawMain(); } return; }
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

// ═══════════════════════════════════════════════════
//  BT MODULE
// ═══════════════════════════════════════════════════

void drawBT() {
  pg=5; hdr("BLUETOOTH KB", ST77XX_MAGENTA);
  tft.setTextColor(ST77XX_WHITE); tft.setCursor(8, 32); tft.print("Status: ");
#if ENABLE_BT
  bool okk = bleKeyboard.isConnected();
  tft.setTextColor(okk ? ST77XX_GREEN : ST77XX_YELLOW);
  tft.print(okk ? "CONNECTED" : "WAITING...");
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(8, 52); tft.print("Device: pixiZ Remote");
  tft.setTextColor(ST77XX_GREY);
  tft.setCursor(8, 72); tft.print("Pair from your device's");
  tft.setCursor(8, 86); tft.print("Bluetooth settings.");
  tft.setCursor(8, 100); tft.print("Name: pixiZ Remote");
#else
  tft.setTextColor(ST77XX_GREY); tft.print("DISABLED");
#endif
  ftr("MENU=Back");
}

void actBT() {
  if (menu()) { pg=0; drawMain(); }
}

// ═══════════════════════════════════════════════════
//  WIFI MODULE
// ═══════════════════════════════════════════════════

char wifiSSIDs[10][33];
int wifiN = 0;
unsigned long wifiScanStart = 0;
bool wifiScanning = false;

void drawWiFi() {
  pg=6; hdr("WIFI TOOLS", ST77XX_CYAN);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(8, 30); tft.print("[1] Scan Networks");
  tft.setCursor(8, 50); tft.print("[2] Network Info");
  tft.setCursor(8, 70); tft.print("[3] Packet Monitor");
  tft.setTextColor(ST77XX_GREY);
  tft.setCursor(8, 96); tft.print("WiFi: ");
  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(ST77XX_GREEN); tft.print("Connected");
  } else {
    tft.setTextColor(ST77XX_RED); tft.print("Disconnected");
  }
  ftr("MENU=Back");
}

void drawWiFiScan() {
  tft.fillScreen(ST77XX_BLACK);
  tft.fillRect(0, 0, 160, 20, ST77XX_CYAN);
  tft.drawFastHLine(0, 20, 160, 0x3A9F);
  tft.setTextSize(1); tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(44, 5); tft.print("WIFI SCAN");

  if (wifiScanning) {
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(8, 50); tft.print("Scanning...");
    return;
  }

  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(8, 30); tft.printf("Found %d networks:", wifiN);
  for (int i = 0; i < wifiN && i < 4; i++) {
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(8, 46+i*18); tft.print(wifiSSIDs[i]);
  }
  ftr("MENU=Back");
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
  wifiScanning = false;
  drawWiFiScan();
}

void actWiFi() {
  if (menu()) { pg=0; drawMain(); }
  else if (ok()) { doWiFiScan(); }
}

// ═══════════════════════════════════════════════════
//  ESP-NOW MODULE (hazır)
// ═══════════════════════════════════════════════════

#if ENABLE_ESPNOW
void drawESPNOW() {
  pg=7; hdr("ESP-NOW", ST77XX_YELLOW);
  tft.setTextColor(ST77XX_GREY);
  tft.setCursor(8, 40); tft.print("ESP-NOW ready");
  tft.setCursor(8, 56); tft.print("Configure peers in");
  tft.setCursor(8, 70); tft.print("config.h when");
  tft.setCursor(8, 84); tft.print("modules are ready.");
  ftr("MENU=Back");
}
void actESPNOW() { if (menu()) { pg=0; drawMain(); } }
#endif

// ═══════════════════════════════════════════════════
//  PASSWORD MODULE (hazır)
// ═══════════════════════════════════════════════════

#if ENABLE_PASS
void drawPassList() {
  pg=8; hdr("PASSWORDS", ST77XX_CYAN);
  if (passN == 0) {
    tft.setTextColor(ST77XX_GREY);
    tft.setCursor(8, 50); tft.print("No passwords saved");
    ftr("MENU=Back"); return;
  }
  for (int i = 0; i < passN && i < 5; i++) {
    int y = 24+i*19; bool s = i==sel2;
    tft.fillRect(2, y, 156, 17, s ? ST77XX_CYAN : ST77XX_BLACK);
    if (s) tft.drawRoundRect(2, y, 156, 17, 3, ST77XX_CYAN);
    tft.setTextColor(s ? ST77XX_BLACK : ST77XX_WHITE);
    tft.setCursor(8, y+4); tft.print(passes[i].svc);
  }
  ftr("OK=View  MENU=Back");
}
void actPass() {
  if (pg == 8) {
    if (menu()) { pg=0; drawMain(); }
    else if (ok() && passN > 0) {
      hdr(passes[sel2].svc, ST77XX_CYAN);
      tft.setTextColor(ST77XX_GREY);
      tft.setCursor(8, 30); tft.print("User: ");
      tft.setTextColor(ST77XX_WHITE);
      tft.print(passes[sel2].user);
      tft.setCursor(8, 50); tft.setTextColor(ST77XX_GREY);
      tft.print("Pass: ");
      tft.setTextColor(ST77XX_WHITE);
      tft.print(passes[sel2].pass);
      tft.setCursor(8, 70); tft.setTextColor(ST77XX_GREEN);
      tft.print("Send via BT (future)");
      ftr("MENU=Back");
      pg = 9;
    }
    else if (up() && passN > 0) { int o=sel2; sel2=(sel2-1+passN)%passN; /* redraw items */ drawPassList(); }
    else if (down() && passN > 0) { int o=sel2; sel2=(sel2+1)%passN; drawPassList(); }
  } else if (pg == 9) {
    if (menu()) { pg=8; drawPassList(); }
  }
}
#endif

// ═══════════════════════════════════════════════════
//  TOOLS
// ═══════════════════════════════════════════════════

const char* tl[] = { "About", "Clear IR Data", "Restart" };
const int TLC = 3;

void drawTools() {
  pg=10; hdr("TOOLS", ST77XX_CYAN);
  for (int i = 0; i < TLC; i++) mi(i, TLC, 28, 28, ST77XX_CYAN, sel3, tl[i]);
  ftr("UP/DOWN  OK=Sel  MENU=Back");
}

void toolsUp() { int o=sel3; sel3=(sel3-1+TLC)%TLC; mi(o,TLC,28,28,ST77XX_CYAN,sel3,tl[o]); mi(sel3,TLC,28,28,ST77XX_CYAN,sel3,tl[sel3]); }
void toolsDown() { int o=sel3; sel3=(sel3+1)%TLC; mi(o,TLC,28,28,ST77XX_CYAN,sel3,tl[o]); mi(sel3,TLC,28,28,ST77XX_CYAN,sel3,tl[sel3]); }
void toolsOk() {
  switch (sel3) {
    case 0: // About
      hdr("ABOUT pixiZ", ST77XX_BLUE);
      tft.setTextColor(ST77XX_CYAN); tft.setCursor(8, 30); tft.print("pixiZ Universal Remote");
      tft.setTextColor(ST77XX_WHITE); tft.setCursor(8, 46); tft.print("Version 3.0");
      tft.setCursor(8, 62); tft.print("PolyCast5 + Flipper");
      tft.setTextColor(ST77XX_GREY); tft.setCursor(8, 82); tft.print("IR + BT + WiFi + ESP-NOW");
      tft.setCursor(8, 98); tft.print("github.com/azerenes/pixiZ-v1");
      ftr("MENU=Back");
      pg = 11; break;
    case 1:
      clrIR(); tft.setTextColor(ST77XX_GREEN);
      tft.setCursor(8, 80); tft.print("All IR data cleared!");
      delay(1000); drawTools(); break;
    case 2: ESP.restart(); break;
  }
}

// ═══════════════════════════════════════════════════
//  BUTTON HELPERS
// ═══════════════════════════════════════════════════

bool up() { return digitalRead(BTN_UP) == LOW; }
bool down() { return digitalRead(BTN_DOWN) == LOW; }
bool ok() { return digitalRead(BTN_OK) == LOW; }
bool menu() { return digitalRead(BTN_MENU) == LOW; }

// ═══════════════════════════════════════════════════
//  LOOP
// ═══════════════════════════════════════════════════

void loop() {
  unsigned long now = millis();
  IRloop();

  if ((up()||down()||ok()||menu()) && now-lastBtn>DB) {
    lastBtn = now;

    switch (pg) {
      case 0:  // main
        if (up()) mainUp();
        else if (down()) mainDown();
        else if (ok()) mainOk();
        else if (menu()) { sel3=0; drawTools(); }
        break;
      case 1: case 2: case 3: case 4: actIR(); break;
      case 5: actBT(); break;
      case 6: actWiFi(); break;
#if ENABLE_ESPNOW
      case 7: actESPNOW(); break;
#endif
#if ENABLE_PASS
      case 8: case 9: actPass(); break;
#endif
      case 10: // tools
        if (up()) toolsUp();
        else if (down()) toolsDown();
        else if (ok()) toolsOk();
        else if (menu()) { pg=0; drawMain(); }
        break;
      case 11: // about
        if (menu()) { pg=10; drawTools(); }
        break;
    }
  }
}
