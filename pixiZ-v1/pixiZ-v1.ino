#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <ArduinoJson.h>
#include <base64.h>
#include <time.h>

// 1 = Modern flat design, 0 = Retro pixel design
#define MODERN_DESIGN 1

// FIX: v1 used GPIO 6,7,8,10 which are ESP32 flash pins — crash on boot.
// Changed to standard DevKit V4 safe pins:
#define TFT_LED   1   // UART TX — OK if Serial not used
#define TFT_SCLK  18  // Was GPIO 2 (flash conflict)
#define TFT_MOSI  23  // Was GPIO 4 (flash conflict)
#define TFT_DC    17  // Was GPIO 6 (FLASH PIN! crash)
#define TFT_RST   16  // Was GPIO 8 (FLASH PIN! crash)
#define TFT_CS    5   // Was GPIO 10 (FLASH PIN! crash)
#define BTN_AI    33
#define BTN_UP    32  // Was 35 — GPIO 35 has no internal pullup
#define BTN_DOWN  15  // Was 37 — GPIO 37 has no internal pullup
#define BTN_MENU  14  // Was 39 — GPIO 39 has no internal pullup
#define MIC_PIN   34  // Was GPIO 7 (FLASH PIN! crash) — now ADC1_CH6, no WiFi conflict

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

#define C_BG      0x0000
#define C_SURF    0x0841
#define C_CARD    0x1082
#define C_BORDER  0x2124
#define C_BLUE1   0x3A9F
#define C_BLUE2   0x051F
#define C_CYAN    0x07FF
#define C_GREEN   0x07E0
#define C_LIME    0x9FE0
#define C_YELLOW  0xFFE0
#define C_GOLD    0xFD20
#define C_ORANGE  0xFC00
#define C_RED     0xF800
#define C_PINK    0xF81F
#define C_PURPLE  0x601F
#define C_WHITE   0xFFFF
#define C_LGREY   0xC618
#define C_GREY    0x7BEF
#define C_DGREY   0x39E7
// Retro colors (used when MODERN_DESIGN=0)
#define C_DARK    0x0863
#define C_PANEL   0x18C6

bool   showSlide[]  = {true,true,true,true,true,false};
String slideName[]  = {"SAAT","DOLAR","EURO","BTC","ETH","GOREV"};
uint16_t slideClr[] = {C_CYAN, C_GREEN, C_GOLD, C_ORANGE, C_BLUE1, C_RED};

int  sNo=0, menuIdx=0, cfgIdx=0;
bool inMenu=false, inSub=false, inCfg=false;
unsigned long sTime=0;

uint8_t* wavBuf;
const int WAV_SIZE = 8000*4;

String trFix(String s){
  s.replace("ü","u");s.replace("ğ","g");s.replace("ı","i");
  s.replace("ş","s");s.replace("ç","c");s.replace("ö","o");
  s.replace("Ü","U");s.replace("Ğ","G");s.replace("İ","I");
  s.replace("Ş","S");s.replace("Ç","C");s.replace("Ö","O");
  return s;
}

String getSaat(){
  struct tm t; if(!getLocalTime(&t)) return "--:--";
  char b[6]; strftime(b,6,"%H:%M",&t); return String(b);
}
String getTarih(){
  struct tm t; if(!getLocalTime(&t)) return "----";
  char b[12]; strftime(b,12,"%d/%m/%Y",&t); return String(b);
}
String getGun(){
  struct tm t; if(!getLocalTime(&t)) return "";
  const char* g[]={"Pazar","Pazartesi","Sali","Carsamba","Persembe","Cuma","Cumartesi"};
  return String(g[t.tm_wday]);
}

String httpGet(const String& url, int timeout) {
  if (WiFi.status() != WL_CONNECTED) return "---";
  WiFiClientSecure c; c.setInsecure();
  HTTPClient h; h.setTimeout(timeout);
  h.begin(c, url);
  int code = h.GET();
  String r = (code == 200) ? h.getString() : "---";
  h.end();
  return r;
}

String kriptoAl(String s){
  String r = httpGet("https://api.btcturk.com/api/v2/ticker?pairSymbol="+s, 2000);
  if(r == "---") return "---";
  DynamicJsonDocument d(1024);
  if(deserializeJson(d, r)) return "---";
  float v = d["data"][0]["last"].as<float>();
  if(v >= 1000000) return "$"+String((long)(v/1000))+"K";
  return "$"+String((long)v);
}

String kurAl(String b,String t){
  String r = httpGet("https://v6.exchangerate-api.com/v6/"+exKey+"/pair/"+b+"/"+t, 1500);
  if(r == "---") return "---";
  StaticJsonDocument<200> d;
  if(deserializeJson(d, r)) return "---";
  return String(d["conversion_rate"].as<float>(),2)+" TL";
}

String gorevAl(){
  if (WiFi.status() != WL_CONNECTED) return "Baglanti yok.";
  WiFiClientSecure c; c.setInsecure();
  HTTPClient h; h.setTimeout(4000);
  h.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  h.begin(c, scriptURL);
  int code = h.GET();
  String r = "Gorev yok.";
  if(code == 200) r = trFix(h.getString());
  h.end();
  return r;
}

void gradH(int x,int y,int w,int h,uint16_t c1,uint16_t c2){
  for(int i=0;i<w;i++){
    uint8_t r=(((c1>>11)&31)*(w-i)+((c2>>11)&31)*i)/w;
    uint8_t g=(((c1>>5)&63)*(w-i)+((c2>>5)&63)*i)/w;
    uint8_t b=((c1&31)*(w-i)+(c2&31)*i)/w;
    tft.drawFastVLine(x+i,y,h,(r<<11)|(g<<5)|b);
  }
}

