/* AB GhostGuard LIVE 1.4 CYBER GREEN - MS ROBOTIKA
   ESP32-2432S028R / ILI9341 240x320 / XPT2046
   Verified physical pin profile from MS Robotika WiFi Radar.
   USB serial telemetry, read-only; NO security enforcement.
*/
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <math.h>
#include <Preferences.h>

SPIClass lcdBus(HSPI), touchBus(VSPI);
// Full-frame off-screen buffer: swap complete frames without clearing the visible TFT.
// No full-frame allocation: incremental renderer is used after page changes.
GFXcanvas16 frame(1,1);
Adafruit_GFX *screen = nullptr;
bool frameReady = false;
Adafruit_ILI9341 lcd(&lcdBus,2,15,-1);
XPT2046_Touchscreen touch(33,36);
constexpr uint16_t BG=0x0000, CARD=0x0822, CYAN=0x07FF, BLUE=0x05BF;
constexpr uint16_t GREEN=0x07E0, ORANGE=0xFD20, RED=0xF800, WHITE=0xFFFF;
constexpr uint16_t DIM=0x8C71, GRID=0x12A4, PURPLE=0x57FF, YELLOW=0xFFE0;
constexpr uint32_t STALE_MS=6500;
struct Telemetry {
  int cpu=-1,ram=-1,disk=-1,connections=-1,defender=-1,events=-1;
  int down=-1,up=-1; // KiB/s
  char host[21]="";
  char eventText[57]="";
  uint32_t last=0;
} pc;
uint8_t page=0;
bool touchOK=false,prevTouch=false,needsDraw=true;
bool fullRedraw=true; // only on screen transitions or boot
Preferences touchPrefs;
// Raw touch points from a 3-target calibration, persisted in NVS.
// This handles axis swaps and inverted axes without guessing the CYD revision.
int32_t calX0=0,calY0=0,calX1=0,calY1=0,calX2=0,calY2=0;
bool calibrated=false;
uint32_t touchDownAt=0;

uint32_t lastTap=0,lastPaint=0,lastSecond=0;
char lineBuf[320];size_t lineLen=0;
uint8_t netHistory[32]={};
uint32_t lastNetSample=0;
const char* tabs[]={"HOME","RED","EVENT","ESP32"};

