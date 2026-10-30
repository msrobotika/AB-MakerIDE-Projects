/* MS Robotika Â· Mini NAS CYD Â· ESP32-2432S028R, core 3.3.12.
   Local AP + authenticated web file manager. No formatting, no overwrite.
   Physical SD/AP transfers and controls validated; QR access has separate evidence. */
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <esp_system.h>
#include <errno.h>
#include "nas_rules.h"
#include "nas_selftest.h"
#include "nas_touch.h"
#include "nas_web.h"
#include "nas_qr.h"

SPIClass lcdBus(HSPI), sdBus(VSPI);
Adafruit_ILI9341 lcd(&lcdBus, 2, 15, -1);
WebServer server(80);
constexpr char ROOT[]="/MSR_NAS", TEMP[]="/MSR_NAS/.upload.part";
constexpr int NAS_BOOT_GPIO=0;
constexpr uint16_t BG=0x0864, PANEL=0x1129, CYAN=0x5F78, MUTED=0x8D16;
String apName, apPassword, writeToken, activity="Preparado", lastName;
String pairToken,webSession;
uint32_t pairDeadline=0;
bool pairAvailable=false;
enum class QrView:uint8_t {Status,Wifi,Web};
QrView qrView=QrView::Wifi;
bool apReady=false, sdReady=false, showKey=false, dirty=true, usbCalibrationRequested=false;
uint64_t totalBytes=0, usedBytes=0;
uint32_t keyDeadline=0, lastPaint=0, lastTouch=0, lastStats=0;
unsigned previousClients=999;
nas::TouchGate touchGate;
uint32_t touchSamples=0,touchValid=0,touchActions=0;
uint32_t uploadStarts=0,uploadFailures=0;
size_t lastWriteRequested=0,lastWriteActual=0;
int lastWriteErrno=0;
// This card/board produced short writes (EIO) at 4 MHz; 1 MHz passed read-back.
uint32_t sdClock=1000000;
int lastTouchX=-1,lastTouchY=-1;
File uploadFile;
struct UploadState {
  bool active=false, finished=false;
  uint64_t expected=0, received=0;
  uint32_t lastProgress=0;
  int error=0;
  String message, name;
} transferState;