void gradV(int x,int y,int w,int h,uint16_t c1,uint16_t c2){
  for(int i=0;i<h;i++){
    uint8_t r=(((c1>>11)&31)*(h-i)+((c2>>11)&31)*i)/h;
    uint8_t g=(((c1>>5)&63)*(h-i)+((c2>>5)&63)*i)/h;
    uint8_t b=((c1&31)*(h-i)+(c2&31)*i)/h;
    tft.drawFastHLine(x,y+i,w,(r<<11)|(g<<5)|b);
  }
}

#if !MODERN_DESIGN
void kart(int x,int y,int w,int h,uint16_t bg,uint16_t br){
  tft.fillRoundRect(x,y,w,h,5,bg);
  tft.drawRoundRect(x,y,w,h,5,br);
  tft.drawRoundRect(x+1,y+1,w-2,h-2,4,br>>1 & 0x7BEF);
}

void dot(int x,int y,uint16_t c){
  tft.fillCircle(x,y,2,c);
  tft.drawCircle(x,y,3,c>>1&0x7BEF);
}
#endif

void pageDots(int cur,int tot){
  if(tot<=1) return;
  int sx=(160-(tot*10-2))/2;
  for(int i=0;i<tot;i++){
    if(i==cur) tft.fillRoundRect(sx+i*10-4,120,8,4,2,C_CYAN);
    else        tft.fillRoundRect(sx+i*10-2,121,4,2,1,C_DGREY);
  }
}

int aktifSay(){ int n=0; for(int i=0;i<6;i++) if(showSlide[i]) n++; return n; }
int aktifNo(){ int n=0; for(int i=0;i<sNo;i++) if(showSlide[i]) n++; return n; }

void wrapText(String t,int x,int y,int mw,int my,uint16_t c){
  tft.setTextColor(c);
  String w="",l=""; t+=" ";
  for(int i=0;i<(int)t.length();i++){
    char ch=t[i];
    if(ch==' '||ch=='\n'){
      String tl=l+(l.length()?" ":"")+w;
      if((int)(tl.length()*6)>mw||ch=='\n'){
        tft.setCursor(x,y); tft.print(l); y+=12; l=w;
        if(y>my){tft.setCursor(x,y);tft.print("...");return;}
      } else l=tl;
      w="";
    } else w+=ch;
  }
  if(l.length()&&y<=my){tft.setCursor(x,y);tft.print(l);}
}

void header(String title, uint16_t c1, uint16_t c2){
#if MODERN_DESIGN
  gradH(0,0,160,24,c1,c2);
  tft.drawFastHLine(0,24,160,c2>>1&0x7BEF);
  tft.setTextSize(1);
  int tw=title.length()*6;
  int tx=(160-tw)/2;
  tft.setTextColor(C_WHITE); tft.setCursor(tx,8); tft.print(title);
#else
  gradH(0,0,160,22,c1,c2);
  tft.drawFastHLine(0,22,160,C_WHITE);
  tft.drawFastHLine(0,23,160,c1>>1&0x7BEF);
  tft.setTextSize(1);
  int tw=title.length()*6;
  int tx=(160-tw)/2;
  tft.setTextColor(C_BG); tft.setCursor(tx+1,8); tft.print(title);
  tft.setTextColor(C_WHITE); tft.setCursor(tx,7); tft.print(title);
  tft.fillRect(0,0,3,3,C_WHITE);
  tft.fillRect(157,0,3,3,C_WHITE);
#endif
}

void drawSlaytSaat(){
#if MODERN_DESIGN
  tft.fillScreen(C_BG);
  header("PIKSEL CLOCK", C_BLUE2, C_PURPLE);
  tft.fillRoundRect(8,32,144,56,6,C_CARD);
  tft.drawRoundRect(8,32,144,56,6,C_CYAN);
  String saat=getSaat();
  tft.setTextSize(4);
  int sx=(160-(int)saat.length()*24)/2;
  tft.setCursor(sx,36); tft.setTextColor(C_CYAN); tft.print(saat);
  tft.drawFastHLine(12,92,136,C_BORDER);
  tft.setTextSize(1);
  String tarih=getTarih();
  tft.setTextColor(C_LGREY);
  tft.setCursor((160-(int)tarih.length()*6)/2, 98); tft.print(tarih);
  String gun=getGun();
  tft.setTextColor(C_GREY);
  tft.setCursor((160-(int)gun.length()*6)/2, 110); tft.print(gun);
  pageDots(aktifNo(), aktifSay());
#else
  tft.fillScreen(C_BG);
  for(int y=26;y<118;y+=8) tft.drawFastHLine(0,y,160,C_DARK);
  header("◈ PIKSEL CLOCK ◈", C_BLUE2, C_PURPLE);
  kart(8,28,144,56,C_DARK,C_CYAN);
  gradV(10,30,140,52,C_PANEL,C_DARK);
  tft.drawRoundRect(8,28,144,56,5,C_CYAN);
  String saat=getSaat();
  tft.setTextSize(4);
  int sx=(160-(int)saat.length()*24)/2;
  tft.setCursor(sx+1,35); tft.setTextColor(C_BORDER); tft.print(saat);
  tft.setCursor(sx,34);   tft.setTextColor(C_CYAN);   tft.print(saat);
  tft.drawFastHLine(12,86,136,C_BORDER);
  tft.setTextSize(1);
  String tarih=getTarih();
  tft.setTextColor(C_LGREY);
  tft.setCursor((160-(int)tarih.length()*6)/2, 90); tft.print(tarih);
  String gun=getGun();
  tft.setTextColor(C_GREY);
  tft.setCursor((160-(int)gun.length()*6)/2, 102); tft.print(gun);
  pageDots(aktifNo(), aktifSay());
#endif
}

