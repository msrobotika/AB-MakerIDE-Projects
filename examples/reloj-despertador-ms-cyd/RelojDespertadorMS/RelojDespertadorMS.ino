/*
  Reloj Despertador MS · AB MakerIDE / MS Robotika
  Reloj por Wi-Fi para la CYD ESP32-2432S028R: se pone en hora solo (NTP), cambia
  solo al horario de verano y tiene una alarma que se ajusta tocando la pantalla.

  Pantalla: hora grande en 7 segmentos, barra de segundos, fecha en español y la
  tarjeta de la alarma con tres botones: H+ (hora), M+ (5 minutos) y ON/OFF.
  Al sonar: la pantalla parpadea, el LED se pone rojo y suena el altavoz (si tienes
  uno en el conector SPEAK). Toca POSPONER (5 min) o PARAR.
  Mantén pulsado 3 s para cambiar la Wi-Fi.

  Monitor serie (115200): "alarma 07:30", "alarma on", "alarma off", "hora".

  Wi-Fi: escribe la tuya abajo en WIFI_SSID / WIFI_PASS, o déjalas vacías: la placa
  reutiliza la de Kilovatio MS o Radar Aviones MS, y si no hay ninguna crea la red
  "Reloj-MS" con un QR para configurarla desde el móvil. Se guarda solo en la placa.

  Librerías: Adafruit GFX, Adafruit ILI9341, XPT2046_Touchscreen (Paul Stoffregen).
  Placa: "ESP32 Dev Module" (esp32 3.x). Licencia MIT.
*/
#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <qrcode.h>

// ---------- Tus ajustes ----------
const char* WIFI_SSID = "";  // tu Wi-Fi de 2,4 GHz, o vacío para usar la guardada
const char* WIFI_PASS = "";
const char* ZONA_HORARIA = "CET-1CEST,M3.5.0,M10.5.0/3";  // España peninsular; Canarias: "WET0WEST,M3.5.0/1,M10.5.0"
constexpr int ALARMA_POR_DEFECTO = 7 * 60 + 30;           // 07:30
constexpr int POSPONER_MIN = 5;
constexpr uint32_t ALARMA_MAX_MS = 5UL * 60 * 1000;       // deja de sonar sola a los 5 min

// ---------- Hardware de la CYD ----------
SPIClass lcdBus(HSPI), touchBus(VSPI);
Adafruit_ILI9341 lcd(&lcdBus, 2, 15, -1);
XPT2046_Touchscreen touch(33, 36);
constexpr int LED_R = 4, LED_G = 16, LED_B = 17;  // activo a nivel bajo
constexpr int PIN_BL = 21, PIN_SPK = 26;
constexpr int W = 320, H = 240;

// ---------- Colores ----------
constexpr uint16_t BG = 0x0000, PANEL = 0x0861, TXT = 0xFFFF, DIM = 0x8C71;
constexpr uint16_t SEG_ON = 0x07E0, SEG_OFF = 0x0120, ACC = 0x07FF, AMB = 0xFD20, RED = 0xF8A6;

Preferences prefs;
WebServer web(80);
DNSServer dns;
bool portalMode = false;
String scannedOptions;
float ax = 0, bx = 0, cx = 0, ay = 0, by = 0, cy = 0;

int alarmMin = ALARMA_POR_DEFECTO;  // minutos desde las 00:00
bool alarmOn = false;
bool ringing = false;
int snoozeMin = -1;                  // minuto del día al que vuelve a sonar tras posponer
int lastCheckedMin = -1;
uint32_t ringStart = 0;

