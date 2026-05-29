#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <IRremote.h>
#include <Preferences.h>
#include <HijelHID_BLEKeyboard.h>

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

#define MAX_REMOTES 20
#define MAX_BTNS    16
#define MAX_NAME    20
#define MAX_BNAME   14

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
Preferences prefs;
HijelHID_BLEKeyboard bleKeyboard("pixiZ Remote", "pixiZ", 100);

struct IRButton {
  char name[MAX_BNAME];
  uint16_t address;
  uint16_t command;
  uint8_t protocol;
};

struct Remote {
  char name[MAX_NAME];
  int btnCount;
  IRButton buttons[MAX_BTNS];
};

Remote remotes[MAX_REMOTES];
int remoteCount = 0;
int selRemote = 0;
int selButton = 0;

enum Screen { SCR_MAIN, SCR_RMT_LIST, SCR_BTN_LIST, SCR_LEARN, SCR_SEND, SCR_BT, SCR_SETTINGS };
Screen screen = SCR_MAIN;
int subSel = 0;

bool learning = false;
int learnState = 0;
int learnTargetRemote = -1;
IRData irResult;
char tmpName[MAX_BNAME];
int tmpNameIdx = 0;

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
  loadAll();

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
  tft.print("Remote v2");
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

void footer(const char* text) {
  tft.fillRect(0, 118, 160, 10, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_GREY);
  tft.setCursor(2, 119);
  tft.print(text);
}

void drawMain() {
  screen = SCR_MAIN;
  drawHeader("pixiZ REMOTE", ST77XX_BLUE);
  for (int i = 0; i < MAIN_COUNT; i++) {
    int y = 28 + i * 30;
    if (i == selRemote) {
      tft.fillRoundRect(6, y, 148, 24, 4, ST77XX_CYAN);
      tft.setTextColor(ST77XX_BLACK);
    } else {
      tft.drawRoundRect(6, y, 148, 24, 4, ST77XX_GREY);
      tft.setTextColor(ST77XX_WHITE);
    }
    tft.setCursor(16, y + 7);
    tft.print(mainItems[i]);
  }
  footer("UP/DOWN  OK=Sec  MENU=BT");
}

void drawRemoteList() {
  screen = SCR_RMT_LIST;
  drawHeader("REMOTES", ST77XX_GREEN);
  if (remoteCount == 0) {
    tft.setTextColor(ST77XX_GREY);
    tft.setCursor(20, 50);
    tft.print("No remotes yet.");
    tft.setCursor(8, 66);
    tft.print("Press OK to add one");
    footer("OK=Add  MENU=Back");
    return;
  }
  int start = selRemote > 4 ? selRemote - 4 : 0;
  int end = min(start + 5, remoteCount);
  for (int i = start; i < end; i++) {
    int y = 24 + (i - start) * 19;
    if (i == selRemote) {
      tft.fillRoundRect(2, y, 156, 17, 3, ST77XX_GREEN);
      tft.setTextColor(ST77XX_BLACK);
    } else {
      tft.setTextColor(ST77XX_WHITE);
    }
    tft.setCursor(8, y + 4);
    tft.print(remotes[i].name);
    tft.setTextColor(ST77XX_GREY);
    tft.setCursor(120, y + 4);
    tft.printf("%d btn", remotes[i].btnCount);
  }
  footer("OK=Open  MENU=Back");
}

void drawButtonList() {
  screen = SCR_BTN_LIST;
  drawHeader(remotes[selRemote].name, ST77XX_GREEN);
  int cnt = remotes[selRemote].btnCount;
  int total = cnt + (cnt < MAX_BTNS ? 1 : 0);
  if (cnt == 0) {
    tft.setTextColor(ST77XX_GREY);
    tft.setCursor(12, 50);
    tft.print("No buttons yet.");
    tft.setCursor(8, 66);
    tft.print("Press OK to learn");
    footer("OK=Learn  MENU=Back");
    return;
  }
  int start = selButton > 4 ? selButton - 4 : 0;
  int end = min(start + 5, total);
  for (int i = start; i < end; i++) {
    int y = 24 + (i - start) * 19;
    if (i == selButton) {
      tft.fillRoundRect(2, y, 156, 17, 3, ST77XX_CYAN);
      tft.setTextColor(ST77XX_BLACK);
    } else {
      tft.setTextColor(ST77XX_WHITE);
    }
    if (i < cnt) {
      tft.setCursor(8, y + 4);
      tft.print(remotes[selRemote].buttons[i].name);
    } else {
      tft.setTextColor(ST77XX_GREEN);
      tft.setCursor(8, y + 4);
      tft.print("+ Add Button");
    }
  }
  footer("OK=Send/Add  MENU=Back");
}