void drawSlaytKur(String title, String val, String sym, uint16_t ca, uint16_t cb){
  tft.fillScreen(C_BG);
  header(title, ca, cb);
#if MODERN_DESIGN
  tft.fillRoundRect(8,32,144,60,6,C_CARD);
  tft.drawRoundRect(8,32,144,60,6,ca);
  tft.fillRoundRect(14,40,36,20,4,ca);
  tft.setTextSize(1); tft.setTextColor(C_BG);
  tft.setCursor(14+(36-(int)sym.length()*6)/2, 47); tft.print(sym);
  tft.setTextSize(2); tft.setTextColor(C_WHITE);
  tft.setCursor(56,44); tft.print(val);
  tft.setTextSize(1); tft.setTextColor(C_DGREY);
  tft.setCursor(14,100); tft.print("ExchangeRate API");
  tft.drawFastHLine(8,95,144,C_BORDER);
  pageDots(aktifNo(), aktifSay());
#else
  for(int y=26;y<118;y+=8) tft.drawFastHLine(0,y,160,C_DARK);
  kart(8,28,144,60,C_DARK,ca);
  gradV(10,30,140,56,C_PANEL,C_DARK);
  tft.drawRoundRect(8,28,144,60,5,ca);
  tft.fillRoundRect(14,35,32,22,3,ca);
  tft.setTextColor(C_BG);
  tft.setCursor(14+(32-(int)sym.length()*6)/2, 43); tft.print(sym);
  tft.setTextSize(2); tft.setTextColor(C_WHITE);
  tft.setCursor(52,42); tft.print(val);
  tft.setTextSize(1); tft.setTextColor(C_GREY);
  tft.setCursor(14,96); tft.print("ExchangeRate API - Canli");
  dot(8,28,ca); dot(151,28,ca); dot(8,87,ca); dot(151,87,ca);
  pageDots(aktifNo(), aktifSay());
#endif
}

void drawSlaytKripto(String sym, String val, String title, uint16_t ca, uint16_t cb){
  tft.fillScreen(C_BG);
  header(title, ca, cb);
#if MODERN_DESIGN
  tft.fillRoundRect(8,32,144,60,6,C_CARD);
  tft.drawRoundRect(8,32,144,60,6,ca);
  tft.fillCircle(32,60,14,ca);
  tft.setTextSize(1); tft.setTextColor(C_BG);
  tft.setCursor(32-(int)sym.length()*3,56); tft.print(sym);
  tft.setTextSize(2); tft.setTextColor(C_GREEN);
  tft.setCursor(54,50); tft.print(val);
  tft.setTextSize(1); tft.setTextColor(C_GREY);
  tft.setCursor(54,72); tft.print("USDT - BTCTurk");
  tft.drawFastHLine(8,95,144,C_BORDER);
  pageDots(aktifNo(), aktifSay());
#else
  for(int y=26;y<118;y+=8) tft.drawFastHLine(0,y,160,C_DARK);
  kart(8,28,144,60,C_DARK,ca);
  gradV(10,30,140,56,C_PANEL,C_DARK);
  tft.drawRoundRect(8,28,144,60,5,ca);
  tft.fillCircle(32,58,16,ca);
  tft.drawCircle(32,58,17,C_WHITE);
  tft.setTextSize(1); tft.setTextColor(C_BG);
  tft.setCursor(32-(int)sym.length()*3,54); tft.print(sym);
  tft.setTextSize(2); tft.setTextColor(C_GREEN);
  tft.setCursor(54,48); tft.print(val);
  tft.setTextSize(1); tft.setTextColor(C_GREY);
  tft.setCursor(54,66); tft.print("USDT - BTCTurk");
  dot(8,28,ca); dot(151,28,ca); dot(8,87,ca); dot(151,87,ca);
  pageDots(aktifNo(), aktifSay());
#endif
}

void drawSlaytGorev(){
  tft.fillScreen(C_BG);
  header("GOREVLER", C_RED, C_PINK);
  tft.setTextSize(1); tft.setTextColor(C_GREY);
  tft.setCursor(60,40); tft.print("Yukleniyor...");
  String g=gorevAl();
  tft.fillRect(0,28,160,92,C_BG);
#if MODERN_DESIGN
  tft.fillRoundRect(6,32,148,84,6,C_CARD);
  tft.drawRoundRect(6,32,148,84,6,C_RED);
  tft.setTextSize(1);
  wrapText(g,12,40,136,110,C_WHITE);
#else
  for(int y=26;y<118;y+=8) tft.drawFastHLine(0,y,160,C_DARK);
  kart(6,28,148,84,C_DARK,C_RED);
  gradV(8,30,144,80,C_PANEL,C_DARK);
  tft.drawRoundRect(6,28,148,84,5,C_RED);
  tft.setTextSize(1);
  wrapText(g,12,36,136,106,C_WHITE);
  dot(6,28,C_RED); dot(153,28,C_RED);
#endif
  pageDots(aktifNo(), aktifSay());
}

void transition(){
#if MODERN_DESIGN
  tft.fillScreen(C_BG);
#else
  for(int x=0;x<160;x+=4){
    tft.drawFastVLine(x,0,128,C_BG);
    tft.drawFastVLine(x+1,0,128,C_BG);
    delay(1);
  }
#endif
}

void slaytGoster(int no){
  transition();
  if     (no==0) drawSlaytSaat();
  else if(no==1) drawSlaytKur("DOLAR KURU",kurAl("USD","TRY"),"USD",C_GREEN,C_LIME);
  else if(no==2) drawSlaytKur("EURO KURU", kurAl("EUR","TRY"),"EUR",C_GOLD,C_YELLOW);
  else if(no==3) drawSlaytKripto("BTC",kriptoAl("BTCUSDT"),"BITCOIN",C_ORANGE,C_GOLD);
  else if(no==4) drawSlaytKripto("ETH",kriptoAl("ETHUSDT"),"ETHEREUM",C_BLUE1,C_CYAN);
  else if(no==5) drawSlaytGorev();
}