void txt(int x,int y,const String &s,uint16_t c=WHITE,uint8_t size=1){
  screen->setTextWrap(false);screen->setTextColor(c);screen->setTextSize(size);screen->setCursor(x,y);screen->print(s);
}
void card(int x,int y,int w,int h,uint16_t edge=GRID){
  screen->fillRoundRect(x,y,w,h,8,CARD);screen->drawRoundRect(x,y,w,h,8,edge);
}
void bar(int x,int y,int w,int h,int pct,uint16_t color){
  screen->fillRoundRect(x,y,w,h,3,GRID);
  if(pct>=0 && pct>0)screen->fillRoundRect(x,y,max(2,w*constrain(pct,0,100)/100),h,3,color);
}
void ring(int cx,int cy,int r,int value,uint16_t color){
  screen->drawCircle(cx,cy,r,GRID);screen->drawCircle(cx,cy,r-1,GRID);
  if(value>=0)for(int a=-90;a<-90+constrain(value,0,100)*360/100;a+=5){
    float t=a*0.0174532925f;
    int x1=cx+(r-3)*cosf(t),y1=cy+(r-3)*sinf(t);
    int x2=cx+r*cosf(t),y2=cy+r*sinf(t);
    screen->drawLine(x1,y1,x2,y2,color);
  }
}
String val(int n,const char* suffix="%") {return n<0?"--":String(n)+suffix;}
bool online(){return pc.last && millis()-pc.last<STALE_MS;}
// Cyber Green identity: static chrome drawn only on page change.
void shell(const char* title,uint16_t accent){
  screen->fillScreen(BG);
  screen->drawFastHLine(0,0,240,GREEN);
  screen->fillRect(0,1,240,42,CARD);
  screen->drawFastHLine(0,42,240,GREEN);
  txt(9,7,"GHOSTGUARD",GREEN,2);
  txt(10,29,"MS ROBOTIKA  /  CYBER MONITOR",DIM);
  screen->fillRoundRect(176,8,59,18,4,GRID);
  txt(180,13,online()?"PC LIVE":"OFFLINE",online()?GREEN:ORANGE);
  txt(10,52,title,accent,2);
  screen->drawFastHLine(9,73,222,GRID);
  screen->fillRect(0,279,240,41,CARD);
  screen->drawFastHLine(0,279,240,GREEN);
  for(int i=0;i<4;i++){
    int x=i*60;
    if(page==i)screen->fillRoundRect(x+3,283,54,4,2,GREEN);
    txt(x+((i==2)?9:12),299,tabs[i],page==i?GREEN:DIM);
  }
}
void home(){
  shell("SECURITY HUB",GREEN);
  card(9,77,222,84,GREEN);
  ring(52,119,30,online()?(pc.defender==1?100:pc.defender==0?30:0):0,online()&&pc.defender==1?GREEN:ORANGE);
  txt(42,111,"PC",WHITE);
  txt(36,124,online()?"LIVE":"OFF",online()?GREEN:ORANGE);
  txt(90,88,online()?String(pc.host):"SIN AGENTE",WHITE);
  txt(90,107,"DEFENDER",DIM);
  txt(90,122,!online()?"SIN DATOS":pc.defender==1?"ACTIVADO":pc.defender==0?"DESACTIVADO":"DESCONOCIDO",online()&&pc.defender==1?GREEN:ORANGE);
  txt(90,141,"SOLO MONITORIZA",CYAN);
  card(9,169,105,101,GREEN);card(124,169,107,101,CYAN);
  txt(18,180,"CPU WINDOWS",DIM);
  txt(19,201,online()?val(pc.cpu):"--",GREEN,3);
  bar(19,248,84,8,online()?pc.cpu:-1,GREEN);
  txt(133,180,"RAM WINDOWS",DIM);
  txt(134,201,online()?val(pc.ram):"--",CYAN,3);
  bar(134,248,84,8,online()?pc.ram:-1,CYAN);
}
void network(){
  shell("NETWORK LIVE",CYAN);
  card(9,77,222,64,CYAN);
  txt(18,88,"DESCARGA",CYAN);txt(129,88,"SUBIDA",ORANGE);
  txt(18,105,online()?val(pc.down," KB/s"):"--",CYAN,2);
  txt(129,105,online()?val(pc.up," KB/s"):"--",ORANGE,2);
  card(9,149,222,121,GREEN);
  txt(18,159,"NETWORK WAVEFORM",GREEN);
  for(int j=0;j<3;j++)screen->drawFastHLine(19,193+j*23,201,GRID);
  for(int i=1;i<32;i++){
    int x1=19+(i-1)*6,x2=19+i*6;
    int y1=251-(netHistory[i-1]*54/100),y2=251-(netHistory[i]*54/100);
    screen->drawLine(x1,y1,x2,y2,CYAN);
  }
  txt(19,258,online()?"CONEXIONES: "+val(pc.connections,""):"SIN TELEMETRIA",online()?GREEN:ORANGE);
}
void eventsPage(){
  shell("SECURITY EVENTS",ORANGE);
  card(9,77,222,58,ORANGE);
  txt(18,89,"EVENTOS DEFENDER 24H",DIM);
  txt(18,107,online()?val(pc.events,""):"--",online()?YELLOW:ORANGE,2);
  card(9,144,222,126,GREEN);
  txt(18,155,"ULTIMO EVENTO",CYAN);
  if(!online()){
    txt(18,184,"AGENTE DESCONECTADO",ORANGE);
    txt(18,203,"SIN ALERTAS EN VIVO",DIM);
  }else{
    String s=String(pc.eventText);
    if(s.length()==0)s="Sin eventos recientes";
    for(int k=0;k<3 && s.length();k++){
      String part=s.substring(0,min((int)s.length(),27));
      txt(18,181+k*20,part,WHITE);
      s.remove(0,part.length());
    }
  }
  txt(18,247,"LECTURA / NO ANTIVIRUS",DIM);
}
void systemPage(){
  shell("SYSTEM CORE",GREEN);
  card(9,77,222,49,GREEN);
  txt(18,87,"ESP32 RAM LIBRE",DIM);
  txt(18,102,String(ESP.getFreeHeap()/1024)+" KB",GREEN,2);
  card(9,133,107,65,CYAN);card(124,133,107,65,ORANGE);
  txt(18,144,"CPU PC",DIM);txt(18,164,online()?val(pc.cpu):"--",CYAN,2);
  txt(133,144,"DISCO PC",DIM);txt(133,164,online()?val(pc.disk):"--",ORANGE,2);
  card(9,205,222,65,GREEN);
  txt(18,217,"ESP32: "+String(ESP.getCpuFreqMHz())+" MHz",WHITE);
  txt(18,234,"UPTIME: "+String(millis()/1000)+" s",DIM);
  txt(18,251,online()?"USB: DATOS REALES":"USB: ESPERANDO AGENTE",online()?GREEN:ORANGE);
}
// Incremental updates: never clear or upload a complete frame on telemetry.
// Initial page composition still uses the off-screen canvas, once per navigation.
void textPatch(int x,int y,int w,int h,const String &value,uint16_t color,uint8_t size=1){
  lcd.fillRect(x,y,w,h,CARD);
  lcd.setTextWrap(false);lcd.setTextColor(color,CARD);lcd.setTextSize(size);
  lcd.setCursor(x,y);lcd.print(value);
}
void barPatch(int x,int y,int w,int h,int pct,uint16_t color){
  lcd.fillRoundRect(x,y,w,h,3,GRID);
  if(pct>0)lcd.fillRoundRect(x,y,max(2,w*constrain(pct,0,100)/100),h,3,color);
}
void ringPatch(int cx,int cy,int r,int value,uint16_t color){
  lcd.fillCircle(cx,cy,r+1,CARD);
  screen=&lcd;ring(cx,cy,r,value,color);
}
void updateDynamic(){
  screen=&lcd;
  bool live=online();
  // Status label is the only changing region of the fixed header.
  lcd.fillRect(177,12,56,14,CARD);
  textPatch(180,13,53,11,live?"PC LIVE":"OFFLINE",live?GREEN:ORANGE);
  if(page==0){
    ringPatch(52,119,30,live?(pc.defender==1?100:pc.defender==0?30:0):0,live&&pc.defender==1?GREEN:ORANGE);
    textPatch(35,124,37,11,live?"LIVE":"OFF",live?GREEN:ORANGE);
    textPatch(90,88,135,12,live?String(pc.host):"SIN AGENTE",WHITE);
    textPatch(90,122,135,12,!live?"SIN DATOS":pc.defender==1?"ACTIVADO":pc.defender==0?"DESACTIVADO":"DESCONOCIDO",live&&pc.defender==1?GREEN:ORANGE);
    textPatch(19,201,90,26,live?val(pc.cpu):"--",GREEN,3);
    textPatch(134,201,90,26,live?val(pc.ram):"--",CYAN,3);
    barPatch(19,248,84,8,live?pc.cpu:-1,GREEN);
    barPatch(134,248,84,8,live?pc.ram:-1,CYAN);
  }else if(page==1){
    textPatch(18,105,105,19,live?val(pc.down," KB/s"):"--",CYAN,2);
    textPatch(129,105,96,19,live?val(pc.up," KB/s"):"--",ORANGE,2);
    // Only redraw the graph's inner plot, never its surrounding card.
    lcd.fillRect(19,190,202,64,CARD);
    for(int j=0;j<3;j++)lcd.drawFastHLine(19,193+j*23,201,GRID);
    for(int i=1;i<32;i++){
      int x1=19+(i-1)*6,x2=19+i*6;
      int y1=251-(netHistory[i-1]*54/100),y2=251-(netHistory[i]*54/100);
      lcd.drawLine(x1,y1,x2,y2,CYAN);
    }
    textPatch(19,258,204,11,live?"CONEXIONES: "+val(pc.connections,""):"SIN TELEMETRIA",live?GREEN:ORANGE);
  }else if(page==2){
    textPatch(18,107,200,20,live?val(pc.events,""):"--",live?YELLOW:ORANGE,2);
    lcd.fillRect(18,181,206,64,CARD);
    if(!live){
      textPatch(18,184,204,12,"AGENTE DESCONECTADO",ORANGE);
      textPatch(18,203,204,12,"SIN ALERTAS EN VIVO",DIM);
    }else{
      String t=String(pc.eventText);if(t.isEmpty())t="Sin eventos recientes";
      for(int k=0;k<3 && t.length();k++){
        String part=t.substring(0,min((int)t.length(),27));
        textPatch(18,181+k*20,204,12,part,WHITE);t.remove(0,part.length());
      }
    }
  }else{
    textPatch(18,102,195,20,String(ESP.getFreeHeap()/1024)+" KB",GREEN,2);
    textPatch(18,164,93,20,live?val(pc.cpu):"--",CYAN,2);
    textPatch(133,164,91,20,live?val(pc.disk):"--",ORANGE,2);
    textPatch(18,234,200,12,"UPTIME: "+String(millis()/1000)+" s",DIM);
    textPatch(18,251,207,12,live?"USB: DATOS REALES":"USB: ESPERANDO AGENTE",live?GREEN:ORANGE);
  }
}
void paint(){
  if(fullRedraw){
    if(frameReady){screen=&frame;frame.fillScreen(BG);}else screen=&lcd;
    switch(page){case 0:home();break;case 1:network();break;case 2:eventsPage();break;default:systemPage();}
    if(frameReady)lcd.drawRGBBitmap(0,0,frame.getBuffer(),240,320);
    fullRedraw=false;
  }else updateDynamic();
}
// JSON flat keys, strings without escape decoding; bounded buffers.
// Agent guarantees ASCII and no embedded newlines or quotes in values.
const char* field(const char* json,const char* key){
  static char pattern[32];snprintf(pattern,sizeof(pattern),"\"%s\":",key);
  const char* p=strstr(json,pattern);if(!p)return nullptr;
  p+=strlen(pattern);while(*p==' '||*p=='\t')p++;return p;
}
int number(const char* json,const char* key,int fallback){
  const char* p=field(json,key);if(!p||!(*p=='-'||(*p>='0'&&*p<='9')))return fallback;
  return constrain(atoi(p),-1,1000000);
}
void stringField(const char* json,const char* key,char* out,size_t cap){
  const char* p=field(json,key);if(!p||*p!='\"')return;
  p++;size_t n=0;
  while(*p && *p!='\"' && n+1<cap){
    if((unsigned char)*p>=32 && (unsigned char)*p<=126)out[n++]=*p;
    p++;
  }
  out[n]=0;
}
void ingest(const char* json){
  if(!strstr(json,"\"type\":\"telemetry\""))return;
  pc.cpu=number(json,"cpu",-1);pc.ram=number(json,"ram",-1);
  pc.disk=number(json,"disk",-1);pc.connections=number(json,"connections",-1);
  pc.defender=number(json,"defender",-1);pc.events=number(json,"events",-1);
  pc.down=number(json,"down",-1);pc.up=number(json,"up",-1);
  stringField(json,"host",pc.host,sizeof(pc.host));
  stringField(json,"event",pc.eventText,sizeof(pc.eventText));
  pc.last=millis();if(pc.last==0)pc.last=1;
  needsDraw=true;
}
void serialPoll(){
  int budget=480;
  while(Serial.available()&&budget-->0){
    char c=(char)Serial.read();
    if(c=='\n'){
      lineBuf[lineLen]=0;if(lineLen)ingest(lineBuf);lineLen=0;
    }else if(c!='\r'){
      if(lineLen<sizeof(lineBuf)-1)lineBuf[lineLen++]=c;
      else lineLen=0; // reject overlong frames
    }
  }
}
void calibrationTarget(int x,int y,int index){
  lcd.fillScreen(BG);
  lcd.setTextColor(CYAN,BG);lcd.setTextSize(2);lcd.setCursor(8,10);
  lcd.print("CALIBRAR TACTIL");
  lcd.setTextSize(1);lcd.setCursor(8,42);
  lcd.print("Toca el centro de la cruz ");lcd.print(index);lcd.print("/3");
  lcd.drawCircle(x,y,12,YELLOW);
  lcd.drawFastHLine(x-18,y,37,YELLOW);
  lcd.drawFastVLine(x,y-18,37,YELLOW);
}
bool collectCalibration(int x,int y,int index,int32_t &rx,int32_t &ry){
  calibrationTarget(x,y,index);
  uint32_t start=millis();
  while(touch.touched() && millis()-start<1500)delay(10);
  while(!touch.touched()){
    if(millis()-start>20000)return false;
    delay(10);
  }
  // Median-like average of a few stable samples while pressed.
  int64_t sx=0,sy=0;int n=0;
  for(int i=0;i<12;i++){
    if(touch.touched()){
      TS_Point pt=touch.getPoint();
      if(pt.z>100){sx+=pt.x;sy+=pt.y;n++;}
    }
    delay(15);
  }
  if(n<3)return false;
  rx=sx/n;ry=sy/n;
  while(touch.touched() && millis()-start<22000)delay(10);
  delay(200);
  return true;
}
void runCalibration(){
  int32_t x0,y0,x1,y1,x2,y2;
  bool ok=collectCalibration(25,70,1,x0,y0)
       && collectCalibration(215,70,2,x1,y1)
       && collectCalibration(25,250,3,x2,y2);
  if(ok){
    calX0=x0;calY0=y0;calX1=x1;calY1=y1;calX2=x2;calY2=y2;
    // Reject degenerate points and avoid division by zero.
    int64_t det=(int64_t)(x1-x0)*(y2-y0)-(int64_t)(x2-x0)*(y1-y0);
    ok=(det>30000 || det< -30000);
  }
  if(ok){
    touchPrefs.begin("ggtouch",false);
    touchPrefs.putInt("x0",calX0);touchPrefs.putInt("y0",calY0);
    touchPrefs.putInt("x1",calX1);touchPrefs.putInt("y1",calY1);
    touchPrefs.putInt("x2",calX2);touchPrefs.putInt("y2",calY2);
    touchPrefs.putBool("valid",true);touchPrefs.end();
    calibrated=true;
    lcd.fillScreen(BG);lcd.setCursor(20,130);lcd.setTextSize(2);
    lcd.setTextColor(GREEN);lcd.print("CALIBRADO OK");delay(800);
  }else{
    lcd.fillScreen(BG);lcd.setCursor(20,130);lcd.setTextSize(2);
    lcd.setTextColor(ORANGE);lcd.print("REINTENTAR");delay(800);
  }
  prevTouch=false;fullRedraw=true;needsDraw=true;lastPaint=0;
}
void loadCalibration(){
  touchPrefs.begin("ggtouch",true);
  calibrated=touchPrefs.getBool("valid",false);
  if(calibrated){
    calX0=touchPrefs.getInt("x0",0);calY0=touchPrefs.getInt("y0",0);
    calX1=touchPrefs.getInt("x1",0);calY1=touchPrefs.getInt("y1",0);
    calX2=touchPrefs.getInt("x2",0);calY2=touchPrefs.getInt("y2",0);
  }
  touchPrefs.end();
}
bool mapTouch(TS_Point p,int &x,int &y){
  if(!calibrated)return false;
  int64_t ax=calX1-calX0,ay=calY1-calY0;
  int64_t bx=calX2-calX0,by=calY2-calY0;
  int64_t det=ax*by-bx*ay;
  if(det>-30000 && det<30000)return false;
  int64_t dx=p.x-calX0,dy=p.y-calY0;
  // Three-point affine mapping from raw coordinates to LCD pixels.
  x=25+(int)((190*(dx*by-dy*bx))/det);
  y=70+(int)((180*(ax*dy-ay*dx))/det);
  x=constrain(x,0,239);y=constrain(y,0,319);
  return true;
}
void touchPoll(){
  if(!touchOK)return;
  bool down=touch.touched();
  uint32_t now=millis();
  if(down && !prevTouch)touchDownAt=now;
  if(!down && prevTouch && now-lastTap>230){
    // Only one navigation action per released touch.
    // Read coordinate at release from the last stable touch sample.
    extern int lastTouchX,lastTouchY;
    if(calibrated && lastTouchY>=276 && now-touchDownAt>=35 && now-touchDownAt<1800){
      uint8_t target=constrain(lastTouchX/60,0,3);
      if(target!=page){page=target;fullRedraw=true;needsDraw=true;}
      lastTap=now;
    }
  }
  if(down && calibrated){
    TS_Point p=touch.getPoint();
    int x,y;
    if(p.z>100 && mapTouch(p,x,y)){
      extern int lastTouchX,lastTouchY;
      lastTouchX=x;lastTouchY=y;
    }
  }
  prevTouch=down;
}
int lastTouchX=-1,lastTouchY=-1;
void setup(){
  Serial.begin(115200);delay(200);
  pinMode(5,OUTPUT);digitalWrite(5,HIGH);
  pinMode(21,OUTPUT);digitalWrite(21,HIGH);
  lcdBus.begin(14,12,13,15);
  lcd.begin(20000000);lcd.setRotation(0);lcd.setTextWrap(false);
  touchBus.begin(25,39,32,33);touchOK=touch.begin(touchBus);touch.setRotation(0);
  if(touchOK){loadCalibration();if(!calibrated)runCalibration();}
  frameReady = false; // No 240x320 full-frame transfers: prevent SPI refresh sweep.
  screen=&lcd;
  lcd.fillScreen(BG);
  Serial.printf("GHOSTGUARD_FRAMEBUFFER=%s free_heap=%u\n",frameReady?"OK":"FALLBACK",(unsigned)ESP.getFreeHeap());
  Serial.printf("GHOSTGUARD_LIVE_READY v1.4 CYBER_GREEN screen=%dx%d touch=%d\n",lcd.width(),lcd.height(),touchOK);
  fullRedraw=true;needsDraw=true;
}
void loop(){
  serialPoll();
  // Send CAL over serial to repeat touch calibration when required.
  touchPoll();
  uint32_t now=millis();
  if(now-lastNetSample>1000){
    lastNetSample=now;
    memmove(netHistory,netHistory+1,sizeof(netHistory)-1);
    netHistory[31]=online()?(uint8_t)constrain((pc.down+pc.up)/8,0,100):0;
    if(page==1)needsDraw=true;
  }
  if(now-lastSecond>1500){lastSecond=now;if(page==3)needsDraw=true;}
  if(!online() && pc.last && now-pc.last<STALE_MS+1600 && now-pc.last>=STALE_MS)needsDraw=true;
  if(needsDraw&&now-lastPaint>=200){paint();lastPaint=now;needsDraw=false;}
}
