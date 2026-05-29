#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <IRremote.h>
#include <Preferences.h>
#include <BleKeyboard.h>

#define TFT_CS    5
#define TFT_RST   16
#define TFT_DC    17
#define TFT_MOSI  23
#define TFT_SCLK  18

#define BTN_UP    32
#define BTN_DOWN  33
#define BTN_OK    14
#define BTN_MENU  12

#define IR_RECV   13
#define IR_SEND   27

#define ST77XX_GREY   0x7BEF
#define ST77XX_DGREY  0x39E7

#define MAX_REMOTES 50
#define MAX_NAME_LEN 24

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
Preferences prefs;
BleKeyboard bleKeyboard("pixiZ Remote", "pixiZ", 100);

struct IRCode {
  char name[MAX_NAME_LEN];
  uint16_t address;
  uint16_t command;
  uint8_t protocol;
};

IRCode irCodes[MAX_REMOTES];
int irCount = 0;
int menuSel = 0;

enum Screen { SCR_MAIN, SCR_IR_LIST, SCR_IR_LEARN, SCR_IR_SEND, SCR_BT, SCR_SETTINGS };
Screen screen = SCR_MAIN;

int subSel = 0;
bool learning = false;
int learnState = 0;
IRData irResult;
char learnName[MAX_NAME_LEN];

unsigned long lastBtn = 0;
const int DEBOUNCE = 200;

const char* mainItems[] = {"IR Remote", "Bluetooth KB", "Settings"};
const int MAIN_COUNT = 3;

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

  prefs.begin("pixiZ", false);
  loadIR();

  bleKeyboard.begin();

  splash();
  delay(1500);
  drawMain();
}

void splash() {
  tft.fillScreen(ST77XX_BLACK);
  tft.fillRoundRect(20, 16, 120, 96, 8, ST77XX_BLUE);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(32, 30);
  tft.print("pixiZ");
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(28, 56);
  tft.print("Universal");
  tft.setCursor(20, 72);
  tft.print("Remote v1");
  tft.setTextColor(ST77XX_GREY);
  tft.setCursor(12, 100);
  tft.print("ESP32 DevKit V4");
}

void drawHeader(const char* title, uint16_t color) {
  tft.fillScreen(ST77XX_BLACK);
  tft.fillRect(0, 0, 160, 20, color);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  int tw = strlen(title) * 6;
  tft.setCursor((160 - tw) / 2, 5);
  tft.print(title);
  tft.drawFastHLine(0, 20, 160, color >> 1 & 0x7BEF);
}

void drawMain() {
  screen = SCR_MAIN;
  drawHeader("pixiZ REMOTE", ST77XX_BLUE);

  for (int i = 0; i < MAIN_COUNT; i++) {
    int y = 28 + i * 30;
    if (i == menuSel) {
      tft.fillRoundRect(6, y, 148, 24, 4, ST77XX_CYAN);
      tft.setTextColor(ST77XX_BLACK);
    } else {
      tft.drawRoundRect(6, y, 148, 24, 4, ST77XX_GREY);
      tft.setTextColor(ST77XX_WHITE);
    }
    tft.setTextSize(1);
    tft.setCursor(16, y + 7);
    tft.print(mainItems[i]);
  }
  footer("UP/DOWN  OK=Sec  MENU=BT");
}

void footer(const char* text) {
  tft.fillRect(0, 118, 160, 10, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_GREY);
  tft.setCursor(2, 119);
  tft.print(text);
}

void drawIRList() {
  screen = SCR_IR_LIST;
  drawHeader("IR REMOTES", ST77XX_GREEN);

  if (irCount == 0) {
    tft.setTextColor(ST77XX_GREY);
    tft.setCursor(20, 50);
    tft.print("No remotes saved.");
    tft.setCursor(12, 66);
    tft.print("Menu to learn new");
    footer("OK=Learn  MENU=Back");
    return;
  }

  int start = menuSel > 4 ? menuSel - 4 : 0;
  int end = min(start + 5, irCount);
  for (int i = start; i < end; i++) {
    int y = 24 + (i - start) * 19;
    if (i == menuSel) {
      tft.fillRoundRect(2, y, 156, 17, 3, ST77XX_GREEN);
      tft.setTextColor(ST77XX_BLACK);
    } else {
      tft.setTextColor(ST77XX_WHITE);
    }
    tft.setCursor(8, y + 4);
    tft.print(irCodes[i].name);
  }
  footer("UP/DOWN  OK=Send  MENU=Back");
}