String randomHex(unsigned bytes) {
  String s;s.reserve(bytes*2);
  const char digits[]="0123456789ABCDEF";
  for(unsigned i=0;i<bytes;++i){uint8_t b=uint8_t(esp_random());s+=digits[b>>4];s+=digits[b&15];}
  return s;
}
String jsonString(const String& s) {
  String out="\"";
  for(size_t i=0;i<s.length();++i){uint8_t c=s[i];if(c=='"'||c=='\\'){out+='\\';out+=char(c);}else if(c>=32)out+=char(c);}
  return out+'"';
}
String pathFor(const String& name) {return String(ROOT)+"/"+name;}
String sizeText(uint64_t n) {
  if(n>=1024ULL*1024*1024)return String(double(n)/(1024*1024*1024),1)+" GiB";
  if(n>=1024*1024)return String(double(n)/(1024*1024),1)+" MiB";
  if(n>=1024)return String(double(n)/1024,1)+" KiB";
  return String((unsigned long)n)+" B";
}
void refreshSpace() {
  if(sdReady){
    totalBytes=SD.totalBytes();usedBytes=SD.usedBytes();
    if(totalBytes==0){sdReady=false;usedBytes=0;activity="SD no disponible";}
    if(usedBytes>totalBytes)usedBytes=totalBytes;
  }
  else totalBytes=usedBytes=0;
  lastStats=millis();dirty=true;
}
void mountCard() {
  if(transferState.active)return;
  SD.end();sdReady=SD.begin(5,sdBus,sdClock,"/sd",6,false)&&SD.cardType()!=CARD_NONE;
  if(sdReady){
    if(!SD.exists(ROOT))sdReady=SD.mkdir(ROOT);
    if(sdReady){File dir=SD.open(ROOT);sdReady=dir&&dir.isDirectory();dir.close();}
  }
  activity=sdReady?"microSD disponible":"SD ausente o error";refreshSpace();
}
void text(int x,int y,const String& value,uint16_t color=ILI9341_WHITE,uint8_t size=1) {
  lcd.setCursor(x,y);lcd.setTextColor(color);lcd.setTextSize(size);lcd.print(value);
}
void button(int x,int y,int w,const char* value) {
  lcd.fillRoundRect(x,y,w,30,5,0x1A0C);lcd.drawRoundRect(x,y,w,30,5,0x2BD5);text(x+7,y+11,value,CYAN);
}
bool pairLive(){return nas::pairUsable(pairAvailable,millis(),pairDeadline);}
void openQrWeb(){
  if(!pairLive()){pairToken=randomHex(16);pairDeadline=millis()+300000;pairAvailable=true;nasqr::initializeWeb(pairToken);}
  qrView=QrView::Web;dirty=true;
}
void renderQr(){
  const bool wifi=qrView==QrView::Wifi;
  text(12,9,wifi?"1. WIFI DEL NAS":"2. ENTRA AL NAS",CYAN,2);
  const MSRQRCode& qr=wifi?nasqr::wifi:nasqr::web;
  const bool ready=wifi?nasqr::wifiReady:nasqr::webReady;
  if(ready){
    constexpr int x0=(240-nasqr::SIDE)/2,y0=36;
    lcd.fillRect(x0,y0,nasqr::SIDE,nasqr::SIDE,ILI9341_WHITE);
    lcd.startWrite();
    for(uint8_t y=0;y<qr.size;y++)for(uint8_t x=0;x<qr.size;x++)if(msr_qrcode_getModule(const_cast<MSRQRCode*>(&qr),x,y))
      lcd.writeFillRect(x0+(x+nasqr::QUIET)*nasqr::SCALE,y0+(y+nasqr::QUIET)*nasqr::SCALE,nasqr::SCALE,nasqr::SCALE,ILI9341_BLACK);
    lcd.endWrite();
  }else text(16,120,"No se pudo crear el QR",ILI9341_RED);
  text(12,224,wifi?apName:"Escanea tras conectar la WiFi",ILI9341_WHITE);
  text(12,239,wifi?"La clave va dentro del QR":"Sin escribir usuario ni clave",MUTED);
  text(12,254,wifi?"Despues toca QR WEB":(pairLive()?"Acceso de un uso / 5 minutos":"Toca QR WEB para renovarlo"),CYAN);
  button(10,280,68,"QR WIFI");button(85,280,64,"QR WEB");button(156,280,74,"ESTADO");
}
void render(bool force=false) {
  uint32_t now=millis();if(!force&&(!dirty||now-lastPaint<250))return;
  dirty=false;lastPaint=now;lcd.fillScreen(BG);
  if(qrView!=QrView::Status){renderQr();return;}
  text(12,10,"MS ROBOTIKA",CYAN);text(12,29,"MINI NAS",ILI9341_WHITE,3);
  text(177,13,apReady?"AP OK":"AP ERROR",apReady?ILI9341_GREEN:ILI9341_RED);
  lcd.fillRoundRect(10,65,220,69,8,PANEL);
  text(20,75,sdReady?"MICROSD / LIBRE":"MICROSD NO DISPONIBLE",CYAN);
  text(20,91,sdReady?sizeText(totalBytes-usedBytes):"SIN TARJETA",ILI9341_WHITE,2);
  if(sdReady){
    text(20,115,"de "+sizeText(totalBytes)+" / "+String(WiFi.softAPgetStationNum())+" cliente(s)",MUTED);
    lcd.fillRect(20,128,200,3,0x294D);if(totalBytes)lcd.fillRect(20,128,(unsigned)(200*usedBytes/totalBytes),3,CYAN);
  }else text(20,115,"FAT32 / no se formatea",MUTED);
  text(12,146,"RED: "+apName,CYAN);
  text(12,162,"http://192.168.4.1",ILI9341_WHITE,2);
  text(12,187,"Usuario web: maker",MUTED);
  text(12,201,showKey?"Clave: "+apPassword:"Clave: [toca CLAVE o BOOT]",showKey?ILI9341_YELLOW:MUTED);
  lcd.fillRoundRect(10,220,220,48,6,PANEL);
  text(18,228,activity.substring(0,33),CYAN);
  if(transferState.active){
    text(18,243,transferState.name.substring(0,32));
    unsigned percent=transferState.expected?unsigned(transferState.received*100/transferState.expected):100;
    lcd.fillRect(18,258,min(202U,202U*percent/100),3,CYAN);
  }else text(18,244,touchpad::calibrated?lastName.substring(0,32):"Toca para calibrar",MUTED);
  button(10,280,68,"QR WIFI");button(85,280,64,"SD");button(156,280,74,"CALIBRAR");
}
void calibrateTouch() {
  const int sx[]={25,215,120},sy[]={45,160,290};int rx[3],ry[3];
  bool old=touchpad::calibrated;
  for(int i=0;i<3;++i){
    lcd.fillScreen(BG);text(12,8,"CALIBRACION / "+String(i+1)+" DE 3",CYAN);
    text(12,24,"Toca la cruz. BOOT: omitir",MUTED);
    lcd.drawLine(sx[i]-9,sy[i],sx[i]+9,sy[i],ILI9341_WHITE);lcd.drawLine(sx[i],sy[i]-9,sx[i],sy[i]+9,ILI9341_WHITE);
    uint32_t start=millis();while(digitalRead(touchpad::IRQ)==LOW&&millis()-start<1500)delay(10);
    if(digitalRead(touchpad::IRQ)==LOW){activity="Suelta y vuelve a calibrar";dirty=true;return;}
    bool got=false;start=millis();
    while(millis()-start<10000){
      if(digitalRead(NAS_BOOT_GPIO)==LOW)break;
      if(touchpad::raw(rx[i],ry[i])){got=true;break;}delay(10);
    }
    if(!got){touchpad::calibrated=old;activity="Calibracion omitida";dirty=true;return;}
    delay(200);
  }
  activity=touchpad::fit(rx,ry,sx,sy)?"Tactil calibrado":"Repite la calibracion";dirty=true;
}
void headers() {
  server.sendHeader("Cache-Control","no-store");server.sendHeader("X-Content-Type-Options","nosniff");
  server.sendHeader("Referrer-Policy","no-referrer");
  server.sendHeader("Content-Security-Policy","default-src 'none'; script-src 'self'; style-src 'unsafe-inline'; img-src 'self'; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'; form-action 'self'");
}
void errorResponse(int code,const String& message) {headers();server.send(code,"application/json; charset=utf-8","{\"error\":"+jsonString(message)+"}");}
bool authenticated() {
  String cookie=server.header("Cookie");
  int start=0;
  while(start<int(cookie.length())){
    int end=cookie.indexOf(';',start);if(end<0)end=cookie.length();
    String item=cookie.substring(start,end);item.trim();
    if(webSession.length()==32&&item=="MSR_NAS="+webSession)return true;
    start=end+1;
  }
  return server.authenticate("maker",apPassword.c_str());
}
void pairHandler(){
  if(!pairLive()){errorResponse(410,"QR usado o caducado. Pulsa QR WEB en la placa.");return;}
  String received=server.header("X-NAS-Pair");
  if(received.length()!=32||received!=pairToken){errorResponse(403,"Acceso QR no valido.");return;}
  if(!nas::acceptPair(pairAvailable,millis(),pairDeadline,true)){errorResponse(410,"QR caducado. Pulsa QR WEB.");return;}
  pairToken="";dirty=true;
  headers();server.sendHeader("Set-Cookie","MSR_NAS="+webSession+"; HttpOnly; SameSite=Strict; Path=/");
  server.send(200,"application/json","{\"ok\":true}");
}
bool requireAuth() {
  if(authenticated())return true;
  headers();server.requestAuthentication(BASIC_AUTH,"MSR Mini NAS","Acceso privado: usuario maker y clave de la pantalla.");return false;
}
bool writeAllowed() {return server.header("X-NAS-Token")==writeToken&&writeToken.length()==32;}
bool requireWrite() {
  if(!requireAuth())return false;
  if(!writeAllowed()){errorResponse(403,"Token de escritura no valido. Actualiza la pagina.");return false;}return true;
}
bool requireCard() {if(sdReady)return true;errorResponse(503,"microSD ausente o no disponible.");return false;}
void statusHandler() {
  if(!requireAuth())return;
  String s="{\"sd_ready\":"+String(sdReady?"true":"false")+",\"total\":"+String((unsigned long long)totalBytes);
  s+=",\"used\":"+String((unsigned long long)usedBytes)+",\"free\":"+String((unsigned long long)(totalBytes-usedBytes));
  s+=",\"clients\":"+String(WiFi.softAPgetStationNum())+",\"max_file\":"+String((unsigned long long)nas::MAX_FILE);
  s+=",\"token\":"+jsonString(writeToken)+"}";headers();server.send(200,"application/json",s);
}
void filesHandler() {
  if(!requireAuth()||!requireCard())return;
  String raw=server.arg("offset");if(!raw.length())raw="0";
  if(raw.length()>5){errorResponse(400,"Pagina no valida.");return;}
  for(char c:raw)if(c<'0'||c>'9'){errorResponse(400,"Pagina no valida.");return;}
  unsigned offset=raw.toInt(),seen=0,count=0;bool more=false;
  File dir=SD.open(ROOT);if(!dir||!dir.isDirectory()){errorResponse(503,"No se puede leer la carpeta.");return;}
  String s="{\"files\":[";s.reserve(6400);
  for(File file=dir.openNextFile();file;file=dir.openNextFile()){
    String name=file.name();int slash=name.lastIndexOf('/');if(slash>=0)name=name.substring(slash+1);
    if(!file.isDirectory()&&nas::validName(name.c_str())){
      if(seen++>=offset){
        if(count==nas::PAGE_SIZE){more=true;file.close();break;}
        if(count++)s+=',';
        s+="{\"name\":"+jsonString(name)+",\"size\":"+String((unsigned long long)file.size())+"}";
      }
    }
    file.close();
  }
  dir.close();s+="],\"more\":"+String(more?"true":"false")+"}";headers();server.send(200,"application/json",s);
}
void cleanTemporary() {
  if(uploadFile)uploadFile.close();
  if(sdReady&&SD.exists(TEMP))SD.remove(TEMP);
  transferState.active=false;
}
void transferError(int code,const String& message) {
  if(!transferState.error){transferState.error=code;transferState.message=message;}
  cleanTemporary();activity="Subida no completada";dirty=true;
}
void uploadBody() {
  if(!server.hasRaw())return;  // Multipart is deliberately unsupported.
  HTTPRaw& raw=server.raw();
  if(raw.status==RAW_START){
    uploadStarts++;
    cleanTemporary();transferState=UploadState();transferState.lastProgress=millis();
    if(!authenticated()){transferError(401,"Autenticacion necesaria.");return;}
    if(!writeAllowed()){transferError(403,"Token de escritura no valido.");return;}
    if(!sdReady){transferError(503,"microSD no disponible.");return;}
    if(server.header("Content-Type")!="application/octet-stream"){transferError(415,"Envia un archivo binario.");return;}
    transferState.name=server.header("X-File-Name");
    if(!nas::validName(transferState.name.c_str())){transferError(400,"Nombre no valido. Usa letras sin tildes, numeros, puntos, guiones y espacios interiores.");return;}
    int declared=server.clientContentLength();
    if(!server.hasHeader("Content-Length")||declared<0){transferError(411,"Content-Length obligatorio.");return;}
    transferState.expected=uint64_t(declared);
    if(transferState.expected>nas::MAX_FILE){transferError(413,"El limite es 128 MiB por archivo.");return;}
    if(SD.exists(pathFor(transferState.name))){transferError(409,"Ese nombre ya existe. No se sobrescribe.");return;}
    refreshSpace();
    if(!nas::canStart(transferState.expected,totalBytes-usedBytes)){transferError(507,"Espacio insuficiente en la microSD.");return;}
    uploadFile=SD.open(TEMP,FILE_WRITE);
    if(!uploadFile){transferError(503,"No se puede escribir en la tarjeta.");return;}
    transferState.active=true;activity="SUBIENDO A MICROSD";qrView=QrView::Status;dirty=true;render(true);
  }else if(raw.status==RAW_WRITE){
    if(!transferState.active||transferState.error)return;
    if(!nas::canAppend(transferState.received,raw.currentSize,transferState.expected)){transferError(413,"Tamano recibido no valido.");return;}
    lastWriteRequested=raw.currentSize;errno=0;
    lastWriteActual=uploadFile.write(raw.buf,raw.currentSize);lastWriteErrno=errno;
    if(lastWriteActual!=raw.currentSize){
      uploadFailures++;
      Serial.printf("NAS_WRITE_FAIL REQUESTED=%u ACTUAL=%u ERRNO=%d RECEIVED=%llu EXPECTED=%llu CLOCK=%u\n",unsigned(lastWriteRequested),unsigned(lastWriteActual),lastWriteErrno,(unsigned long long)transferState.received,(unsigned long long)transferState.expected,unsigned(sdClock));
      transferError(507,"Fallo de escritura en microSD. Codigo "+String(lastWriteErrno)+". No se ha guardado el archivo.");return;
    }
    transferState.received+=raw.currentSize;transferState.lastProgress=millis();dirty=true;render();delay(0);
  }else if(raw.status==RAW_ABORTED){
    transferError(400,"Transferencia interrumpida.");refreshSpace();
  }else if(raw.status==RAW_END){
    if(transferState.error||!transferState.active)return;
    uploadFile.flush();
    bool exact=nas::complete(transferState.received,transferState.expected)&&uint64_t(uploadFile.size())==transferState.expected;
    uploadFile.close();
    if(!exact){transferError(400,"El archivo esta incompleto.");return;}
    transferState.finished=true; // Final route commits only after the complete body.
  }
}
void uploadHandler() {
  if(!requireWrite()){cleanTemporary();return;}
  if(!server.hasRaw()){errorResponse(415,"Subida binaria necesaria.");return;}
  if(transferState.error){errorResponse(transferState.error,transferState.message);return;}
  if(!transferState.finished||!transferState.active){cleanTemporary();errorResponse(400,"Subida incompleta.");return;}
  if(SD.exists(pathFor(transferState.name))){transferError(409,"Ese nombre ya existe.");errorResponse(409,transferState.message);return;}
  if(!SD.rename(TEMP,pathFor(transferState.name))){transferError(503,"No se pudo finalizar el archivo.");errorResponse(503,transferState.message);return;}
  transferState.active=false;lastName=transferState.name;activity="Archivo guardado";refreshSpace();
  headers();server.send(201,"application/json","{\"ok\":true,\"name\":"+jsonString(lastName)+"}");
}
void fileHandler(bool preview) {
  if(!requireAuth()||!requireCard())return;
  String name=server.arg("name");if(!nas::validName(name.c_str())){errorResponse(400,"Nombre no valido.");return;}
  String mime="application/octet-stream";
  if(preview){
    String ext=name;ext.toLowerCase();
    if(ext.endsWith(".jpg")||ext.endsWith(".jpeg"))mime="image/jpeg";
    else if(ext.endsWith(".png"))mime="image/png";
    else if(ext.endsWith(".webp"))mime="image/webp";
    else{errorResponse(415,"Este tipo se descarga como archivo.");return;}
  }
  File file=SD.open(pathFor(name),FILE_READ);if(!file||file.isDirectory()){errorResponse(404,"Archivo no encontrado.");return;}
  activity=preview?"Vista de imagen":"Descargando archivo";lastName=name;dirty=true;render(true);headers();
  server.sendHeader("Content-Disposition",String(preview?"inline":"attachment")+"; filename=\""+name+"\"");
  size_t expected=file.size();size_t sent=server.streamFile(file,mime);file.close();
  activity=sent==expected?"Transferencia completada":"Descarga interrumpida";dirty=true;
}
void deleteHandler() {
  if(!requireWrite()||!requireCard())return;
  String name=server.arg("name");if(!nas::validName(name.c_str())){errorResponse(400,"Nombre no valido.");return;}
  File file=SD.open(pathFor(name));if(!file||file.isDirectory()){errorResponse(404,"Archivo no encontrado.");return;}file.close();
  if(!SD.remove(pathFor(name))){errorResponse(503,"No se pudo borrar el archivo.");return;}
  activity="Archivo borrado";lastName=name;refreshSpace();headers();server.send(200,"application/json","{\"ok\":true}");
}
void serialStatus() {
  Serial.print("MSR_MININAS v0.1 AP=");Serial.print(apName);
  Serial.print(" SD_READY=");Serial.print(sdReady?1:0);
  Serial.print(" TOTAL=");Serial.print((unsigned long long)totalBytes);
  Serial.print(" USED=");Serial.println((unsigned long long)usedBytes);
  Serial.print("NAS_DIAG AP_READY=");Serial.print(apReady?1:0);
  Serial.print(" MODE=");Serial.print(int(WiFi.getMode()));
  Serial.print(" AP_STARTED=");Serial.print(WiFi.AP.started()?1:0);
  Serial.print(" AP_MATCH=");Serial.print(WiFi.softAPSSID()==apName?1:0);
  Serial.print(" IP=");Serial.print(WiFi.softAPIP());
  Serial.print(" TCAL=");Serial.print(touchpad::calibrated?1:0);
  Serial.print(" IRQ=");Serial.print(digitalRead(touchpad::IRQ));
  Serial.print(" BOOT=");Serial.print(digitalRead(NAS_BOOT_GPIO));
  Serial.print(" HEAP=");Serial.println(ESP.getFreeHeap());
  Serial.print("NAS_QR WIFI=");Serial.print(nasqr::wifiReady?1:0);Serial.print(" WEB=");Serial.println(nasqr::webReady?1:0);
  Serial.print("NAS_TOUCH SAMPLES=");Serial.print(touchSamples);Serial.print(" VALID=");Serial.print(touchValid);
  Serial.print(" ACTIONS=");Serial.print(touchActions);Serial.print(" X=");Serial.print(lastTouchX);Serial.print(" Y=");Serial.println(lastTouchY);
  Serial.printf("NAS_UPLOAD STARTS=%u FAILURES=%u REQUESTED=%u ACTUAL=%u ERRNO=%d CLOCK=%u\n",unsigned(uploadStarts),unsigned(uploadFailures),unsigned(lastWriteRequested),unsigned(lastWriteActual),lastWriteErrno,unsigned(sdClock));
}
bool sdProbe(size_t chunk,uint32_t length){
  const String path=String(ROOT)+"/nas-probe-"+randomHex(8)+".dat";
  if(!sdReady||SD.exists(path)){Serial.println("NAS_SD_PROBE PRECONDITION_FAILED");return false;}
  uint8_t buffer[1436];uint32_t written=0,checked=0;int writeError=0;bool verified=false;
  errno=0;File probe=SD.open(path,FILE_WRITE);
  if(!probe){Serial.printf("NAS_SD_PROBE OPEN_FAILED ERRNO=%d\n",errno);return false;}
  while(written<length){
    size_t n=min(chunk,size_t(length-written));
    for(size_t i=0;i<n;i++)buffer[i]=uint8_t((written+i)*31U+17U);
    errno=0;size_t got=probe.write(buffer,n);writeError=errno;written+=got;
    if(got!=n)break;
    delay(0);
  }
  errno=0;probe.flush();int flushError=errno;probe.close();
  probe=SD.open(path,FILE_READ);
  if(probe&&probe.size()==length&&written==length){
    verified=true;
    while(checked<length){
      size_t n=min(sizeof(buffer),size_t(length-checked));
      size_t got=probe.read(buffer,n);
      if(got!=n){verified=false;break;}
      for(size_t i=0;i<n;i++)if(buffer[i]!=uint8_t((checked+i)*31U+17U)){verified=false;break;}
      if(!verified)break;
      checked+=got;delay(0);
    }
  }
  probe.close();bool removed=SD.remove(path);refreshSpace();
  Serial.printf("NAS_SD_PROBE CLOCK=%u CHUNK=%u LENGTH=%u WRITTEN=%u CHECKED=%u WRITE_ERRNO=%d FLUSH_ERRNO=%d VERIFIED=%u CLEANED=%u\n",unsigned(sdClock),unsigned(chunk),unsigned(length),unsigned(written),unsigned(checked),writeError,flushError,verified?1:0,removed?1:0);
  return verified&&removed;
}
void sdDiagnostic(){
  if(transferState.active){Serial.println("NAS_SD_TEST BUSY");return;}
  bool a=sdProbe(1436,262161),b=sdProbe(512,262161);
  if((!a||!b)&&sdClock>1000000){sdClock=1000000;mountCard();a=sdProbe(1436,262161);b=sdProbe(512,262161);}
  Serial.printf("NAS_SD_TEST COMPLETE=%u CLOCK=%u\n",a&&b?1:0,unsigned(sdClock));
}
void setup() {
  Serial.begin(115200);
  pinMode(NAS_BOOT_GPIO,INPUT_PULLUP);pinMode(5,OUTPUT);digitalWrite(5,HIGH);pinMode(33,OUTPUT);digitalWrite(33,HIGH);
  pinMode(21,OUTPUT);digitalWrite(21,HIGH);
  lcdBus.begin(14,12,13,15);lcd.begin(40000000);lcd.setRotation(0);lcd.setTextWrap(false);
  touchpad::begin();if(!touchpad::calibrated)calibrateTouch();
  sdBus.begin(18,19,23,5);mountCard();
  WiFi.persistent(false);WiFi.mode(WIFI_OFF);delay(100);
  apName="MSR-NAS-"+randomHex(2);apPassword=randomHex(8);writeToken=randomHex(16);webSession=randomHex(16);
  nasqr::initializeWifi(apName,apPassword);
  IPAddress ip(192,168,4,1);
  apReady=WiFi.mode(WIFI_AP)&&WiFi.softAP(apName.c_str(),apPassword.c_str(),1,false,4,false,WIFI_AUTH_WPA2_PSK,WIFI_CIPHER_TYPE_CCMP);
  if(apReady)apReady=WiFi.softAPConfig(ip,ip,IPAddress(255,255,255,0),IPAddress(192,168,4,2),ip);
  const char* keys[]={"X-NAS-Token","X-File-Name","Content-Type","Content-Length","Cookie","X-NAS-Pair"};server.collectHeaders(keys,6);
  server.on("/",HTTP_GET,[]{if(server.arg("pair")=="1"||requireAuth()){headers();server.send_P(200,"text/html; charset=utf-8",NAS_HTML);}});
  server.on("/app.js",HTTP_GET,[]{headers();server.send_P(200,"application/javascript; charset=utf-8",NAS_JS);});
  server.on("/api/pair",HTTP_POST,pairHandler);
  server.on("/api/status",HTTP_GET,statusHandler);server.on("/api/files",HTTP_GET,filesHandler);
  server.on("/api/upload",HTTP_POST,uploadHandler,uploadBody);
  server.on("/api/download",HTTP_GET,[]{fileHandler(false);});server.on("/api/preview",HTTP_GET,[]{fileHandler(true);});
  server.on("/api/delete",HTTP_POST,deleteHandler);
  server.onNotFound([]{if(requireAuth())errorResponse(404,"Ruta no encontrada.");});
  if(apReady)server.begin();showKey=true;keyDeadline=millis()+90000;dirty=true;render(true);serialStatus();
}
void loop() {
  if(apReady)server.handleClient();
  uint32_t now=millis();
  if(transferState.active&&now-transferState.lastProgress>30000){transferError(408,"Subida interrumpida por tiempo de espera.");refreshSpace();}
  if(showKey&&int32_t(now-keyDeadline)>=0){showKey=false;dirty=true;}
  if(now-lastStats>15000&&!transferState.active)refreshSpace();
  unsigned clients=WiFi.softAPgetStationNum();if(clients!=previousClients){previousClients=clients;dirty=true;}
  static bool bootDown=false, bootLong=false;static uint32_t bootStart=0;
  bool down=digitalRead(NAS_BOOT_GPIO)==LOW;
  if(down&&!bootDown){bootStart=now;bootLong=false;qrView=QrView::Wifi;showKey=true;keyDeadline=now+30000;dirty=true;}
  if(down&&!bootLong&&now-bootStart>=2000&&!transferState.active){bootLong=true;usbCalibrationRequested=true;}
  bootDown=down;
  int x,y;bool pressed=digitalRead(touchpad::IRQ)==LOW;touchGate.update(pressed,now);
  if(!transferState.active&&touchGate.sample(now)){
    if(!touchpad::calibrated){touchGate.consume();usbCalibrationRequested=true;}
    else{
      touchSamples++;
      if(touchpad::point(x,y)){
        touchValid++;lastTouchX=x;lastTouchY=y;touchGate.consume();
        if(y>=280&&now-lastTouch>250){
          lastTouch=now;touchActions++;
          if(qrView!=QrView::Status){
            if(x<80){qrView=QrView::Wifi;dirty=true;}
            else if(x<153)openQrWeb();else{qrView=QrView::Status;showKey=true;keyDeadline=now+90000;dirty=true;}
          }else if(x<80){qrView=QrView::Wifi;dirty=true;}
          else if(x<153)mountCard();else calibrateTouch();
        }
      }
    }
  }
  static char command[17];static unsigned commandSize=0;
  while(Serial.available()){
    char c=Serial.read();
    if(c=='\n'||c=='\r'){
      command[commandSize]=0;
      if(strcmp(command,"STATUS")==0)serialStatus();
      else if(strcmp(command,"SHOWKEY")==0){showKey=true;keyDeadline=millis()+90000;dirty=true;Serial.println("NAS_KEY_DISPLAYED");}
      else if(strcmp(command,"CALIBRATE")==0)usbCalibrationRequested=true;
      else if(strcmp(command,"QRWIFI")==0){qrView=QrView::Wifi;dirty=true;}
      else if(strcmp(command,"QRWEB")==0)openQrWeb();
      else if(strcmp(command,"QRTEST")==0)nasqr::serialDemo();
      else if(strcmp(command,"SDTEST")==0)sdDiagnostic();
      commandSize=0;
    }
    else if(commandSize<16)command[commandSize++]=c;
    else commandSize=0;
  }
  if(usbCalibrationRequested&&!transferState.active&&!down){usbCalibrationRequested=false;calibrateTouch();keyDeadline=millis()+90000;showKey=true;dirty=true;}
  render();delay(2);
}