void drawLearn() {
  screen = SCR_LEARN;
  drawHeader("LEARN IR", ST77XX_RED);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(8, 30);
  tft.print("Remote: ");
  tft.setTextColor(ST77XX_WHITE);
  tft.print(remotes[learnTargetRemote].name);
  tft.drawFastHLine(8, 46, 144, ST77XX_GREY);
  if (learnState == 0) {
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(8, 56);
    tft.print("Point & press a");
    tft.setCursor(8, 70);
    tft.print("button on remote");
    tft.setTextColor(ST77XX_GREY);
    tft.setCursor(8, 90);
    tft.print("Button name:");
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(8, 104);
    tft.print(tmpName);
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
    tft.print("Proto: ");
    switch (irResult.protocol) {
      case NEC: tft.print("NEC"); break;
      case SONY: tft.print("SONY"); break;
      case RC5: tft.print("RC5"); break;
      case RC6: tft.print("RC6"); break;
      case SAMSUNG: tft.print("SAMSUNG"); break;
      default: tft.print(irResult.protocol);
    }
    tft.setCursor(8, 86);
    tft.print("Addr:0x");
    tft.print(irResult.address, HEX);
    tft.setCursor(8, 100);
    tft.print("Cmd: 0x");
    tft.print(irResult.command, HEX);
  }
  footer("OK=Learn/Save  MENU=Cancel");
}

void drawSend() {
  screen = SCR_SEND;
  drawHeader("SENDING", ST77XX_GREEN);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(8, 40);
  tft.print("Remote: ");
  tft.setTextColor(ST77XX_WHITE);
  tft.print(remotes[selRemote].name);
  tft.setCursor(8, 56);
  tft.setTextColor(ST77XX_CYAN);
  tft.print("Button: ");
  tft.setTextColor(ST77XX_WHITE);
  tft.print(remotes[selRemote].buttons[selButton].name);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(8, 78);
  tft.print("Sending IR...");
  IRButton* b = &remotes[selRemote].buttons[selButton];
  for (int i = 0; i < 3; i++) {
    IrSender.sendNEC(b->address, b->command, 0);
    delay(50);
  }
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(8, 96);
  tft.print("Done!");
  delay(600);
  drawButtonList();
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
  tft.print("Pair from Bluetooth");
  tft.setCursor(8, 86);
  tft.print("settings on your");
  tft.setCursor(8, 100);
  tft.print("computer/phone.");
  footer("MENU=Back");
}

void drawSettings() {
  screen = SCR_SETTINGS;
  drawHeader("SETTINGS", ST77XX_CYAN);
  const char* items[] = {"WiFi Setup", "About", "Clear All", "Restart"};
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
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(8, 30);
  tft.print("pixiZ Universal Remote");
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(8, 46);
  tft.print("Version 2.0");
  tft.setCursor(8, 62);
  tft.print("Multi-button remotes");
  tft.setTextColor(ST77XX_GREY);
  tft.setCursor(8, 82);
  tft.print("IR + Bluetooth + WiFi");
  tft.setCursor(8, 98);
  tft.print("github.com/azerenes/pixiZ-v1");
  footer("MENU=Back");
}

void loadAll() {
  remoteCount = prefs.getInt("rmcnt", 0);
  if (remoteCount > MAX_REMOTES) remoteCount = MAX_REMOTES;
  for (int r = 0; r < remoteCount; r++) {
    char key[20];
    sprintf(key, "rmname_%d", r);
    String name = prefs.getString(key, "Remote");
    strncpy(remotes[r].name, name.c_str(), MAX_NAME - 1);
    remotes[r].name[MAX_NAME - 1] = 0;
    sprintf(key, "rmbtn_%d", r);
    remotes[r].btnCount = prefs.getInt(key, 0);
    if (remotes[r].btnCount > MAX_BTNS) remotes[r].btnCount = MAX_BTNS;
    for (int b = 0; b < remotes[r].btnCount; b++) {
      sprintf(key, "r%db%dname", r, b);
      String bname = prefs.getString(key, "Btn");
      strncpy(remotes[r].buttons[b].name, bname.c_str(), MAX_BNAME - 1);
      remotes[r].buttons[b].name[MAX_BNAME - 1] = 0;
      sprintf(key, "r%db%daddr", r, b);
      remotes[r].buttons[b].address = prefs.getUShort(key, 0);
      sprintf(key, "r%db%dcmd", r, b);
      remotes[r].buttons[b].command = prefs.getUShort(key, 0);
      sprintf(key, "r%db%dproto", r, b);
      remotes[r].buttons[b].protocol = prefs.getUChar(key, 0);
    }
  }
}

void saveAll() {
  prefs.putInt("rmcnt", remoteCount);
  for (int r = 0; r < remoteCount; r++) {
    char key[20];
    sprintf(key, "rmname_%d", r);
    prefs.putString(key, remotes[r].name);
    sprintf(key, "rmbtn_%d", r);
    prefs.putInt(key, remotes[r].btnCount);
    for (int b = 0; b < remotes[r].btnCount; b++) {
      sprintf(key, "r%db%dname", r, b);
      prefs.putString(key, remotes[r].buttons[b].name);
      sprintf(key, "r%db%daddr", r, b);
      prefs.putUShort(key, remotes[r].buttons[b].address);
      sprintf(key, "r%db%dcmd", r, b);
      prefs.putUShort(key, remotes[r].buttons[b].command);
      sprintf(key, "r%db%dproto", r, b);
      prefs.putUChar(key, remotes[r].buttons[b].protocol);
    }
  }
}

