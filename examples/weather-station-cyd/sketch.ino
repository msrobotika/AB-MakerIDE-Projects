/* Estacion Wi-Fi para CYD ESP32-2432S028R, ILI9341 + XPT2046.
   Open-Meteo: datos de modelo, uso personal no comercial, CC BY 4.0.
   Configuracion web auxiliar; toda la meteorologia se dibuja en el LCD.
   Perfil DECLARADO; no identifica ni verifica el hardware conectado. */
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "weather_model.h"
#include "weather_ca.h"

SPIClass lcdBus(HSPI), touchBus(VSPI);
Adafruit_ILI9341 lcd(&lcdBus, 2, 15, -1);
XPT2046_Touchscreen touch(33, 36);
Preferences prefs;
WiFiManager wm;
WiFiManagerParameter cityParam("city","Ciudad (ej. Sevilla, Espana)","",80);
WiFiManagerParameter countryParam("country","Pais ISO opcional (ES, MX, AR...)","",2);
QueueHandle_t requestQueue=nullptr, resultQueue=nullptr;
meteo::Request settings={};
meteo::Weather weather={};
bool hasWeather=false, busy=false, attempted=false, dirty=true, workerReady=false;
bool lastFetchOk=false, locationChanged=false;
bool lastConnected=false, lastPortal=false, pressed=false, longHandled=false;
uint32_t lastAttempt=0, lastSuccess=0, lastDraw=0, pressStart=0, offlineSince=0;
char statusText[80]="Configura Wi-Fi y ciudad";
char apName[32], apPassword[17];
float ax=0,bx=0,cx=0,ay=0,by=0,cy=0;
bool calibrated=false;
constexpr uint16_t BG=0x0843, CARD=0x10C6, MUTED=0xAD75, ACCENT=0x4E9F;