// ---------- Texto con acentos (fuente clásica CP437) ----------
uint8_t mapUtf8(const uint8_t*& p) {
  uint8_t c = *p++;
  if (c < 0x80) return c;
  if (c == 0xC3) {
    switch (*p++) {
      case 0xA1: return 0xA0; case 0xA9: return 0x82; case 0xAD: return 0xA1;
      case 0xB3: return 0xA2; case 0xBA: return 0xA3; case 0xB1: return 0xA4;
      case 0x91: return 0xA5; case 0xBC: return 0x81; case 0x89: return 0x90;
      case 0x81: return 'A'; case 0x8D: return 'I'; case 0x93: return 'O'; case 0x9A: return 'U';
      default: return '?';
    }
  }
  if (c == 0xC2) { uint8_t d = *p++; return d == 0xBF ? 0xA8 : d == 0xA1 ? 0xAD : d == 0xBA ? 0xF8 : d == 0xB7 ? 0xFA : '?'; }
  while ((*p & 0xC0) == 0x80) p++;
  return '?';
}
int textWidth(const char* s, uint8_t size) { int n = 0; for (const uint8_t* p = (const uint8_t*)s; *p;) { mapUtf8(p); n++; } return n * 6 * size; }
void put(int x, int y, const char* s, uint16_t fg, uint8_t size = 1, int32_t bg = -1) {
  lcd.setFont();
  for (const uint8_t* p = (const uint8_t*)s; *p; x += 6 * size) lcd.drawChar(x, y, mapUtf8(p), fg, bg < 0 ? fg : (uint16_t)bg, size);
}
void putCenter(int cxm, int y, const char* s, uint16_t fg, uint8_t size = 1, int32_t bg = -1) { put(cxm - textWidth(s, size) / 2, y, s, fg, size, bg); }

void setLed(bool r, bool g, bool b) { digitalWrite(LED_R, !r); digitalWrite(LED_G, !g); digitalWrite(LED_B, !b); }

// ---------- Lógica de la alarma (sin pantalla, para poder probarla) ----------
// "07:30" -> 450; cualquier otra cosa -> -1.
int parseHHMM(String s) {
  s.trim();
  int c = s.indexOf(':');
  if (c < 1 || c > 2 || s.length() != (unsigned)c + 3) return -1;
  for (unsigned i = 0; i < s.length(); i++) if (i != (unsigned)c && !isDigit(s[i])) return -1;
  int h = s.substring(0, c).toInt(), m = s.substring(c + 1).toInt();
  return (h < 24 && m < 60) ? h * 60 + m : -1;
}
int addMinutes(int dayMin, int add) { return ((dayMin + add) % 1440 + 1440) % 1440; }
// Suena una vez al entrar en su minuto (o en el de posponer).
bool shouldRing(int nowMin, int lastMin, bool on, int alarm, int snooze) {
  if (nowMin == lastMin) return false;
  return (on && nowMin == alarm) || nowMin == snooze;
}

bool selfTest() {
  return parseHHMM("07:30") == 450 && parseHHMM("7:05") == 425 && parseHHMM("23:59") == 1439 &&
         parseHHMM("24:00") == -1 && parseHHMM("12:60") == -1 && parseHHMM("ab:cd") == -1 && parseHHMM("1230") == -1 &&
         addMinutes(1438, 5) == 3 && addMinutes(0, -1) == 1439 &&
         shouldRing(450, 449, true, 450, -1) && !shouldRing(450, 450, true, 450, -1) &&
         !shouldRing(450, 449, false, 450, -1) && shouldRing(455, 454, false, 450, 455);
}

// ---------- Dígitos de 7 segmentos ----------
constexpr int DW = 50, DH = 96, ST = 10, DY = 34;
constexpr int DX[4] = {35, 95, 175, 235};  // dos dígitos, los dos puntos y otros dos
const uint8_t SEGS[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};  // bit0 = a ... bit6 = g
int shown[4] = {-2, -2, -2, -2};

void drawDigit(int pos, int d) {  // d = -1 dibuja "-"
  if (shown[pos] == d) return;  // solo se repinta lo que cambia: sin parpadeos
  shown[pos] = d;
  int x = DX[pos], y = DY, hh = DH / 2, v = hh - ST - ST / 2;
  uint8_t m = d < 0 ? 0x40 : SEGS[d];
  const int r[7][4] = {{x + ST, y, DW - 2 * ST, ST},           {x + DW - ST, y + ST, ST, v},
                       {x + DW - ST, y + hh + ST / 2, ST, v},   {x + ST, y + DH - ST, DW - 2 * ST, ST},
                       {x, y + hh + ST / 2, ST, v},             {x, y + ST, ST, v},
                       {x + ST, y + hh - ST / 2, DW - 2 * ST, ST}};
  for (int i = 0; i < 7; i++) lcd.fillRoundRect(r[i][0], r[i][1], r[i][2], r[i][3], 3, (m >> i) & 1 ? SEG_ON : SEG_OFF);
}
void drawColon(bool on) {
  uint16_t c = on ? SEG_ON : SEG_OFF;
  lcd.fillRoundRect(155, DY + 24, ST, ST, 2, c);
  lcd.fillRoundRect(155, DY + DH - 34, ST, ST, 2, c);
}