void drawIRLearn() {
  screen = SCR_IR_LEARN;
  drawHeader("LEARN IR", ST77XX_RED);

  tft.setCursor(8, 30);
  tft.setTextColor(ST77XX_CYAN);
  tft.print("Name: ");
  tft.setTextColor(ST77XX_WHITE);
  tft.print(learnName);

  tft.drawFastHLine(8, 46, 144, ST77XX_GREY);

  if (learnState == 0) {
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(8, 56);
    tft.print("Point remote at");
    tft.setCursor(8, 70);
    tft.print("receiver and press");
    tft.setCursor(8, 84);
    tft.print("a button...");
    tft.fillRoundRect(44, 98, 72, 16, 3, ST77XX_RED);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(60, 102);
    tft.print("WAITING");
  } else {
    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(8, 56);
    tft.print("Signal received!");
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(8, 72);
    tft.print("Protocol: ");
    switch (irResult.protocol) {
      case NEC: tft.print("NEC"); break;
      case SONY: tft.print("SONY"); break;
      case RC5: tft.print("RC5"); break;
      case RC6: tft.print("RC6"); break;
      case SAMSUNG: tft.print("SAMSUNG"); break;
      default: tft.print(irResult.protocol);
    }
    tft.setCursor(8, 86);
    tft.print("Addr: 0x");
    tft.print(irResult.address, HEX);
    tft.setCursor(8, 100);
    tft.print("Cmd:  0x");
    tft.print(irResult.command, HEX);
  }
  footer("OK=Learn  MENU=Cancel");
}

void drawIRSend() {
  screen = SCR_IR_SEND;
  drawHeader("SENDING", ST77XX_GREEN);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(8, 40);
  tft.print("Remote: ");
  tft.setTextColor(ST77XX_WHITE);
  tft.print(irCodes[menuSel].name);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(8, 60);
  tft.print("Sending IR signal...");

  for (int i = 0; i < 3; i++) {
    IrSender.sendNEC(irCodes[menuSel].address, irCodes[menuSel].command, 0);
    delay(50);
  }

  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(8, 80);
  tft.print("Done!");
  delay(800);
  drawIRList();
}

void drawBT() {
  screen = SCR_BT;
  drawHeader("BLUETOOTH KB", ST77XX_MAGENTA);

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(8, 32);
  tft.print("Status: ");
  if (bleKeyboard.isConnected()) {
    tft.setTextColor(ST77XX_GREEN);
    tft.print("CONNECTED");
  } else {
    tft.setTextColor(ST77XX_YELLOW);
    tft.print("WAITING...");
  }

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(8, 52);
  tft.print("Device: pixiZ Remote");
  tft.setTextColor(ST77XX_GREY);
  tft.setCursor(8, 72);
  tft.print("Pair from your");
  tft.setCursor(8, 86);
  tft.print("computer/phone");
  tft.setCursor(8, 100);
  tft.print("Bluetooth settings.");

  footer("MENU=Back");
}

void drawSettings() {
  screen = SCR_SETTINGS;
  drawHeader("SETTINGS", ST77XX_CYAN);

  const char* items[] = {"WiFi Setup", "About", "Clear IR Codes", "Restart Device"};

  for (int i = 0; i < 4; i++) {
    int y = 28 + i * 22;
    if (i == subSel) {
      tft.fillRoundRect(4, y, 152, 18, 3, ST77XX_CYAN);
      tft.setTextColor(ST77XX_BLACK);
    } else {
      tft.setTextColor(ST77XX_WHITE);
    }
    tft.setCursor(12, y + 4);
    tft.print(items[i]);
  }
  footer("UP/DOWN  OK=Sec  MENU=Back");
}

void drawAbout() {
  drawHeader("ABOUT pixiZ", ST77XX_BLUE);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(8, 30);
  tft.print("pixiZ Universal Remote");
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(8, 46);
  tft.print("Version 1.0");
  tft.setCursor(8, 62);
  tft.print("ESP32 DevKit V4");
  tft.setTextColor(ST77XX_GREY);
  tft.setCursor(8, 82);
  tft.print("IR + Bluetooth + WiFi");
  tft.setCursor(8, 98);
  tft.print("github.com/azerenes/pixiZ-v1");
  footer("MENU=Back");
}

void loadIR() {
  irCount = prefs.getInt("ircount", 0);
  if (irCount > MAX_REMOTES) irCount = MAX_REMOTES;
  for (int i = 0; i < irCount; i++) {
    char key[16];
    sprintf(key, "irname_%d", i);
    String name = prefs.getString(key, "Unknown");
    strncpy(irCodes[i].name, name.c_str(), MAX_NAME_LEN - 1);
    irCodes[i].name[MAX_NAME_LEN - 1] = 0;
    sprintf(key, "iraddr_%d", i);
    irCodes[i].address = prefs.getUShort(key, 0);
    sprintf(key, "ircmd_%d", i);
    irCodes[i].command = prefs.getUShort(key, 0);
    sprintf(key, "irproto_%d", i);
    irCodes[i].protocol = prefs.getUChar(key, 0);
  }
}