void drawMenu(){
#if MODERN_DESIGN
  tft.fillScreen(C_BG);
  gradH(0,0,160,24,C_BLUE2,C_PURPLE);
  tft.drawFastHLine(0,24,160,C_CYAN);
  tft.setTextSize(1); tft.setTextColor(C_WHITE);
  tft.setCursor(52,8); tft.print("ANA MENU");
  tft.fillRect(0,117,160,11,C_SURF);
  tft.setTextColor(C_GREY);
  tft.setCursor(2,119); tft.print("UP+DOWN = Cikis");

  const char* lbl[]={"Canli Piyasa","Slayt Yonetimi","Hatirlatici","WiFi Bilgisi","Yeniden Baslat","Cikis"};
  uint16_t lc[]={C_GREEN,C_CYAN,C_YELLOW,C_BLUE1,C_RED,C_DGREY};

  for(int i=0;i<6;i++){
    int y=27+i*15;
    if(i==menuIdx){
      tft.fillRoundRect(4,y,152,13,3,lc[i]);
      tft.setTextColor(C_BG); tft.setCursor(10,y+3); tft.print(lbl[i]);
    } else {
      tft.setTextColor(C_GREY); tft.setCursor(10,y+3); tft.print(lbl[i]);
    }
  }
#else
  tft.fillScreen(C_BG);
  gradH(0,0,160,24,C_BLUE2,C_PURPLE);
  tft.drawFastHLine(0,24,160,C_CYAN);
  tft.setTextSize(1); tft.setTextColor(C_WHITE);
  tft.setCursor(28,8); tft.print("[ ANA MENU ]");
  tft.fillRect(0,117,160,11,C_DARK);
  tft.setTextColor(C_GREY);
  tft.setCursor(2,119); tft.print("UP+DOWN = Cikis");

  const char* lbl[]={"Canli Piyasa","Slayt Yonetimi","Hatirlatici","WiFi Bilgisi","Yeniden Baslat","Cikis"};
  uint16_t lc[]={C_GREEN,C_CYAN,C_YELLOW,C_BLUE1,C_RED,C_LGREY};

  for(int i=0;i<6;i++){
    int y=27+i*15;
    if(i==menuIdx){
      gradH(2,y,156,13,lc[i]>>1&0x7BEF,C_DARK);
      tft.drawRoundRect(2,y,156,13,3,lc[i]);
      tft.fillRect(2,y,4,13,lc[i]);
      tft.setTextColor(C_WHITE); tft.setCursor(10,y+3); tft.print(lbl[i]);
      tft.setCursor(148,y+3); tft.setTextColor(lc[i]); tft.print(">");
    } else {
      tft.fillRect(2,y,4,13,lc[i]>>2&0x39E7);
      tft.setTextColor(C_GREY); tft.setCursor(10,y+3); tft.print(lbl[i]);
    }
  }
#endif
}

void drawCfg(){
#if MODERN_DESIGN
  tft.fillScreen(C_BG);
  gradH(0,0,160,24,C_RED,C_PINK);
  tft.drawFastHLine(0,24,160,C_WHITE);
  tft.setTextSize(1); tft.setTextColor(C_WHITE);
  tft.setCursor(36,8); tft.print("SLAYT YONETIMI");
  tft.fillRect(0,117,160,11,C_SURF);
  tft.setTextColor(C_GREY);
  tft.setCursor(2,119); tft.print("OK=Degistir  UP+DOWN=Geri");

  for(int i=0;i<6;i++){
    int y=27+i*15;
    bool sel=(i==cfgIdx);
    bool on=showSlide[i];
    uint16_t ac=slideClr[i];
    if(sel){
      tft.drawRoundRect(2,y,156,13,3,ac);
    }
    tft.fillRect(2,y,4,13,on ? ac : C_DGREY);
    tft.setTextColor(sel ? C_WHITE : C_GREY);
    tft.setCursor(10,y+3); tft.print(slideName[i]);
    if(on){
      tft.fillRoundRect(120,y+2,36,9,4,ac);
      tft.setTextColor(C_BG); tft.setCursor(126,y+3); tft.print("ACIK");
    } else {
      tft.fillRoundRect(120,y+2,36,9,4,C_DGREY);
      tft.setTextColor(C_GREY); tft.setCursor(126,y+3); tft.print("KAPALI");
    }
  }
#else
  tft.fillScreen(C_BG);
  gradH(0,0,160,24,C_RED,C_PINK);
  tft.drawFastHLine(0,24,160,C_WHITE);
  tft.setTextSize(1); tft.setTextColor(C_WHITE);
  tft.setCursor(20,8); tft.print("SLAYT YONETIMI");
  tft.fillRect(0,117,160,11,C_DARK);
  tft.setTextColor(C_GREY);
  tft.setCursor(2,119); tft.print("OK=Degistir  UP+DOWN=Geri");

  for(int i=0;i<6;i++){
    int y=27+i*15;
    bool sel=(i==cfgIdx);
    bool on=showSlide[i];
    uint16_t ac=slideClr[i];
    if(sel){
      gradH(2,y,156,13,ac>>2&0x39E7,C_DARK);
      tft.drawRoundRect(2,y,156,13,3,ac);
    }
    tft.fillRect(2,y,4,13,on ? ac : C_DGREY);
    tft.setTextColor(sel ? C_WHITE : C_GREY);
    tft.setCursor(10,y+3); tft.print(slideName[i]);
    if(on){
      tft.fillRoundRect(120,y+2,36,9,4,ac);
      tft.setTextColor(C_BG); tft.setCursor(126,y+3); tft.print("ACIK");
    } else {
      tft.fillRoundRect(120,y+2,36,9,4,C_DGREY);
      tft.drawRoundRect(120,y+2,36,9,4,C_GREY);
      tft.setTextColor(C_GREY); tft.setCursor(122,y+3); tft.print("KAPALI");
    }
  }
#endif
}