// ---------- Pantalla principal ----------
const char* DIAS[7] = {"domingo", "lunes", "martes", "miércoles", "jueves", "viernes", "sábado"};
const char* MESES[12] = {"enero", "febrero", "marzo", "abril", "mayo", "junio", "julio",
                         "agosto", "septiembre", "octubre", "noviembre", "diciembre"};

bool timeValid() { return time(nullptr) > 1700000000; }

void drawHeader() {
  lcd.fillRect(0, 0, W, 22, PANEL);
  put(8, 7, "RELOJ DESPERTADOR MS", SEG_ON, 1, PANEL);
  bool ok = WiFi.status() == WL_CONNECTED;
  put(ok ? 262 : 244, 7, ok ? "Wi-Fi OK" : "Sin Wi-Fi", ok ? ACC : RED, 1, PANEL);
}

void drawAlarmCard() {
  lcd.fillRoundRect(4, 190, 312, 46, 8, PANEL);
  char t[6];
  snprintf(t, sizeof(t), "%02d:%02d", alarmMin / 60, alarmMin % 60);
  put(14, 196, "ALARMA", alarmOn ? AMB : DIM, 1, PANEL);
  put(14, 210, t, alarmOn ? TXT : DIM, 3, PANEL);
  const char* lbl[3] = {"H+", "M+", alarmOn ? "ON" : "OFF"};
  for (int i = 0; i < 3; i++) {
    int x = 130 + i * 62;
    uint16_t bg = (i == 2 && alarmOn) ? AMB : 0x2124;
    lcd.fillRoundRect(x, 196, 56, 34, 6, bg);
    putCenter(x + 28, 206, lbl[i], i == 2 && alarmOn ? BG : TXT, 2, bg);
  }
}

void drawMainScreen() {
  lcd.fillScreen(BG);
  for (int& s : shown) s = -2;
  drawHeader();
  drawAlarmCard();
}

int lastSec = -1, lastDay = -1;
void drawClock() {
  if (!timeValid()) {
    for (int i = 0; i < 4; i++) drawDigit(i, -1);
    drawColon(true);
    putCenter(160, 152, "Sincronizando la hora...", DIM, 1, BG);
    return;
  }
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  if (t.tm_sec == lastSec) return;
  lastSec = t.tm_sec;
  drawDigit(0, t.tm_hour / 10);
  drawDigit(1, t.tm_hour % 10);
  drawDigit(2, t.tm_min / 10);
  drawDigit(3, t.tm_min % 10);
  drawColon(t.tm_sec % 2 == 0);
  // barra de segundos
  int w = 250 * (t.tm_sec + 1) / 60;
  lcd.fillRect(35, 140, w, 4, ACC);
  lcd.fillRect(35 + w, 140, 250 - w, 4, 0x1082);
  if (t.tm_mday != lastDay) {
    lastDay = t.tm_mday;
    char d[40];
    snprintf(d, sizeof(d), "%s, %d de %s", DIAS[t.tm_wday], t.tm_mday, MESES[t.tm_mon]);
    d[0] = toupper(d[0]);
    lcd.fillRect(0, 152, W, 30, BG);
    putCenter(160, 156, d, TXT, 2, BG);
  }
}