void clearAll() {
  remoteCount = 0;
  prefs.putInt("rmcnt", 0);
  prefs.end();
  prefs.begin("pixiZ", false);
}

int findRemote(const char* name) {
  for (int i = 0; i < remoteCount; i++)
    if (strcmp(remotes[i].name, name) == 0) return i;
  return -1;
}

String genRemoteName() {
  int n = 1;
  while (true) {
    char buf[MAX_NAME];
    sprintf(buf, "Remote_%d", n);
    if (findRemote(buf) < 0) return String(buf);
    n++;
  }
}

void startLearn(int rmtIdx) {
  learnTargetRemote = rmtIdx;
  learnState = 0;
  learning = true;
  int n = 1;
  while (true) {
    char buf[MAX_BNAME];
    sprintf(buf, "Btn_%d", n);
    bool found = false;
    for (int i = 0; i < remotes[rmtIdx].btnCount; i++) {
      if (strcmp(remotes[rmtIdx].buttons[i].name, buf) == 0) { found = true; n++; break; }
    }
    if (!found) { strcpy(tmpName, buf); break; }
  }
  drawLearn();
}

void addNewRemote() {
  if (remoteCount >= MAX_REMOTES) return;
  String name = genRemoteName();
  int r = remoteCount;
  strcpy(remotes[r].name, name.c_str());
  remotes[r].btnCount = 0;
  remoteCount++;
  saveAll();
  selRemote = r;
  selButton = 0;
  startLearn(r);
}

void saveLearnedBtn() {
  int r = learnTargetRemote;
  if (r < 0 || r >= remoteCount) return;
  int b = remotes[r].btnCount;
  if (b >= MAX_BTNS) return;
  strcpy(remotes[r].buttons[b].name, tmpName);
  remotes[r].buttons[b].address = irResult.address;
  remotes[r].buttons[b].command = irResult.command;
  remotes[r].buttons[b].protocol = irResult.protocol;
  remotes[r].btnCount++;
  saveAll();
  selButton = b;
  learning = false;
  tft.fillRect(0, 50, 160, 30, ST77XX_BLACK);
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(8, 64);
  tft.print("Saved!");
  delay(600);
  drawButtonList();
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
      drawLearn();
      return;
    }
  }

  if ((up || down || ok || menu) && now - lastBtn > DEBOUNCE) {
    lastBtn = now;
    switch (screen) {
      case SCR_MAIN:
        if (up) { selRemote = (selRemote - 1 + MAIN_COUNT) % MAIN_COUNT; drawMain(); }
        else if (down) { selRemote = (selRemote + 1) % MAIN_COUNT; drawMain(); }
        else if (ok) {
          switch (selRemote) {
            case 0: selRemote = 0; selButton = 0; drawRemoteList(); break;
            case 1: drawBT(); break;
            case 2: subSel = 0; drawSettings(); break;
          }
        }
        else if (menu) { drawBT(); }
        break;

      case SCR_RMT_LIST:
        if (remoteCount == 0) {
          if (ok) addNewRemote();
          else if (menu) { selRemote = 0; drawMain(); }
        } else {
          if (up) { selRemote = (selRemote - 1 + remoteCount) % remoteCount; drawRemoteList(); }
          else if (down) { selRemote = (selRemote + 1) % remoteCount; drawRemoteList(); }
          else if (ok) { selButton = 0; drawButtonList(); }
          else if (menu) { selRemote = 0; drawMain(); }
        }
        break;

      case SCR_BTN_LIST: {
        int cnt = remotes[selRemote].btnCount;
        int total = cnt + (cnt < MAX_BTNS ? 1 : 0);
        if (ok) {
          if (cnt == 0 || selButton >= cnt) { startLearn(selRemote); }
          else { drawSend(); }
        } else if (up && total > 0) { selButton = (selButton - 1 + total) % total; drawButtonList(); }
        else if (down && total > 0) { selButton = (selButton + 1) % total; drawButtonList(); }
        else if (menu) { drawRemoteList(); }
        break;
      }

      case SCR_LEARN:
        if (ok) {
          if (learnState == 0) { drawLearn(); }
          else { saveLearnedBtn(); }
        } else if (menu) {
          learning = false; learnState = 0;
          if (remotes[learnTargetRemote].btnCount > 0) drawButtonList();
          else drawRemoteList();
        }
        break;

      case SCR_SEND:
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
              clearAll();
              tft.setTextColor(ST77XX_GREEN);
              tft.setCursor(8, 80);
              tft.print("All data cleared!");
              delay(1000);
              drawSettings();
              break;
            case 3: ESP.restart(); break;
          }
        }
        else if (menu) { drawMain(); }
        break;
    }
  }
  delay(10);
}