void drawAIListen(){
  tft.fillScreen(C_BG);
  gradH(0,0,160,24,C_RED,C_PINK);
  tft.drawFastHLine(0,24,160,C_WHITE);
  tft.setTextSize(1); tft.setTextColor(C_WHITE);
#if MODERN_DESIGN
  tft.setCursor(48,8); tft.print("AI ASISTAN");
  tft.fillCircle(80,68,34,C_RED);
  tft.fillCircle(80,68,28,C_BG);
  tft.fillCircle(80,68,22,C_RED);
  tft.setTextSize(2); tft.setTextColor(C_WHITE);
  tft.setCursor(66,61); tft.print("MIC");
  tft.setTextSize(1); tft.setTextColor(C_GREY);
  tft.setCursor(10,114); tft.print("Birakinca gonderiyor...");
#else
  tft.setCursor(38,8); tft.print("AI ASISTAN");
  tft.drawCircle(80,74,38,C_RED>>2&0x39E7);
  tft.drawCircle(80,74,30,C_RED>>1&0x7BEF);
  tft.fillCircle(80,74,22,C_RED);
  tft.drawCircle(80,74,22,C_WHITE);
  tft.setTextSize(2); tft.setTextColor(C_WHITE);
  tft.setCursor(66,67); tft.print("MIC");
  tft.setTextSize(1); tft.setTextColor(C_GREY);
  tft.setCursor(10,112); tft.print("Birakinca gonderiyor...");
#endif
}

void drawAIThink(){
  tft.fillScreen(C_BG);
  gradH(0,0,160,24,C_BLUE2,C_PURPLE);
  tft.drawFastHLine(0,24,160,C_CYAN);
  tft.setTextSize(1); tft.setTextColor(C_WHITE);
#if MODERN_DESIGN
  tft.setCursor(44,8); tft.print("DU$UNUYOR...");
  tft.fillCircle(80,66,32,C_BLUE2);
  tft.fillCircle(80,66,26,C_BG);
  tft.fillCircle(80,66,20,C_BLUE2);
  tft.setTextSize(2); tft.setTextColor(C_WHITE);
  tft.setCursor(68,59); tft.print("AI");
  for(int i=0;i<3;i++) tft.fillCircle(64+i*16,106,4,C_CYAN);
  tft.setTextSize(1); tft.setTextColor(C_GREY);
  tft.setCursor(22,116); tft.print("Gemini 2.5 Flash");
#else
  tft.setCursor(38,8); tft.print("DUSUNUYOR...");
  tft.drawCircle(80,70,32,C_BORDER);
  tft.drawCircle(80,70,26,C_BLUE2);
  tft.fillCircle(80,70,20,C_PANEL);
  tft.setTextSize(2); tft.setTextColor(C_CYAN);
  tft.setCursor(68,63); tft.print("AI");
  for(int i=0;i<3;i++) tft.fillCircle(64+i*16,104,3,C_BLUE2);
  tft.setTextSize(1); tft.setTextColor(C_GREY);
  tft.setCursor(22,114); tft.print("Gemini 2.5 Flash");
#endif
}

void drawAIResp(String ans){
  tft.fillScreen(C_BG);
  gradH(0,0,160,24,C_PURPLE,C_BLUE2);
  tft.drawFastHLine(0,24,160,C_CYAN);
  tft.setTextSize(1); tft.setTextColor(C_WHITE);
#if MODERN_DESIGN
  tft.setCursor(64,8); tft.print("YANIT");
  tft.fillRoundRect(4,32,152,84,6,C_CARD);
  tft.drawRoundRect(4,32,152,84,6,C_CYAN);
  tft.setTextSize(1);
  wrapText(ans,10,40,140,110,C_WHITE);
  tft.setTextColor(C_DGREY);
  tft.setCursor(4,120); tft.print("7 saniye sonra kapanir");
#else
  tft.setCursor(52,8); tft.print("YANIT");
  kart(4,28,152,84,C_DARK,C_CYAN);
  gradV(6,30,148,80,C_PANEL,C_DARK);
  tft.drawRoundRect(4,28,152,84,5,C_CYAN);
  tft.setTextSize(1);
  wrapText(ans,10,36,140,106,C_WHITE);
  tft.setTextColor(C_DGREY);
  tft.setCursor(4,118); tft.print("7 saniye sonra kapanir");
#endif
}

void writeWav(int ds){
  memcpy(wavBuf,"RIFF",4);
  uint32_t fs=ds+36; memcpy(wavBuf+4,&fs,4);
  memcpy(wavBuf+8,"WAVEfmt ",8);
  uint32_t f=16; memcpy(wavBuf+16,&f,4);
  uint16_t a=1;  memcpy(wavBuf+20,&a,2);
  uint16_t ch=1; memcpy(wavBuf+22,&ch,2);
  uint32_t sr=8000; memcpy(wavBuf+24,&sr,4);
  uint32_t br=8000; memcpy(wavBuf+28,&br,4);
  uint16_t ba=1; memcpy(wavBuf+32,&ba,2);
  uint16_t bp=8; memcpy(wavBuf+34,&bp,2);
  memcpy(wavBuf+36,"data",4); memcpy(wavBuf+40,&ds,4);
}