// ---------- Alarma sonando ----------
void drawRinging(bool phase) {
  uint16_t bg = phase ? RED : BG, fg = phase ? BG : RED;
  lcd.fillRect(0, 0, W, 186, bg);
  char t[6];
  snprintf(t, sizeof(t), "%02d:%02d", alarmMin / 60, alarmMin % 60);
  putCenter(160, 40, "¡ARRIBA!", fg, 5, bg);
  if (timeValid()) {
    time_t now = time(nullptr);
    struct tm tm;
    localtime_r(&now, &tm);
    snprintf(t, sizeof(t), "%02d:%02d", tm.tm_hour, tm.tm_min);
  }
  putCenter(160, 110, t, fg, 6, bg);
}
void drawRingButtons() {
  lcd.fillRect(0, 186, W, 54, BG);
  lcd.fillRoundRect(6, 190, 150, 46, 8, AMB);
  putCenter(81, 200, "POSPONER", BG, 2, AMB);
  putCenter(81, 220, "5 min", BG, 1, AMB);
  lcd.fillRoundRect(164, 190, 150, 46, 8, SEG_ON);
  putCenter(239, 206, "PARAR", BG, 2, SEG_ON);
}

void startRinging() {
  ringing = true;
  ringStart = millis();
  snoozeMin = -1;
  drawRingButtons();
  Serial.println("MSR_RELOJ_ALARM ring");
}
void stopRinging(bool snooze) {
  ringing = false;
  ledcWriteTone(PIN_SPK, 0);
  setLed(false, false, false);
  if (snooze && timeValid()) {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    snoozeMin = addMinutes(t.tm_hour * 60 + t.tm_min, POSPONER_MIN);
  }
  Serial.printf("MSR_RELOJ_ALARM %s\n", snooze ? "snooze" : "stop");
  drawMainScreen();
}

void updateRinging() {
  uint32_t e = millis() - ringStart;
  if (e > ALARMA_MAX_MS) { stopRinging(false); return; }
  static int lastPhase = -1;
  int ph = (e / 500) % 2;
  if (ph != lastPhase) { lastPhase = ph; drawRinging(ph); setLed(ph, false, false); }
  uint32_t b = e % 1000;  // bip-bip y silencio
  ledcWriteTone(PIN_SPK, (b < 120 || (b > 200 && b < 320)) ? 2000 : 0);
}

void saveAlarm() {
  prefs.putInt("alarm", alarmMin);
  prefs.putBool("on", alarmOn);
  Serial.printf("MSR_RELOJ_ALARM_SET %02d:%02d %s\n", alarmMin / 60, alarmMin % 60, alarmOn ? "on" : "off");
}

// ---------- Táctil (igual que Kilovatio MS) ----------
bool sampleRaw(int& x, int& y) {
  if (!touch.tirqTouched() || !touch.touched()) return false;
  TS_Point p = touch.getPoint();
  if (p.z < 250) return false;
  x = p.x; y = p.y;
  return true;
}
void toScreen(int rx, int ry, int& x, int& y) { x = lroundf(ax * rx + bx * ry + cx); y = lroundf(ay * rx + by * ry + cy); }

void calibrate() {
  const int sx[3] = {24, 296, 24}, sy[3] = {24, 24, 216};
  int rx[3], ry[3];
  while (true) {
    for (int i = 0; i < 3; i++) {
      lcd.fillScreen(BG);
      putCenter(160, 100, "CALIBRAR TÁCTIL", ACC, 2);
      putCenter(160, 128, "Toca el centro de la cruz", TXT);
      lcd.drawFastHLine(sx[i] - 10, sy[i], 21, AMB);
      lcd.drawFastVLine(sx[i], sy[i] - 10, 21, AMB);
      lcd.drawCircle(sx[i], sy[i], 8, AMB);
      int x, y;
      while (sampleRaw(x, y)) delay(10);
      int32_t sxr = 0, syr = 0;
      int n = 0;
      while (n < 12) {
        if (sampleRaw(x, y)) { sxr += x; syr += y; n++; }
        else { n = 0; sxr = syr = 0; }
        delay(12);
      }
      rx[i] = sxr / n; ry[i] = syr / n;
      while (sampleRaw(x, y)) delay(10);
      delay(150);
    }
    float ux = rx[1] - rx[0], uy = ry[1] - ry[0], vx = rx[2] - rx[0], vy = ry[2] - ry[0];
    float det = ux * vy - vx * uy;
    if (fabsf(det) < 10000) { putCenter(160, 160, "Repite las tres cruces", RED); delay(1000); continue; }
    ax = ((sx[1] - sx[0]) * vy - (sx[2] - sx[0]) * uy) / det;
    bx = (ux * (sx[2] - sx[0]) - vx * (sx[1] - sx[0])) / det;
    cx = sx[0] - ax * rx[0] - bx * ry[0];
    ay = ((sy[1] - sy[0]) * vy - (sy[2] - sy[0]) * uy) / det;
    by = (ux * (sy[2] - sy[0]) - vx * (sy[1] - sy[0])) / det;
    cy = sy[0] - ay * rx[0] - by * ry[0];
    prefs.putFloat("ax", ax); prefs.putFloat("bx", bx); prefs.putFloat("cx", cx);
    prefs.putFloat("ay", ay); prefs.putFloat("by", by); prefs.putFloat("cy", cy);
    prefs.putBool("cal", true);
    return;
  }
}