void status(const char *msg) { snprintf(statusText,sizeof(statusText),"%s",msg); dirty=true; }
// El tipo de letra GFX integrado es de un byte: transliterar texto UTF-8 del proveedor.
String screenText(const char *src, size_t limit=37) {
  String s;
  while (*src && s.length()<limit) {
    uint8_t c=*src++;
    if (c<128) { s+=char(c>=32?c:' '); continue; }
    char mapped='?';
    if (c==0xC3 && *src) {
      uint8_t b=*src++;
      switch(b) {
        case 0xA1:case 0xA0:case 0xA4:case 0xA2:mapped='a';break;
        case 0xA9:case 0xA8:case 0xAB:case 0xAA:mapped='e';break;
        case 0xAD:case 0xAC:case 0xAF:case 0xAE:mapped='i';break;
        case 0xB3:case 0xB2:case 0xB6:case 0xB4:mapped='o';break;
        case 0xBA:case 0xB9:case 0xBC:case 0xBB:mapped='u';break;
        case 0xB1:mapped='n';break; case 0x91:mapped='N';break;
        case 0x81:case 0x80:case 0x84:mapped='A';break;
        case 0x89:case 0x88:case 0x8B:mapped='E';break;
        case 0x8D:case 0x8C:case 0x8F:mapped='I';break;
        case 0x93:case 0x92:case 0x96:mapped='O';break;
        case 0x9A:case 0x99:case 0x9C:mapped='U';break;
      }
    } else while ((*src & 0xC0)==0x80) ++src;
    s+=mapped;
  }
  return s;
}
void text(int x,int y,const char *s,uint16_t color=ILI9341_WHITE,uint8_t size=1) {
  lcd.setTextSize(size); lcd.setTextColor(color); lcd.setCursor(x,y); lcd.print(s);
}
void button(int x,int width,const char *label) {
  lcd.fillRoundRect(x,278,width,27,4,0x19C9);text(x+8,288,label);
}
bool sampleRaw(int &x,int &y) {
  if (!touch.tirqTouched() || !touch.touched()) return false;
  TS_Point p=touch.getPoint(); if(p.z<250) return false;
  x=p.x; y=p.y; return true;
}
bool calibrate() {
  // Misma transformacion afin y cruces que la referencia; no bloquear indefinidamente.
  const int sx[3]={20,220,20},sy[3]={20,20,300}; int rx[3],ry[3];
  for (int point=0;point<3;++point) {
    lcd.fillScreen(BG);text(30,108,"CALIBRAR TACTIL",ACCENT,2);
    text(24,140,"Toca el centro de la cruz");text(24,160,"15 s sin tocar: omitir");
    lcd.drawFastHLine(sx[point]-10,sy[point],21,ILI9341_YELLOW);
    lcd.drawFastVLine(sx[point],sy[point]-10,21,ILI9341_YELLOW);
    lcd.drawCircle(sx[point],sy[point],8,ILI9341_YELLOW);
    uint32_t start=millis(); int x,y,count=0; int32_t sumx=0,sumy=0;
    while(sampleRaw(x,y)) { if(meteo::elapsed(millis(),start,15000)) return false; delay(10); }
    while(count<12) {
      if(meteo::elapsed(millis(),start,15000)) return false;
      if(sampleRaw(x,y)){sumx+=x;sumy+=y;++count;}else{count=0;sumx=sumy=0;}
      delay(12);
    }
    rx[point]=sumx/count;ry[point]=sumy/count;
    while(sampleRaw(x,y)) { if(meteo::elapsed(millis(),start,15000)) return false; delay(10); }
    delay(150);
  }
  float ux=rx[1]-rx[0],uy=ry[1]-ry[0],vx=rx[2]-rx[0],vy=ry[2]-ry[0],det=ux*vy-vx*uy;
  if(fabsf(det)<10000) return false;
  ax=((sx[1]-sx[0])*vy-(sx[2]-sx[0])*uy)/det;
  bx=(ux*(sx[2]-sx[0])-vx*(sx[1]-sx[0]))/det;cx=sx[0]-ax*rx[0]-bx*ry[0];
  ay=((sy[1]-sy[0])*vy-(sy[2]-sy[0])*uy)/det;
  by=(ux*(sy[2]-sy[0])-vx*(sy[1]-sy[0]))/det;cy=sy[0]-ay*rx[0]-by*ry[0];
  float coefficients[]={ax,bx,cx,ay,by,cy};
  return prefs.putBytes("touch",coefficients,sizeof(coefficients))==sizeof(coefficients);
}
String urlEncode(const char *s) {
  const char hex[]="0123456789ABCDEF";String out;
  while(*s) { uint8_t c=*s++; if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
    (c>='0'&&c<='9')||c=='-'||c=='_'||c=='.')out+=char(c);
    else {out+='%';out+=hex[c>>4];out+=hex[c&15];} }
  return out;
}
class LimitedStream : public Stream {
  Stream &source; size_t left;
public:
  LimitedStream(Stream &stream,size_t limit):source(stream),left(limit){setTimeout(12000);}
  int available() override {return int(min(size_t(max(source.available(),0)),left));}
  int read() override {if(!left)return -1;int c=source.read();if(c>=0)--left;return c;}
  int peek() override {return left?source.peek():-1;}
  void flush() override {}
  size_t write(uint8_t) override {return 0;}
};
bool getJson(const String &url,JsonDocument &doc,char *error,size_t errorSize) {
  WiFiClientSecure tls;tls.setCACert(WEATHER_ROOT_CA);tls.setHandshakeTimeout(12);
  HTTPClient http;http.setConnectTimeout(10000);http.setTimeout(12000);
  http.useHTTP10(true); // Evitar chunked al deserializar el stream directamente.
  if(!http.begin(tls,url)){snprintf(error,errorSize,"No se pudo iniciar HTTPS");return false;}
  http.addHeader("Accept","application/json");http.addHeader("Accept-Encoding","identity");
  const int code=http.GET();
  if(code!=HTTP_CODE_OK) {
    if(code==429)snprintf(error,errorSize,"Limite API; reintento en 2 min");
    else snprintf(error,errorSize,"API HTTP %d; reintentando",code);
    http.end();return false;
  }
  // La consulta acotada debe ser pequena; rechazar respuestas inesperadas grandes.
  if(http.getSize()>16384){snprintf(error,errorSize,"Respuesta API demasiado grande");http.end();return false;}
  LimitedStream body(http.getStream(),16384);
  auto result=deserializeJson(doc,body,DeserializationOption::NestingLimit(8));
  http.end();
  if(result){snprintf(error,errorSize,"JSON incompleto o invalido");return false;}
  return true;
}
void networkWorker(void *) {
  meteo::Request req; char cachedCity[81]="",cachedCountry[3]="";
  char place[81]="",region[97]="";float latitude=0,longitude=0;
  for(;;) {
    if(xQueueReceive(requestQueue,&req,portMAX_DELAY)!=pdTRUE)continue;
    meteo::Result result={};result.generation=req.generation;
    do {
      if(WiFi.status()!=WL_CONNECTED){snprintf(result.error,sizeof(result.error),"Sin Wi-Fi; reconectando");break;}
      const uint32_t start=millis();
      while(time(nullptr)<1735689600 && !meteo::elapsed(millis(),start,15000))vTaskDelay(pdMS_TO_TICKS(100));
      if(time(nullptr)<1735689600){snprintf(result.error,sizeof(result.error),"Sin hora NTP; revisa Internet");break;}
      JsonDocument doc;
      if(strcmp(cachedCity,req.city)||strcmp(cachedCountry,req.country)) {
        String url="https://geocoding-api.open-meteo.com/v1/search?count=1&language=es&name="+urlEncode(req.city);
        if(req.country[0])url+="&countryCode="+String(req.country);
        if(!getJson(url,doc,result.error,sizeof(result.error)))break;
        JsonVariantConst first=doc["results"][0];
        const char *name=first["name"] | "";
        float candidateLatitude=0,candidateLongitude=0;
        if(!name[0] || !meteo::number(first["latitude"],-90,90,candidateLatitude) ||
          !meteo::number(first["longitude"],-180,180,candidateLongitude)) {
          snprintf(result.error,sizeof(result.error),"Ciudad no encontrada; revisa WIFI");break;
        }
        latitude=candidateLatitude;longitude=candidateLongitude;
        snprintf(place,sizeof(place),"%s",name);
        snprintf(region,sizeof(region),"%s / %s",first["admin1"] | "",first["country_code"] | "");
        snprintf(cachedCity,sizeof(cachedCity),"%s",req.city);snprintf(cachedCountry,sizeof(cachedCountry),"%s",req.country);
        doc.clear();
      }
      char coords[100];snprintf(coords,sizeof(coords),"?latitude=%.5f&longitude=%.5f",latitude,longitude);
      String url=String("https://api.open-meteo.com/v1/forecast")+coords+
        "&current=temperature_2m,apparent_temperature,relative_humidity_2m,wind_speed_10m,precipitation,pressure_msl,weather_code"
        "&daily=temperature_2m_min,temperature_2m_max,precipitation_probability_max,weather_code"
        "&forecast_days=3&timezone=auto&temperature_unit=celsius&wind_speed_unit=kmh&precipitation_unit=mm";
      if(!getJson(url,doc,result.error,sizeof(result.error)))break;
      if(!meteo::parseWeather(doc,result.weather)){snprintf(result.error,sizeof(result.error),"Datos API incompletos; reintento");break;}
      snprintf(result.weather.place,sizeof(result.weather.place),"%s",place);
      snprintf(result.weather.region,sizeof(result.weather.region),"%s",region);result.ok=true;
    }while(false);
    xQueueOverwrite(resultQueue,&result);
  }
}
void saveParameters() {
  String city=cityParam.getValue(),country=countryParam.getValue();city.trim();country.trim();country.toUpperCase();
  bool valid=city.length()>=2 && city.length()<=80 && (country.isEmpty() ||
    (country.length()==2 && country[0]>='A' && country[0]<='Z' && country[1]>='A' && country[1]<='Z'));
  for(size_t i=0;i<city.length();++i)if(uint8_t(city[i])<32 || city[i]=='<' || city[i]=='>' || city[i]=='"' || city[i]=='\'')valid=false;
  if(!valid){status("Ciudad/pais invalidos; revisa WIFI");return;}
  if(city==settings.city && country==settings.country)return;
  // Un solo blob para no mezclar ciudad y pais ante un corte de alimentacion.
  meteo::Request next={};snprintf(next.city,sizeof(next.city),"%s",city.c_str());
  snprintf(next.country,sizeof(next.country),"%s",country.c_str());next.generation=settings.generation+1;
  if(prefs.putBytes("location",&next,sizeof(next))!=sizeof(next)){status("Error guardando ubicacion en NVS");return;}
  settings=next;hasWeather=false;lastFetchOk=false;locationChanged=true;status("Ubicacion guardada; consultando");
  // Respetar el limite global incluso si cambia la ubicacion repetidamente.
}
void togglePortal() {
  if(wm.getConfigPortalActive()) {wm.stopConfigPortal();offlineSince=millis();status("Configuracion cerrada");}
  else {wm.startConfigPortal(apName,apPassword);dirty=true;}
}
void requestUpdate(bool manual=false) {
  const uint32_t now=millis();
  if(!workerReady){status("Error interno de tarea de red");return;}
  if(!settings.city[0]){status("Introduce tu ciudad en WIFI");return;}
  if(WiFi.status()!=WL_CONNECTED){status("Sin Wi-Fi; abre WIFI para configurar");return;}
  if(busy)return;
  if(attempted && !meteo::elapsed(now,lastAttempt,meteo::MANUAL_MS)) {
    if(manual)status("Espera 60 s entre consultas");return;
  }
  busy=true;attempted=true;locationChanged=false;lastAttempt=now;status("Consultando Open-Meteo...");
  xQueueOverwrite(requestQueue,&settings);
}
void drawIcon(int code) {
  const bool clear=code<=2, wet=(code>=51 && code<=99);
  if(clear){lcd.fillCircle(205,68,14,ILI9341_YELLOW);for(int i=0;i<8;i++) {
    float a=i*PI/4;lcd.drawLine(205+cosf(a)*18,68+sinf(a)*18,205+cosf(a)*23,68+sinf(a)*23,ILI9341_YELLOW);}}
  if(code>0){lcd.fillCircle(195,75,10,MUTED);lcd.fillCircle(208,72,12,MUTED);lcd.fillRoundRect(182,74,41,15,6,MUTED);}
  if(wet)for(int i=0;i<3;i++)lcd.drawLine(189+i*12,94,185+i*12,102,ACCENT);
}
void render() {
  // Repintar solo por cambios y cada minuto, nunca en cada iteracion.
  lcd.fillScreen(BG);text(10,8,"METEO",ACCENT,2);
  const bool online=WiFi.status()==WL_CONNECTED;
  text(170,12,online?"WiFi OK":"SIN WiFi",online?ILI9341_GREEN:ILI9341_ORANGE);
  char line[80];
  if(wm.getConfigPortalActive()) {
    text(12,42,"CONFIGURAR",ACCENT,2);
    text(12,74,"1. Conecta movil/PC a:");text(12,94,apName,ILI9341_YELLOW);
    text(12,120,"Clave de esta sesion:");text(12,140,apPassword,ILI9341_YELLOW,2);
    text(12,172,"2. Abre http://192.168.4.1");text(12,194,"3. Configure WiFi / ciudad");
    text(12,216,"Red domestica de 2.4 GHz");
    text(12,238,"Mantener 2 s: cerrar portal");
  } else if(hasWeather) {
    text(10,31,screenText(weather.place,18).c_str(),ILI9341_WHITE,2);
    text(10,50,screenText(weather.region,36).c_str(),MUTED);
    snprintf(line,sizeof(line),"%.1f",weather.temperature);text(10,66,line,ILI9341_WHITE,4);
    int tx=10+strlen(line)*24;text(tx+3,72,"C",ACCENT,2);drawIcon(weather.code);
    text(10,104,meteo::condition(weather.code),ACCENT);
    lcd.fillRoundRect(8,120,224,57,5,CARD);
    snprintf(line,sizeof(line),"Humedad %3.0f%%   Viento %4.1f km/h",weather.humidity,weather.wind);text(14,128,line);
    snprintf(line,sizeof(line),"Sensacion %.1f C / %.0f hPa",weather.apparent,weather.pressure);text(14,145,line);
    snprintf(line,sizeof(line),"Precip. actual %.1f mm",weather.rain);text(14,162,line);
    text(10,187,"FECHA   MIN / MAX    LLUVIA",MUTED);
    for(int i=0;i<3;i++) {
      auto &d=weather.days[i];snprintf(line,sizeof(line),"%.2s/%.2s %5.1f/%5.1f C %3.0f%%",d.date+8,d.date+5,d.low,d.high,d.rainChance);
      text(10,202+i*15,line,i?ILI9341_WHITE:ACCENT);
    }
    snprintf(line,sizeof(line),"Dato %.5s / %.10s",weather.time+11,weather.time);text(10,250,line,MUTED);
  } else {
    text(16,65,"TU TIEMPO",ACCENT,3);text(16,104,"EN LA PANTALLA",ILI9341_WHITE,2);
    text(16,151,"Abre WIFI para configurar");text(16,171,"tu red y tu ciudad.");
    text(16,207,"Datos del modelo Open-Meteo");text(16,225,"Sin sensores externos");
  }
  text(8,266,screenText(statusText,37).c_str(),ILI9341_ORANGE);
  button(8,139,"ACTUALIZAR");button(155,77,wm.getConfigPortalActive()?"CERRAR":"WIFI");
  if(hasWeather && !wm.getConfigPortalActive()) {
    const uint32_t age=uint32_t(millis()-lastSuccess)/60000;
    snprintf(line,sizeof(line),"%s%lum / Open-Meteo CC BY 4.0",age>=30?"ANTIGUO ":"Hace ",(unsigned long)age);
    text(8,311,line,age>=30?ILI9341_ORANGE:MUTED);
  } else text(8,311,"Open-Meteo CC BY 4.0 / modelo",MUTED);
  dirty=false;lastDraw=millis();
}
void setup() {
  Serial.begin(115200);pinMode(0,INPUT_PULLUP);
  pinMode(5,OUTPUT);digitalWrite(5,HIGH); // SD deseleccionada.
  pinMode(21,OUTPUT);digitalWrite(21,HIGH);
  lcdBus.begin(14,12,13,15);lcd.begin(20000000);lcd.setRotation(0);lcd.setTextWrap(false);
  touchBus.begin(25,39,32,33);touch.begin(touchBus);touch.setRotation(0);
  const bool storageReady=prefs.begin("msr-meteo",false);
  if(storageReady && prefs.getBytesLength("location")==sizeof(settings)) {
    prefs.getBytes("location",&settings,sizeof(settings));settings.city[80]=0;settings.country[2]=0;
  }
  float coefficients[6];
  if(storageReady && prefs.getBytesLength("touch")==sizeof(coefficients)) {
    prefs.getBytes("touch",coefficients,sizeof(coefficients));calibrated=true;
    for(float v:coefficients)if(!isfinite(v))calibrated=false;
    if(calibrated){ax=coefficients[0];bx=coefficients[1];cx=coefficients[2];ay=coefficients[3];by=coefficients[4];cy=coefficients[5];}
  }
  if(!calibrated)calibrated=calibrate();
  if(!storageReady)status("NVS no disponible: no se guardara");
  snprintf(apName,sizeof(apName),"MSR-Meteo-%04X",unsigned(ESP.getEfuseMac()&0xFFFF));
  snprintf(apPassword,sizeof(apPassword),"%08lX%04lX",(unsigned long)esp_random(),(unsigned long)(esp_random()&0xFFFF));
  cityParam.setValue(settings.city,80);countryParam.setValue(settings.country,2);
  wm.setDebugOutput(false);wm.setTitle("MS Robotika - Estacion meteo");
  wm.setConfigPortalBlocking(false);wm.setConnectTimeout(10);wm.setSaveConnectTimeout(10);
  wm.setWiFiAutoReconnect(true);wm.setShowInfoUpdate(false);
  const char *menu[]={"wifi","exit"};wm.setMenu(menu,2);
  wm.addParameter(&cityParam);wm.addParameter(&countryParam);wm.setSaveParamsCallback(saveParameters);
  wm.setAPCallback([](WiFiManager *){dirty=true;});
  WiFi.setHostname("msr-meteo");WiFi.mode(WIFI_STA);render();
  wm.autoConnect(apName,apPassword);
  if(!settings.city[0] && !wm.getConfigPortalActive())wm.startConfigPortal(apName,apPassword);
  WiFi.setAutoReconnect(true);configTime(0,0,"pool.ntp.org","time.cloudflare.com");
  requestQueue=xQueueCreate(1,sizeof(meteo::Request));resultQueue=xQueueCreate(1,sizeof(meteo::Result));
  workerReady=requestQueue && resultQueue && xTaskCreate(networkWorker,"meteo-net",14336,nullptr,1,nullptr)==pdPASS;
  if(!workerReady)status("Error creando tarea de red");
  offlineSince=millis();dirty=true;
  Serial.println("MSR_METEO_READY profile=declared CYD display=ILI9341 touch=XPT2046");
}
void loop() {
  wm.process();const uint32_t now=millis();
  const bool connected=WiFi.status()==WL_CONNECTED,portal=wm.getConfigPortalActive();
  if(connected!=lastConnected) {
    if(connected){status("Wi-Fi conectado");configTime(0,0,"pool.ntp.org","time.cloudflare.com");}
    else {status("Sin Wi-Fi; reconectando");offlineSince=now;}
    lastConnected=connected;
  }
  if(portal!=lastPortal){dirty=true;lastPortal=portal;}
  // Si falla la red guardada, abrir una via de recuperacion sin borrar credenciales.
  if(!connected && !portal && meteo::elapsed(now,offlineSince,60000)) {wm.startConfigPortal(apName,apPassword);offlineSince=now;}
  meteo::Result result;
  if(workerReady && xQueueReceive(resultQueue,&result,0)==pdTRUE) {
    busy=false;
    if(result.generation==settings.generation) {
      lastFetchOk=result.ok;
      if(result.ok){weather=result.weather;hasWeather=true;lastSuccess=now;status("Datos recibidos / hora local");}
      else status(result.error);
    }
  }
  const uint32_t interval=locationChanged?meteo::MANUAL_MS:(lastFetchOk?meteo::REFRESH_MS:meteo::RETRY_MS);
  if(connected && !busy && settings.city[0] && (!attempted || meteo::elapsed(now,lastAttempt,interval)))requestUpdate();
  int rawx=0,rawy=0;bool touched=sampleRaw(rawx,rawy),boot=digitalRead(0)==LOW,down=touched||boot;
  static bool wasBoot=false;static int touchX=0,touchY=0;
  if(down && !pressed){pressStart=now;longHandled=false;wasBoot=boot;touchX=rawx;touchY=rawy;}
  if(down && !longHandled && meteo::elapsed(now,pressStart,2000)){togglePortal();longHandled=true;}
  if(!down && pressed && !longHandled && meteo::elapsed(now,pressStart,60)) {
    if(!wasBoot && calibrated) {
      const int x=lroundf(ax*touchX+bx*touchY+cx),y=lroundf(ay*touchX+by*touchY+cy);
      if(y>=275 && y<309 && x>=150 && x<240)togglePortal();else requestUpdate(true);
    } else requestUpdate(true);
  }
  pressed=down;
  if(dirty || meteo::elapsed(now,lastDraw,60000))render();
  delay(10);
}