void splashBoot(){
#if MODERN_DESIGN
  tft.fillScreen(C_BG);
  gradH(0,0,160,128,C_BLUE2,C_PURPLE);
  tft.setTextSize(2); tft.setTextColor(C_WHITE);
  tft.setCursor(30,36); tft.print("pixiZ v1");
  tft.setTextSize(1); tft.setTextColor(C_CYAN);
  tft.setCursor(40,62); tft.print("AI Assistant");
  tft.drawFastHLine(20,78,120,C_WHITE);
  tft.setTextColor(C_LGREY);
  tft.setCursor(12,88); tft.print("WiFi baglaniyor");
  WiFi.mode(WIFI_STA); WiFi.begin(ssid, password);
  int cnt=0;
  while(WiFi.status()!=WL_CONNECTED && cnt<20){
    tft.fillRect(10,104,140,10,C_BG);
    int prog=cnt*7;
    tft.fillRoundRect(10,104,140,10,5,C_CARD);
    tft.fillRoundRect(10,104,prog,10,5,C_CYAN);
    delay(500); cnt++;
  }
  tft.fillRect(10,104,140,10,C_BG);
  tft.fillRoundRect(10,104,140,10,5,C_GREEN);
  tft.setTextColor(C_BG); tft.setCursor(44,105); tft.print("BAGLANDI");
  delay(700);
#else
  tft.fillScreen(C_BG);
  for(int i=0;i<4;i++){
    tft.drawRoundRect(i,i,160-i*2,128-i*2,8-i,i==0?C_CYAN:i==1?C_BLUE2:i==2?C_PURPLE:C_DARK);
    delay(30);
  }
  gradH(20,40,120,32,C_BLUE2,C_PURPLE);
  tft.setTextSize(2); tft.setTextColor(C_WHITE);
  tft.setCursor(26,48); tft.print("pixiZ v1");
  tft.setTextSize(1); tft.setTextColor(C_CYAN);
  tft.setCursor(40,80); tft.print("AI Assistant");
  tft.setTextColor(C_GREY);
  tft.setCursor(12,96); tft.print("WiFi baglaniyor");
  WiFi.mode(WIFI_STA); WiFi.begin(ssid, password);
  int cnt=0;
  while(WiFi.status()!=WL_CONNECTED && cnt<20){
    tft.fillRect(10,106,140,8,C_BG);
    int prog=cnt*7;
    tft.fillRoundRect(10,106,prog,8,3,C_BLUE2);
    tft.drawRoundRect(10,106,140,8,3,C_BORDER);
    delay(500); cnt++;
  }
  tft.fillRect(10,106,140,8,C_BG);
  tft.fillRoundRect(10,106,140,8,3,C_GREEN);
  tft.setTextColor(C_BG); tft.setCursor(48,107); tft.print("BAGLANDI");
  delay(700);
#endif
}

void geriDon(){
  if(inCfg){
    inCfg=false; drawMenu();
  } else if(inSub){
    inSub=false; drawMenu();
  } else if(inMenu){
    inMenu=false;
    transition();
    slaytGoster(sNo);
    sTime=millis();
  }
  while(digitalRead(BTN_UP)==LOW || digitalRead(BTN_DOWN)==LOW) delay(5);
}

void setup(){
  Serial.begin(115200);
  pinMode(TFT_LED,OUTPUT); digitalWrite(TFT_LED,HIGH);
  pinMode(BTN_AI,INPUT_PULLUP); pinMode(BTN_UP,INPUT_PULLUP);
  pinMode(BTN_DOWN,INPUT_PULLUP); pinMode(BTN_MENU,INPUT_PULLUP);
  pinMode(TFT_RST,OUTPUT);
  digitalWrite(TFT_RST,LOW); delay(100);
  digitalWrite(TFT_RST,HIGH); delay(100);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(C_BG);
  splashBoot();
  configTime(3*3600,0,"pool.ntp.org");
  wavBuf=(uint8_t*)malloc(WAV_SIZE+44);
  if(!wavBuf) { tft.setTextColor(C_RED); tft.setCursor(8,50); tft.print("RAM yetersiz!"); while(1) delay(1000); }
  tft.fillScreen(C_BG);
  sNo=0; slaytGoster(sNo); sTime=millis();
}