// Calibración y Wi-Fi: las propias o, si no hay, las de los otros proyectos MS (misma rotación).
const char* NAMESPACES[3] = {"relojms", "radaravion", "kilovatio"};
bool loadCalibration() {
  for (const char* ns : NAMESPACES) {
    Preferences p;
    if (!p.begin(ns, true)) continue;
    bool ok = p.getBool("cal", false);
    if (ok) { ax = p.getFloat("ax"); bx = p.getFloat("bx"); cx = p.getFloat("cx"); ay = p.getFloat("ay"); by = p.getFloat("by"); cy = p.getFloat("cy"); }
    p.end();
    if (ok) return true;
  }
  return false;
}

// ---------- Portal Wi-Fi ----------
String htmlEscape(const String& s) {
  String o;
  for (char c : s) {
    if (c == '&') o += "&amp;"; else if (c == '<') o += "&lt;"; else if (c == '>') o += "&gt;";
    else if (c == '"') o += "&quot;"; else if (c == '\'') o += "&#39;"; else o += c;
  }
  return o;
}

const char PAGE_HEAD[] PROGMEM =
    "<!doctype html><html lang=es><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'>"
    "<title>Reloj Despertador MS</title><style>body{font-family:system-ui,sans-serif;background:#04110a;color:#e8f7ee;"
    "max-width:420px;margin:auto;padding:20px}h1{font-size:22px}input,button{width:100%;box-sizing:border-box;"
    "padding:12px;margin:6px 0 14px;border-radius:10px;border:1px solid #2a4;background:#0b2215;color:#fff;font-size:16px}"
    "button{background:#22dd66;color:#012;font-weight:700;border:0}small{color:#9cb}</style>";

void handleRoot() {
  String page = FPSTR(PAGE_HEAD);
  page += "<h1>⏰ Reloj Despertador MS</h1><form method=post action=/save>"
          "<label>Wi-Fi de casa (2,4 GHz)</label><input name=s list=nets maxlength=32 required><datalist id=nets>";
  page += scannedOptions;
  page += "</datalist><label>Contraseña</label><input name=p type=password maxlength=63>"
          "<small>Se guarda solo en la placa.</small><button>Guardar</button></form>"
          "<small>MS Robotika · AB MakerIDE</small>";
  web.send(200, "text/html; charset=utf-8", page);
}

void handleSave() {
  String s = web.arg("s"), p = web.arg("p");
  s.trim();
  if (!s.length() || s.length() > 32 || (p.length() && (p.length() < 8 || p.length() > 63))) {
    web.send(400, "text/html; charset=utf-8", String(FPSTR(PAGE_HEAD)) +
             "<h1>Revisa los datos</h1><p>Escribe tu Wi-Fi; la clave debe tener de 8 a 63 caracteres.</p><a href=/>Volver</a>");
    return;
  }
  prefs.putString("ssid", s);
  prefs.putString("pass", p);
  web.send(200, "text/html; charset=utf-8", String(FPSTR(PAGE_HEAD)) + "<h1>Guardado</h1><p>La placa se reinicia.</p>");
  Serial.println("MSR_RELOJ_SETTINGS_SAVED");
  delay(1200);
  ESP.restart();
}