void saveIR() {
  prefs.putInt("ircount", irCount);
  for (int i = 0; i < irCount; i++) {
    char key[16];
    sprintf(key, "irname_%d", i);
    prefs.putString(key, irCodes[i].name);
    sprintf(key, "iraddr_%d", i);
    prefs.putUShort(key, irCodes[i].address);
    sprintf(key, "ircmd_%d", i);
    prefs.putUShort(key, irCodes[i].command);
    sprintf(key, "irproto_%d", i);
    prefs.putUChar(key, irCodes[i].protocol);
  }
}

void saveCurrentIR() {
  if (irCount >= MAX_REMOTES) return;
  int idx = irCount;
  strcpy(irCodes[idx].name, learnName);
  irCodes[idx].address = irResult.address;
  irCodes[idx].command = irResult.command;
  irCodes[idx].protocol = irResult.protocol;
  irCount++;
  saveIR();
}

void clearAllIR() {
  irCount = 0;
  prefs.putInt("ircount", 0);
  prefs.end();
  prefs.begin("pixiZ", false);
}

void startLearn() {
  learnState = 0;
  learning = true;
  int n = 1;
  char buf[MAX_NAME_LEN];
  bool found;
  do {
    found = false;
    sprintf(buf, "Remote_%d", n);
    for (int i = 0; i < irCount; i++) {
      if (strcmp(irCodes[i].name, buf) == 0) { found = true; n++; break; }
    }
  } while (found);
  strcpy(learnName, buf);
  drawIRLearn();
}

void loop() {
  unsigned long now = millis();

  bool up = digitalRead(BTN_UP) == LOW;
  bool down = digitalRead(BTN_DOWN) == LOW;
  bool ok = digitalRead(BTN_OK) == LOW;
  bool menu = digitalRead(BTN_MENU) == LOW;

  if (learning && learnState == 0) {
    if (IrReceiver.decode()) {
      irResult = IrReceiver.decodedIRData;
      IrReceiver.resume();
      learnState = 1;
      drawIRLearn();
    }
  }

  if ((up || down || ok || menu) && now - lastBtn > DEBOUNCE) {
    lastBtn = now;

    switch (screen) {
      case SCR_MAIN:
        if (up) { menuSel = (menuSel - 1 + MAIN_COUNT) % MAIN_COUNT; drawMain(); }
        else if (down) { menuSel = (menuSel + 1) % MAIN_COUNT; drawMain(); }
        else if (ok) {
          switch (menuSel) {
            case 0: menuSel = 0; drawIRList(); break;
            case 1: drawBT(); break;
            case 2: subSel = 0; drawSettings(); break;
          }
        }
        else if (menu) { drawBT(); }
        break;

      case SCR_IR_LIST:
        if (up && irCount > 0) { menuSel = (menuSel - 1 + irCount) % irCount; drawIRList(); }
        else if (down && irCount > 0) { menuSel = (menuSel + 1) % irCount; drawIRList(); }
        else if (ok) { if (irCount > 0) drawIRSend(); else startLearn(); }
        else if (menu) { menuSel = 0; drawMain(); }
        break;

      case SCR_IR_LEARN:
        if (ok) {
          if (learnState == 0) {
            drawIRLearn();
          } else if (learnState == 1) {
            saveCurrentIR();
            learning = false;
            tft.fillRect(0, 50, 160, 30, ST77XX_BLACK);
            tft.setTextColor(ST77XX_GREEN);
            tft.setCursor(8, 64);
            tft.print("Saved!");
            delay(800);
            menuSel = irCount - 1;
            drawIRList();
          }
        }
        else if (menu) { learning = false; learnState = 0; drawIRList(); }
        break;

      case SCR_IR_SEND:
        break;

      case SCR_BT:
        if (menu) { drawMain(); }
        break;

      case SCR_SETTINGS:
        if (up) { subSel = (subSel - 1 + 4) % 4; drawSettings(); }
        else if (down) { subSel = (subSel + 1) % 4; drawSettings(); }
        else if (ok) {
          switch (subSel) {
            case 1: drawAbout(); break;
            case 2:
              clearAllIR();
              tft.setTextColor(ST77XX_GREEN);
              tft.setCursor(8, 80);
              tft.print("All IR codes cleared!");
              delay(1000);
              drawSettings();
              break;
            case 3:
              ESP.restart();
              break;
          }
        }
        else if (menu) { drawMain(); }
        break;
    }
  }

  delay(10);
}