void loop(){
  bool up   = (digitalRead(BTN_UP)   == LOW);
  bool down = (digitalRead(BTN_DOWN) == LOW);
  bool menu = (digitalRead(BTN_MENU) == LOW);

  // ─── UP + DOWN = GERİ ────────────────────────
  if(up && down){
    delay(60);
    if(digitalRead(BTN_UP)==LOW && digitalRead(BTN_DOWN)==LOW){
      delay(100);
      geriDon();
      return;
    }
  }

  // ─── MENU ────────────────────────────────────
  if(menu){
    delay(180);
    if(!inMenu && !inSub && !inCfg){
      inMenu=true; menuIdx=0; drawMenu();
    } else if(inCfg){
      showSlide[cfgIdx]=!showSlide[cfgIdx];
      int ac=0; for(int i=0;i<6;i++) if(showSlide[i]) ac++;
      if(ac==0) showSlide[cfgIdx]=true;
      drawCfg();
    } else if(inMenu && !inSub){
      switch(menuIdx){
        case 0:
          inSub=true;
          tft.fillScreen(C_BG);
          header("PIYASALAR",C_GREEN,C_LIME);
#if MODERN_DESIGN
          tft.fillRoundRect(6,32,148,82,6,C_CARD);
          tft.drawRoundRect(6,32,148,82,6,C_GREEN);
          tft.fillRect(6,32,4,82,C_GREEN);
          tft.setTextSize(1);
          tft.setTextColor(C_GREEN);  tft.setCursor(14,40); tft.print("USD  "); tft.setTextColor(C_WHITE); tft.print(kurAl("USD","TRY"));
          tft.drawFastHLine(8,56,142,C_BORDER);
          tft.setTextColor(C_GOLD);   tft.setCursor(14,60); tft.print("EUR  "); tft.setTextColor(C_WHITE); tft.print(kurAl("EUR","TRY"));
          tft.drawFastHLine(8,76,142,C_BORDER);
          tft.setTextColor(C_ORANGE); tft.setCursor(14,80); tft.print("BTC  "); tft.setTextColor(C_WHITE); tft.print(kriptoAl("BTCUSDT"));
          tft.drawFastHLine(8,96,142,C_BORDER);
          tft.setTextColor(C_BLUE1);  tft.setCursor(14,100); tft.print("ETH  "); tft.setTextColor(C_WHITE); tft.print(kriptoAl("ETHUSDT"));
          tft.fillRect(0,117,160,11,C_SURF);
          tft.setTextColor(C_GREY); tft.setCursor(2,119); tft.print("UP+DOWN = Geri");
#else
          for(int y=26;y<118;y+=8) tft.drawFastHLine(0,y,160,C_DARK);
          header("◈ PIYASALAR ◈",C_GREEN,C_LIME);
          kart(6,28,148,82,C_DARK,C_GREEN);
          gradV(8,30,144,78,C_PANEL,C_DARK);
          tft.setTextSize(1); tft.fillRect(6,28,4,82,C_GREEN);
          tft.setTextColor(C_GREEN);  tft.setCursor(14,36); tft.print("USD  "); tft.setTextColor(C_WHITE); tft.print(kurAl("USD","TRY"));
          tft.drawFastHLine(8,50,142,C_DARK);
          tft.setTextColor(C_GOLD);   tft.setCursor(14,54); tft.print("EUR  "); tft.setTextColor(C_WHITE); tft.print(kurAl("EUR","TRY"));
          tft.drawFastHLine(8,68,142,C_DARK);
          tft.setTextColor(C_ORANGE); tft.setCursor(14,72); tft.print("BTC  "); tft.setTextColor(C_WHITE); tft.print(kriptoAl("BTCUSDT"));
          tft.drawFastHLine(8,86,142,C_DARK);
          tft.setTextColor(C_BLUE1);  tft.setCursor(14,90); tft.print("ETH  "); tft.setTextColor(C_WHITE); tft.print(kriptoAl("ETHUSDT"));
          tft.fillRect(0,117,160,11,C_DARK);
          tft.setTextColor(C_GREY); tft.setCursor(2,119); tft.print("UP+DOWN = Geri");
#endif
          break;
        case 1:
          inCfg=true; cfgIdx=0; drawCfg();
          break;
        case 2:
          inSub=true;
          tft.fillScreen(C_BG);
          header("HATIRLATICI",C_YELLOW,C_GOLD);
          tft.setTextSize(1); tft.setTextColor(C_GREY);
          tft.setCursor(56,50); tft.print("Yukleniyor...");
          { String g=gorevAl();
            tft.fillRect(0,28,160,90,C_BG);
#if MODERN_DESIGN
            tft.fillRoundRect(6,32,148,84,6,C_CARD);
            tft.drawRoundRect(6,32,148,84,6,C_YELLOW);
            tft.setTextSize(1);
            wrapText(g,12,40,136,110,C_WHITE); }
          tft.fillRect(0,117,160,11,C_SURF);
#else
            for(int y=26;y<118;y+=8) tft.drawFastHLine(0,y,160,C_DARK);
            kart(6,28,148,82,C_DARK,C_YELLOW);
            gradV(8,30,144,78,C_PANEL,C_DARK);
            tft.setTextSize(1);
            wrapText(g,12,36,136,104,C_WHITE); }
          tft.fillRect(0,117,160,11,C_DARK);
#endif
          tft.setTextColor(C_GREY); tft.setCursor(2,119); tft.print("UP+DOWN = Geri");
          break;
        case 3:
          inSub=true;
          tft.fillScreen(C_BG);
          header("WiFi DURUMU",C_BLUE1,C_CYAN);
#if MODERN_DESIGN
          tft.fillRoundRect(6,32,148,70,6,C_CARD);
          tft.drawRoundRect(6,32,148,70,6,C_BLUE1);
          tft.fillRect(6,32,4,70,C_BLUE1);
          tft.setTextSize(1);
          tft.setTextColor(C_BLUE1);  tft.setCursor(14,40); tft.print("SSID  "); tft.setTextColor(C_WHITE); tft.print(ssid);
          tft.drawFastHLine(8,54,142,C_BORDER);
          tft.setTextColor(C_CYAN);   tft.setCursor(14,58); tft.print("IP    "); tft.setTextColor(C_WHITE); tft.print(WiFi.localIP());
          tft.drawFastHLine(8,72,142,C_BORDER);
          tft.setTextColor(C_GREEN);  tft.setCursor(14,76); tft.print("RSSI  "); tft.setTextColor(C_WHITE); tft.print(WiFi.RSSI()); tft.print(" dBm");
          tft.drawFastHLine(8,90,142,C_BORDER);
          tft.setTextColor(C_GREEN);  tft.setCursor(14,94); tft.print("Durum "); tft.setTextColor(C_WHITE); tft.print("Bagli");
          tft.fillRect(0,117,160,11,C_SURF);
          tft.setTextColor(C_GREY); tft.setCursor(2,119); tft.print("UP+DOWN = Geri");
#else
          for(int y=26;y<118;y+=8) tft.drawFastHLine(0,y,160,C_DARK);
          header("◈ WiFi DURUMU ◈",C_BLUE1,C_CYAN);
          kart(6,28,148,70,C_DARK,C_BLUE1);
          gradV(8,30,144,66,C_PANEL,C_DARK);
          tft.fillRect(6,28,4,70,C_BLUE1);
          tft.setTextSize(1);
          tft.setTextColor(C_BLUE1);  tft.setCursor(14,36); tft.print("SSID  "); tft.setTextColor(C_WHITE); tft.print(ssid);
          tft.drawFastHLine(8,50,142,C_DARK);
          tft.setTextColor(C_CYAN);   tft.setCursor(14,54); tft.print("IP    "); tft.setTextColor(C_WHITE); tft.print(WiFi.localIP());
          tft.drawFastHLine(8,68,142,C_DARK);
          tft.setTextColor(C_GREEN);  tft.setCursor(14,72); tft.print("RSSI  "); tft.setTextColor(C_WHITE); tft.print(WiFi.RSSI()); tft.print(" dBm");
          tft.drawFastHLine(8,86,142,C_DARK);
          tft.setTextColor(C_GREEN);  tft.setCursor(14,90); tft.print("Durum "); tft.setTextColor(C_WHITE); tft.print("Bagli");
          tft.fillRect(0,117,160,11,C_DARK);
          tft.setTextColor(C_GREY); tft.setCursor(2,119); tft.print("UP+DOWN = Geri");
#endif
          break;
        case 4:
          tft.fillScreen(C_BG);
          gradH(0,48,160,32,C_RED,C_PINK);
          tft.setTextSize(2); tft.setTextColor(C_WHITE);
          tft.setCursor(14,55); tft.print("YENIDEN BASL");
          delay(1400); ESP.restart();
          break;
        case 5:
          geriDon();
          break;
      }
    }
    while(digitalRead(BTN_MENU)==LOW) delay(5);
  }

  // ─── UP ──────────────────────────────────────
  if(digitalRead(BTN_UP)==LOW && digitalRead(BTN_DOWN)==HIGH){
    delay(160);
    if(inMenu && !inSub){
      if(inCfg){ cfgIdx=(cfgIdx-1+6)%6; drawCfg(); }
      else      { menuIdx=(menuIdx-1+6)%6; drawMenu(); }
    } else if(!inMenu){
      do{ sNo=(sNo-1+6)%6; }while(!showSlide[sNo]);
      slaytGoster(sNo); sTime=millis();
    }
    while(digitalRead(BTN_UP)==LOW) delay(5);
  }

  // ─── DOWN ────────────────────────────────────
  if(digitalRead(BTN_DOWN)==LOW && digitalRead(BTN_UP)==HIGH){
    delay(160);
    if(inMenu && !inSub){
      if(inCfg){ cfgIdx=(cfgIdx+1)%6; drawCfg(); }
      else      { menuIdx=(menuIdx+1)%6; drawMenu(); }
    } else if(!inMenu){
      do{ sNo=(sNo+1)%6; }while(!showSlide[sNo]);
      slaytGoster(sNo); sTime=millis();
    }
    while(digitalRead(BTN_DOWN)==LOW) delay(5);
  }

  // ─── OTOMATİK ────────────────────────────────
  if(!inMenu && !inSub && millis()-sTime>12000){
    sTime=millis();
    do{ sNo=(sNo+1)%6; }while(!showSlide[sNo]);
    slaytGoster(sNo);
  }

  // ─── AI ──────────────────────────────────────
  if(digitalRead(BTN_AI)==LOW && !inMenu){
    drawAIListen();
    int i=0;
    while(digitalRead(BTN_AI)==LOW && i<WAV_SIZE){
      wavBuf[44+i]=map(analogRead(MIC_PIN),0,4095,0,255);
      i++; delayMicroseconds(125);
    }
    if(i>500){
      drawAIThink();
      writeWav(i);
      
      WiFiClientSecure cl; 
      cl.setInsecure(); 
      HTTPClient http;
      http.setTimeout(20000);
      http.begin(cl,"https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key="+apiKey);
      http.addHeader("Content-Type","application/json");
      
      String b64Audio = base64::encode(wavBuf,i+44);
      String payload = "{\"contents\":[{\"parts\":[{\"text\":\"Türkçe soru cevapla. En fazla 2-3 cümle. Eğer ses tanınamadıysa 'Anlaşılamadı' de.\"},{\"inline_data\":{\"mime_type\":\"audio/wav\",\"data\":\""+b64Audio+"\"}}]}]}";
      
      int httpCode = http.POST(payload);
      
      if(httpCode==200){
        String response = http.getString();
        DynamicJsonDocument doc(8192);
        DeserializationError error = deserializeJson(doc, response);
        
        if(!error){
          if(doc.containsKey("candidates") && doc["candidates"].size()>0){
            String ans = doc["candidates"][0]["content"]["parts"][0]["text"].as<String>();
            ans = trFix(ans);
            drawAIResp(ans);
            delay(7000);
          } else {
            drawAIResp("Hata: Yanit alinamadi");
            delay(3000);
          }
        } else {
          drawAIResp("JSON Parse Hatasi");
          delay(3000);
        }
      } else if(httpCode==400){
        drawAIResp("Hata: Gecersiz istek");
        delay(3000);
      } else if(httpCode==401 || httpCode==403){
        drawAIResp("Hata: API Anahtari");
        delay(3000);
      } else if(httpCode==-1){
        drawAIResp("Hata: Baglantilar Yok");
        delay(3000);
      } else {
        drawAIResp("Hata: "+String(httpCode));
        delay(3000);
      }
      http.end();
    }
    transition(); slaytGoster(sNo); sTime=millis();
  }
}