static int qrX = 0, qrY = 0;
void drawQr(esp_qrcode_handle_t q) {
  int size = esp_qrcode_get_size(q), sc = max(1, 150 / (size + 4));
  lcd.fillRect(qrX, qrY, (size + 4) * sc, (size + 4) * sc, 0xFFFF);
  for (int y = 0; y < size; y++)
    for (int x = 0; x < size; x++)
      if (esp_qrcode_get_module(q, x, y)) lcd.fillRect(qrX + (x + 2) * sc, qrY + (y + 2) * sc, sc, sc, 0x0000);
}

void startPortal() {
  portalMode = true;
  if (ringing) stopRinging(false);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP_STA);
  int n = WiFi.scanNetworks();
  scannedOptions = "";
  for (int i = 0; i < n && i < 15; i++) scannedOptions += "<option value=\"" + htmlEscape(WiFi.SSID(i)) + "\">";
  WiFi.scanDelete();
  char pass[9];
  snprintf(pass, sizeof(pass), "%08lu", (unsigned long)(esp_random() % 100000000UL));
  WiFi.softAP("Reloj-MS", pass);
  dns.start(53, "*", WiFi.softAPIP());
  web.on("/", HTTP_GET, handleRoot);
  web.on("/save", HTTP_POST, handleSave);
  web.onNotFound([] { web.sendHeader("Location", "http://192.168.4.1/"); web.send(302, "text/plain", ""); });
  web.begin();

  lcd.fillScreen(BG);
  lcd.fillRect(0, 0, W, 26, PANEL);
  put(10, 9, "RELOJ DESPERTADOR · WI-FI", SEG_ON, 1, PANEL);
  String qr = "WIFI:T:WPA;S:Reloj-MS;P:" + String(pass) + ";;";
  qrX = 8; qrY = 40;
  esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
  cfg.display_func = drawQr;
  cfg.max_qrcode_version = 6;
  esp_qrcode_generate(&cfg, qr.c_str());
  put(170, 44, "1. Escanea el QR", TXT);
  put(170, 58, "   con el móvil", TXT);
  put(170, 80, "2. Se abre la web;", TXT);
  put(170, 94, "   si no: 192.168.4.1", DIM);
  put(170, 116, "3. Elige tu Wi-Fi", TXT);
  put(170, 130, "   y su clave", TXT);
  put(170, 156, "Red: Reloj-MS", ACC);
  put(170, 170, "Clave:", ACC);
  put(212, 166, pass, AMB, 2);
  setLed(false, false, true);
  Serial.println("MSR_RELOJ_PORTAL ap=Reloj-MS");
}

bool connectWifi() {
  String s = WIFI_SSID, p = WIFI_PASS;
  for (const char* ns : NAMESPACES) {
    if (s.length()) break;
    Preferences k;
    if (k.begin(ns, true)) { s = k.getString("ssid", ""); p = k.getString("pass", ""); k.end(); }
  }
  if (!s.length()) return false;
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("reloj-ms");
  WiFi.begin(s.c_str(), p.c_str());
  lcd.fillScreen(BG);
  putCenter(160, 96, "RELOJ MS", SEG_ON, 3);
  putCenter(160, 132, "Conectando a la Wi-Fi...", TXT);
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) {
    lcd.fillCircle(130 + (i % 5) * 15, 160, 4, SEG_ON);
    if (i % 5 == 4) lcd.fillRect(120, 154, 80, 12, BG);
    delay(500);
  }
  return WiFi.status() == WL_CONNECTED;
}

// ---------- Gestos ----------
bool touching = false, longFired = false;
int downX = 0, downY = 0;
uint32_t downAt = 0, upAt = 0;

void onTap(int x, int y) {
  if (ringing) {
    if (y > 180) stopRinging(x < 160);
    return;
  }
  if (y < 190 || x < 128) return;
  int b = (x - 128) / 62;
  if (b == 0) alarmMin = addMinutes(alarmMin, 60);
  else if (b == 1) alarmMin = addMinutes(alarmMin - alarmMin % 5, 5);
  else alarmOn = !alarmOn;
  snoozeMin = -1;
  saveAlarm();
  drawAlarmCard();
}

void handleTouch() {
  int rx, ry;
  uint32_t t = millis();
  if (sampleRaw(rx, ry)) {
    int x, y;
    toScreen(rx, ry, x, y);
    if (!touching) { touching = true; longFired = false; downX = x; downY = y; downAt = t; }
    upAt = t;
    if (!longFired && !ringing && t - downAt > 3000) { longFired = true; startPortal(); }
    return;
  }
  if (touching && t - upAt > 60) {
    touching = false;
    if (!longFired && !portalMode) onTap(downX, downY);
  }
}

// ---------- Monitor serie ----------
void handleSerial() {
  static String line;
  while (Serial.available()) {
    char c = Serial.read();
    if (c != '\n' && c != '\r') { if (line.length() < 40) line += c; continue; }
    line.trim();
    line.toLowerCase();
    if (line.startsWith("alarma ")) {
      String a = line.substring(7);
      a.trim();
      int m = parseHHMM(a);
      if (a == "on" || a == "off") alarmOn = a == "on";
      else if (m >= 0) { alarmMin = m; alarmOn = true; }
      if (m < 0 && a != "on" && a != "off") Serial.println("MSR_RELOJ_ERROR usa: alarma 07:30 | alarma on | alarma off");
      else {
        snoozeMin = -1;
        saveAlarm();
        if (ringing && !alarmOn) stopRinging(false);
        else if (!ringing) drawAlarmCard();
      }
    } else if (line == "hora") {
      time_t now = time(nullptr);
      struct tm t;
      localtime_r(&now, &t);
      Serial.printf("MSR_RELOJ_TIME %02d:%02d:%02d valid=%d\n", t.tm_hour, t.tm_min, t.tm_sec, timeValid());
    } else if (line.length()) {
      Serial.println("Comandos: alarma 07:30 | alarma on | alarma off | hora");
    }
    line = "";
  }
}

// ---------- Programa ----------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("MSR_RELOJ_BOOT selftest=%s\n", selfTest() ? "OK" : "FAIL");
  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_B, OUTPUT);
  setLed(false, false, false);
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);  // microSD sin usar
  pinMode(PIN_BL, OUTPUT);
  digitalWrite(PIN_BL, HIGH);
  ledcAttach(PIN_SPK, 2000, 8);
  ledcWriteTone(PIN_SPK, 0);

  lcdBus.begin(14, 12, 13, 15);
  lcd.begin(40000000);
  lcd.setRotation(1);
  lcd.cp437(true);
  touchBus.begin(25, 39, 32, 33);
  touch.begin(touchBus);
  touch.setRotation(1);

  prefs.begin("relojms", false);
  alarmMin = prefs.getInt("alarm", ALARMA_POR_DEFECTO);
  alarmOn = prefs.getBool("on", false);
  if (!loadCalibration()) calibrate();

  if (!connectWifi()) { startPortal(); return; }
  Serial.printf("MSR_RELOJ_WIFI_OK rssi=%ld\n", WiFi.RSSI());
  configTzTime(ZONA_HORARIA, "pool.ntp.org", "time.google.com");
  drawMainScreen();
}

void loop() {
  if (portalMode) { dns.processNextRequest(); web.handleClient(); delay(2); return; }
  handleTouch();
  handleSerial();

  static bool synced = false;
  if (!synced && timeValid()) {
    synced = true;
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    Serial.printf("MSR_RELOJ_NTP_OK %02d:%02d\n", t.tm_hour, t.tm_min);
    lcd.fillRect(0, 152, W, 30, BG);
  }

  if (ringing) updateRinging();
  else drawClock();

  if (timeValid()) {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    int nowMin = t.tm_hour * 60 + t.tm_min;
    if (!ringing && shouldRing(nowMin, lastCheckedMin, alarmOn, alarmMin, snoozeMin)) startRinging();
    lastCheckedMin = nowMin;
  }

  static uint32_t lastHeader = 0;
  static int lastOk = -1;
  if (!ringing && millis() - lastHeader > 5000) {
    lastHeader = millis();
    int ok = WiFi.status() == WL_CONNECTED;
    if (ok != lastOk) { lastOk = ok; drawHeader(); }
    if (!ok) WiFi.reconnect();
  }
  delay(5);
}
