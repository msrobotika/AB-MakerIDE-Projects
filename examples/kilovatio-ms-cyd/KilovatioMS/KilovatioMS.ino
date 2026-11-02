/*
  Kilovatio MS · AB MakerIDE / MS Robotika
  Monitor del precio de la luz (PVPC, peninsula) para la CYD ESP32-2432S028R.
  Cinco pantallas que se cambian deslizando: AHORA, HOY, PLANIFICA, MANANA y AJUSTES.
  El LED RGB trasero marca si la hora actual es barata, normal o cara.

  Datos: API publica de Red Electrica (apidatos.ree.es), sin cuenta ni clave.
  La conexion HTTPS se valida con las raices de GlobalSign incluidas abajo.
  Wi-Fi: la primera vez la placa crea la red "Kilovatio-MS" y muestra un QR;
  desde el movil eliges tu red y escribes la clave. Se guarda en la placa (NVS).

  Librerias: Adafruit GFX, Adafruit ILI9341, XPT2046_Touchscreen (Paul Stoffregen),
  ArduinoJson 7. Placa: "ESP32 Dev Module" (esp32 3.x).
  Referencia de pines: github.com/witnessmenow/ESP32-Cheap-Yellow-Display/PINS.md
  Precio orientativo: es el PVPC publicado por REE (energia + peajes y cargos,
  sin impuestos ni termino de potencia). No sustituye a tu factura.
  Licencia MIT, como el resto de AB MakerIDE.
*/
#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <qrcode.h>
#include <time.h>

// ---------- Hardware de la CYD ----------
SPIClass lcdBus(HSPI), touchBus(VSPI);
Adafruit_ILI9341 lcd(&lcdBus, 2, 15, -1);
XPT2046_Touchscreen touch(33, 36);
constexpr int LED_R = 4, LED_G = 16, LED_B = 17;  // activo a nivel bajo
constexpr int PIN_BL = 21, PIN_LDR = 34, PIN_SPK = 26;
constexpr int W = 320, H = 240;

// ---------- Colores ----------
constexpr uint16_t BG = 0x0861, PANEL = 0x18E4, LINE = 0x31A6, TXT = 0xFFFF, DIM = 0x9CF3;
constexpr uint16_t CHEAP = 0x2E8A, MID = 0xFE41, DEAR = 0xF8A6, ACC = 0x05FF, NEG = 0x6E7F;

// ---------- Certificados raiz (GlobalSign Root R46 y Root R3) ----------
const char ROOT_CA[] PROGMEM = R"PEM(-----BEGIN CERTIFICATE-----
MIIFWjCCA0KgAwIBAgISEdK7udcjGJ5AXwqdLdDfJWfRMA0GCSqGSIb3DQEBDAUA
MEYxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9iYWxTaWduIG52LXNhMRwwGgYD
VQQDExNHbG9iYWxTaWduIFJvb3QgUjQ2MB4XDTE5MDMyMDAwMDAwMFoXDTQ2MDMy
MDAwMDAwMFowRjELMAkGA1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYt
c2ExHDAaBgNVBAMTE0dsb2JhbFNpZ24gUm9vdCBSNDYwggIiMA0GCSqGSIb3DQEB
AQUAA4ICDwAwggIKAoICAQCsrHQy6LNl5brtQyYdpokNRbopiLKkHWPd08EsCVeJ
OaFV6Wc0dwxu5FUdUiXSE2te4R2pt32JMl8Nnp8semNgQB+msLZ4j5lUlghYruQG
vGIFAha/r6gjA7aUD7xubMLL1aa7DOn2wQL7Id5m3RerdELv8HQvJfTqa1VbkNud
316HCkD7rRlr+/fKYIje2sGP1q7Vf9Q8g+7XFkyDRTNrJ9CG0Bwta/OrffGFqfUo
0q3v84RLHIf8E6M6cqJaESvWJ3En7YEtbWaBkoe0G1h6zD8K+kZPTXhc+CtI4wSE
y132tGqzZfxCnlEmIyDLPRT5ge1lFgBPGmSXZgjPjHvjK8Cd+RTyG/FWaha/LIWF
zXg4mutCagI0GIMXTpRW+LaCtfOW3T3zvn8gdz57GSNrLNRyc0NXfeD412lPFzYE
+cCQYDdF3uYM2HSNrpyibXRdQr4G9dlkbgIQrImwTDsHTUB+JMWKmIJ5jqSngiCN
I/onccnfxkF0oE32kRbcRoxfKWMxWXEM2G/CtjJ9++ZdU6Z+Ffy7dXxd7Pj2Fxzs
x2sZy/N78CsHpdlseVR2bJ0cpm4O6XkMqCNqo98bMDGfsVR7/mrLZqrcZdCinkqa
ByFrgY/bxFn63iLABJzjqls2k+g9vXqhnQt2sQvHnf3PmKgGwvgqo6GDoLclcqUC
4wIDAQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNV
HQ4EFgQUA1yrc4GHqMywptWU4jaWSf8FmSwwDQYJKoZIhvcNAQEMBQADggIBAHx4
7PYCLLtbfpIrXTncvtgdokIzTfnvpCo7RGkerNlFo048p9gkUbJUHJNOxO97k4Vg
JuoJSOD1u8fpaNK7ajFxzHmuEajwmf3lH7wvqMxX63bEIaZHU1VNaL8FpO7XJqti
2kM3S+LGteWygxk6x9PbTZ4IevPuzz5i+6zoYMzRx6Fcg0XERczzF2sUyQQCPtIk
pnnpHs6i58FZFZ8d4kuaPp92CC1r2LpXFNqD6v6MVenQTqnMdzGxRBF6XLE+0xRF
FRhiJBPSy03OXIPBNvIQtQ6IbbjhVp+J3pZmOUdkLG5NrmJ7v2B0GbhWrJKsFjLt
rWhV/pi60zTe9Mlhww6G9kuEYO4Ne7UyWHmRVSyBQ7N0H3qqJZ4d16GLuc1CLgSk
ZoNNiTW2bKg2SnkheCLQQrzRQDGQob4Ez8pn7fXwgNNgyYMqIgXQBztSvwyeqiv5
u+YfjyW6hY0XHgL+XVAEV8/+LbzvXMAaq7afJMbfc2hIkCwU9D9SGuTSyxTDYWnP
4vkYxboznxSjBF25cfe1lNj2M8FawTSLfJvdkzrnE6JwYZ+vj+vYxXX4M2bUdGc6
N3ec592kD3ZDZopD8p/7DEJ4Y9HiD2971KE9dJeFt0g5QdYg/NA6s/rob8SKunE3
vouXsXgxT7PntgMTzlSdriVZzH81Xwj3QEUxeCp6
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIDXzCCAkegAwIBAgILBAAAAAABIVhTCKIwDQYJKoZIhvcNAQELBQAwTDEgMB4G
A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjMxEzARBgNVBAoTCkdsb2JhbFNp
Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDkwMzE4MTAwMDAwWhcNMjkwMzE4
MTAwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMzETMBEG
A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI
hvcNAQEBBQADggEPADCCAQoCggEBAMwldpB5BngiFvXAg7aEyiie/QV2EcWtiHL8
RgJDx7KKnQRfJMsuS+FggkbhUqsMgUdwbN1k0ev1LKMPgj0MK66X17YUhhB5uzsT
gHeMCOFJ0mpiLx9e+pZo34knlTifBtc+ycsmWQ1z3rDI6SYOgxXG71uL0gRgykmm
KPZpO/bLyCiR5Z2KYVc3rHQU3HTgOu5yLy6c+9C7v/U9AOEGM+iCK65TpjoWc4zd
QQ4gOsC0p6Hpsk+QLjJg6VfLuQSSaGjlOCZgdbKfd/+RFO+uIEn8rUAVSNECMWEZ
XriX7613t2Saer9fwRPvm2L7DWzgVGkWqQPabumDk3F2xmmFghcCAwEAAaNCMEAw
DgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0OBBYEFI/wS3+o
LkUkrk1Q+mOai97i3Ru8MA0GCSqGSIb3DQEBCwUAA4IBAQBLQNvAUKr+yAzv95ZU
RUm7lgAJQayzE4aGKAczymvmdLm6AC2upArT9fHxD4q/c2dKg8dEe3jgr25sbwMp
jjM5RcOO5LlXbKr8EpbsU8Yt5CRsuZRj+9xTaGdWPoO4zzUhw8lo/s7awlOqzJCK
6fBdRoyV3XpYKBovHd7NADdBj+1EbddTKJd+82cEHhXXipa0095MJ6RMG3NzdvQX
mcIfeg7jLQitChws/zyrVQ4PkX4268NXSb7hLi18YIvDQVETI53O9zJrlAGomecs
Mx86OyXShkDOOyyGeMlhLxS67ttVb9+E7gUJTb0o2HLO02JQZR7rkpeDMdmztcpH
WD9f
-----END CERTIFICATE-----
)PEM";

// ---------- Estado ----------
Preferences prefs;
WebServer web(80);
DNSServer dns;

struct Day {
  float p[24];       // EUR/kWh por hora local
  bool has[24];
  int count = 0;
  int ymd = 0;       // AAAAMMDD
  float lo = 0, hi = 0, avg = 0, t1 = 0, t2 = 0;  // minimo, maximo, media y tercios
};
Day today, tomorrow;
struct Btn { int x, y, w, h; };  // aqui arriba para los prototipos automaticos de Arduino

enum Screen { S_NOW, S_DAY, S_PLAN, S_TOMORROW, S_SETTINGS, S_COUNT };
const char* const SCREEN_NAMES[S_COUNT] = {"AHORA", "HOY", "PLANIFICA", "MAÑANA", "AJUSTES"};
int screen = S_NOW;
bool portalMode = false;
String apPass;
String scannedOptions;

float ax = 0, bx = 0, cx = 0, ay = 0, by = 0, cy = 0;
bool autoBright = true, ledOn = true, beepOn = false;
int planHours = 2;
int selectedBar = -1;
uint32_t lastFetchTry = 0, lastTomorrowTry = 0, lastMinute = 0, lastBright = 0;
uint32_t fetchedAt = 0;
int lastHourShown = -1, lastYmd = 0;
char lastError[48] = "";

// ---------- Texto con acentos (fuente clasica CP437) y simbolo euro ----------
const uint8_t EURO[7] = {0x07, 0x08, 0x1E, 0x08, 0x1E, 0x08, 0x07};

void euro(int x, int y, uint8_t s, uint16_t fg, int32_t bg) {
  if (bg >= 0) lcd.fillRect(x, y, 6 * s, 8 * s, bg);
  for (int r = 0; r < 7; r++)
    for (int c = 0; c < 5; c++)
      if (EURO[r] & (0x10 >> c)) lcd.fillRect(x + c * s, y + r * s, s, s, fg);
}

uint8_t mapUtf8(const uint8_t*& p, bool& isEuro) {
  isEuro = false;
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
  if (c == 0xC2) {
    uint8_t d = *p++;
    return d == 0xBF ? 0xA8 : d == 0xA1 ? 0xAD : d == 0xBA ? 0xF8 : '?';
  }
  if (c == 0xE2 && p[0] == 0x82 && p[1] == 0xAC) { p += 2; isEuro = true; return ' '; }
  while ((*p & 0xC0) == 0x80) p++;
  return '?';
}

int textWidth(const char* s, uint8_t size) {
  int n = 0;
  bool e;
  for (const uint8_t* p = (const uint8_t*)s; *p;) { mapUtf8(p, e); n++; }
  return n * 6 * size;
}

void put(int x, int y, const char* s, uint16_t fg, uint8_t size = 1, int32_t bg = -1) {
  lcd.setFont();
  int cx0 = x;
  bool isEuro;
  for (const uint8_t* p = (const uint8_t*)s; *p;) {
    uint8_t ch = mapUtf8(p, isEuro);
    if (isEuro) euro(cx0, y, size, fg, bg);
    else lcd.drawChar(cx0, y, ch, fg, bg < 0 ? fg : (uint16_t)bg, size);
    cx0 += 6 * size;
  }
}

void putCenter(int cxm, int y, const char* s, uint16_t fg, uint8_t size = 1, int32_t bg = -1) {
  put(cxm - textWidth(s, size) / 2, y, s, fg, size, bg);
}

void putRight(int xr, int y, const char* s, uint16_t fg, uint8_t size = 1, int32_t bg = -1) {
  put(xr - textWidth(s, size), y, s, fg, size, bg);
}

void price(char* out, size_t n, float v, int decimals = 3) {
  snprintf(out, n, "%.*f", decimals, v);
  for (char* c = out; *c; c++) if (*c == '.') *c = ',';
}

// ---------- LED, brillo y sonido ----------
void setLed(bool r, bool g, bool b) {
  digitalWrite(LED_R, r ? LOW : HIGH);
  digitalWrite(LED_G, g ? LOW : HIGH);
  digitalWrite(LED_B, b ? LOW : HIGH);
}

void updateBrightness() {
  int level = 230;
  if (autoBright) {
    int v = analogRead(PIN_LDR);  // con 0 dB: luz fuerte ~0, oscuridad sube
    level = constrain(map(v, 0, 450, 255, 35), 35, 255);
  }
  ledcWrite(PIN_BL, level);
}

void beep(int freq, int ms) {
  if (!beepOn) return;
  tone(PIN_SPK, freq, ms);
}

// ---------- Tiempo ----------
bool now(struct tm& t) { return getLocalTime(&t, 50); }

int ymdOf(const struct tm& t) { return (t.tm_year + 1900) * 10000 + (t.tm_mon + 1) * 100 + t.tm_mday; }

int currentHour() {
  struct tm t;
  return now(t) ? t.tm_hour : -1;
}

// ---------- Clasificacion de precios ----------
void computeStats(Day& d) {
  float v[24];
  int n = 0;
  float sum = 0;
  for (int h = 0; h < 24; h++) if (d.has[h]) { v[n++] = d.p[h]; sum += d.p[h]; }
  d.count = n;
  if (!n) return;
  std::sort(v, v + n);
  d.lo = v[0];
  d.hi = v[n - 1];
  d.avg = sum / n;
  d.t1 = v[n / 3];
  d.t2 = v[(2 * n) / 3];
}

// 0 barata, 1 normal, 2 cara
int level(const Day& d, float p) { return p < d.t1 ? 0 : (p < d.t2 ? 1 : 2); }
uint16_t levelColor(int l) { return l == 0 ? CHEAP : (l == 1 ? MID : DEAR); }
const char* levelName(int l) { return l == 0 ? "BARATA" : (l == 1 ? "NORMAL" : "CARA"); }

void applyLed() {
  int h = currentHour();
  if (!ledOn || h < 0 || !today.has[h]) { setLed(false, false, false); return; }
  int l = level(today, today.p[h]);
  if (l == 0) setLed(false, true, false);
  else if (l == 1) setLed(true, true, false);
  else setLed(true, false, false);
}

// ---------- Descarga de precios (REE) ----------
bool fetchDay(int offset, Day& out) {
  struct tm t;
  if (!now(t)) { strlcpy(lastError, "Sin hora todavia", sizeof(lastError)); return false; }
  t.tm_mday += offset;
  t.tm_hour = 12; t.tm_min = 0; t.tm_sec = 0; t.tm_isdst = -1;
  mktime(&t);
  char url[220];
  snprintf(url, sizeof(url),
           "https://apidatos.ree.es/es/datos/mercados/precios-mercados-tiempo-real"
           "?start_date=%04d-%02d-%02dT00:00&end_date=%04d-%02d-%02dT23:59&time_trunc=hour",
           t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
  char wanted[11];
  snprintf(wanted, sizeof(wanted), "%04d-%02d-%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

  WiFiClientSecure client;
  client.setCACert(ROOT_CA);
  client.setTimeout(15);
  HTTPClient http;
  http.setTimeout(15000);
  if (!http.begin(client, url)) { strlcpy(lastError, "No se pudo abrir HTTPS", sizeof(lastError)); return false; }
  http.setUserAgent("Kilovatio-MS/1.0 (CYD)");
  http.addHeader("Accept", "application/json");
  int code = http.GET();
  if (code != 200) {
    snprintf(lastError, sizeof(lastError), "REE respondio %d", code);
    http.end();
    return false;
  }
  String body = http.getString();
  http.end();

  JsonDocument filter;
  filter["included"][0]["id"] = true;
  filter["included"][0]["attributes"]["values"][0]["value"] = true;
  filter["included"][0]["attributes"]["values"][0]["datetime"] = true;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body, DeserializationOption::Filter(filter));
  if (err) { snprintf(lastError, sizeof(lastError), "JSON: %s", err.c_str()); return false; }

  Day d;
  for (int h = 0; h < 24; h++) d.has[h] = false;
  for (JsonObject s : doc["included"].as<JsonArray>()) {
    if (strcmp(s["id"] | "", "1001") != 0) continue;  // 1001 = PVPC
    for (JsonObject v : s["attributes"]["values"].as<JsonArray>()) {
      const char* dt = v["datetime"] | "";
      if (strlen(dt) < 13 || strncmp(dt, wanted, 10) != 0) continue;
      int h = atoi(dt + 11);
      if (h < 0 || h > 23) continue;
      d.p[h] = (v["value"] | 0.0f) / 1000.0f;
      d.has[h] = true;
    }
  }
  computeStats(d);
  if (d.count < 23) {
    snprintf(lastError, sizeof(lastError), offset ? "Mañana aun no publicado" : "Faltan datos (%d h)", d.count);
    return false;
  }
  d.ymd = ymdOf(t);
  out = d;
  lastError[0] = 0;
  Serial.printf("MSR_KILOVATIO_PRICES day=%d hours=%d min=%.3f max=%.3f\n", d.ymd, d.count, d.lo, d.hi);
  return true;
}

// ---------- Tactil ----------
bool sampleRaw(int& x, int& y) {
  if (!touch.tirqTouched() || !touch.touched()) return false;
  TS_Point p = touch.getPoint();
  if (p.z < 250) return false;
  x = p.x;
  y = p.y;
  return true;
}

void toScreen(int rx, int ry, int& x, int& y) {
  x = lroundf(ax * rx + bx * ry + cx);
  y = lroundf(ay * rx + by * ry + cy);
}

void calibrate() {
  const int sx[3] = {24, 296, 24}, sy[3] = {24, 24, 216};
  int rx[3], ry[3];
  while (true) {
    for (int i = 0; i < 3; i++) {
      lcd.fillScreen(BG);
      putCenter(160, 100, "CALIBRAR TÁCTIL", ACC, 2);
      putCenter(160, 128, "Toca el centro de la cruz", TXT);
      lcd.drawFastHLine(sx[i] - 10, sy[i], 21, MID);
      lcd.drawFastVLine(sx[i], sy[i] - 10, 21, MID);
      lcd.drawCircle(sx[i], sy[i], 8, MID);
      int x, y;
      while (sampleRaw(x, y)) delay(10);
      int32_t sxr = 0, syr = 0;
      int n = 0;
      while (n < 12) {
        if (sampleRaw(x, y)) { sxr += x; syr += y; n++; }
        else { n = 0; sxr = syr = 0; }
        delay(12);
      }
      rx[i] = sxr / n;
      ry[i] = syr / n;
      while (sampleRaw(x, y)) delay(10);
      delay(150);
    }
    float ux = rx[1] - rx[0], uy = ry[1] - ry[0], vx = rx[2] - rx[0], vy = ry[2] - ry[0];
    float det = ux * vy - vx * uy;
    if (fabsf(det) < 10000) {
      putCenter(160, 160, "Repite las tres cruces", DEAR);
      delay(1000);
      continue;
    }
    ax = ((sx[1] - sx[0]) * vy - (sx[2] - sx[0]) * uy) / det;
    bx = (ux * (sx[2] - sx[0]) - vx * (sx[1] - sx[0])) / det;
    cx = sx[0] - ax * rx[0] - bx * ry[0];
    ay = ((sy[1] - sy[0]) * vy - (sy[2] - sy[0]) * uy) / det;
    by = (ux * (sy[2] - sy[0]) - vx * (sy[1] - sy[0])) / det;
    cy = sy[0] - ay * rx[0] - by * ry[0];
    prefs.putFloat("ax", ax); prefs.putFloat("bx", bx); prefs.putFloat("cx", cx);
    prefs.putFloat("ay", ay); prefs.putFloat("by", by); prefs.putFloat("cy", cy);
    prefs.putBool("cal", true);
    Serial.println("MSR_KILOVATIO_CALIBRATION_SAVED");
    return;
  }
}

// ---------- Marco comun ----------
void wifiIcon(int x, int y) {
  int bars = 0;
  if (WiFi.status() == WL_CONNECTED) {
    long r = WiFi.RSSI();
    bars = r > -60 ? 4 : r > -70 ? 3 : r > -80 ? 2 : 1;
  }
  for (int i = 0; i < 4; i++) {
    int h = 3 + i * 3;
    lcd.fillRect(x + i * 5, y + 12 - h, 3, h, i < bars ? TXT : LINE);
  }
}

void drawHeader() {
  lcd.fillRect(0, 0, W, 26, PANEL);
  lcd.fillRoundRect(8, 6, 14, 14, 3, MID);
  lcd.fillTriangle(16, 8, 11, 14, 15, 14, BG);  // rayo
  lcd.fillTriangle(15, 12, 19, 12, 13, 18, BG);
  put(28, 9, "KILOVATIO MS", TXT, 1, PANEL);
  put(108, 9, SCREEN_NAMES[screen], ACC, 1, PANEL);
  struct tm t;
  char clock[8] = "--:--";
  if (now(t)) strftime(clock, sizeof(clock), "%H:%M", &t);
  putRight(286, 6, clock, TXT, 2, PANEL);
  wifiIcon(294, 7);
}

void drawFooter() {
  lcd.fillRect(0, 226, W, 14, BG);
  int x0 = 160 - (S_COUNT * 14) / 2;
  for (int i = 0; i < S_COUNT; i++) {
    if (i == screen) lcd.fillRoundRect(x0 + i * 14, 230, 10, 5, 2, ACC);
    else lcd.fillCircle(x0 + i * 14 + 5, 232, 2, LINE);
  }
}

void clearBody() { lcd.fillRect(0, 26, W, 200, BG); }

void noData(const char* extra) {
  putCenter(160, 100, "Sin precios todavía", TXT, 2);
  putCenter(160, 130, extra && *extra ? extra : "Descargando de REE...", DIM);
}

// ---------- Pantalla 1: AHORA ----------
void drawNow() {
  clearBody();
  int h = currentHour();
  if (h < 0 || !today.has[h]) { noData(lastError); return; }
  float p = today.p[h];
  int l = level(today, p);
  uint16_t c = levelColor(l);

  lcd.fillRoundRect(10, 34, 300, 112, 10, PANEL);
  lcd.fillRoundRect(10, 34, 8, 112, 4, c);
  char line[40];
  snprintf(line, sizeof(line), "Precio de %02d:00 a %02d:00", h, (h + 1) % 24);
  put(28, 42, line, DIM, 1, PANEL);

  char v[16];
  price(v, sizeof(v), p);
  lcd.setFont(&FreeSansBold24pt7b);
  lcd.setTextColor(p < 0 ? NEG : TXT);
  lcd.setCursor(28, 100);
  lcd.print(v);
  int16_t bx0, by0;
  uint16_t bw, bh;
  lcd.getTextBounds(v, 28, 100, &bx0, &by0, &bw, &bh);
  lcd.setFont();
  put(bx0 + bw + 8, 78, "€/kWh", DIM, 2, PANEL);

  lcd.fillRoundRect(28, 112, textWidth(levelName(l), 2) + 16, 24, 6, c);
  put(36, 117, levelName(l), BG, 2);
  if (p < 0) put(150, 120, "¡Precio negativo!", NEG, 1, PANEL);

  // Barra de posicion entre el minimo y el maximo del dia
  int gx = 14, gy = 156, gw = 292;
  for (int i = 0; i < gw; i++) {
    float f = (float)i / gw;
    uint16_t col = f < 0.33f ? CHEAP : (f < 0.66f ? MID : DEAR);
    lcd.drawFastVLine(gx + i, gy, 8, col);
  }
  float span = today.hi - today.lo;
  int mx = gx + (span > 0 ? (int)((p - today.lo) / span * (gw - 1)) : 0);
  lcd.fillTriangle(mx - 5, gy - 7, mx + 5, gy - 7, mx, gy - 1, TXT);
  char a[16], b[16];
  price(a, sizeof(a), today.lo);
  price(b, sizeof(b), today.hi);
  snprintf(line, sizeof(line), "mín %s", a);
  put(gx, gy + 12, line, DIM);
  snprintf(line, sizeof(line), "máx %s", b);
  putRight(gx + gw, gy + 12, line, DIM);

  // Siguiente hora y hora mas barata que queda hoy
  int nh = h + 1;
  float next = nh < 24 ? today.p[nh] : (tomorrow.count ? tomorrow.p[0] : NAN);
  if (!isnan(next)) {
    price(a, sizeof(a), next);
    snprintf(line, sizeof(line), "Siguiente hora: %s %c", a, next < p ? 0x19 : (next > p ? 0x18 : '='));
    put(14, 188, line, next < p ? CHEAP : (next > p ? DEAR : TXT), 1);
  }
  int best = h;
  for (int k = h; k < 24; k++) if (today.has[k] && today.p[k] < today.p[best]) best = k;
  price(a, sizeof(a), today.p[best]);
  if (best == h) snprintf(line, sizeof(line), "Ahora es la hora más barata que queda hoy");
  else snprintf(line, sizeof(line), "Más barata hoy: %02d:00 (%s €)", best, a);
  put(14, 204, line, ACC, 1);
}

// ---------- Graficas de barras (HOY y MANANA) ----------
constexpr int CH_X = 30, CH_Y = 36, CH_W = 282, CH_H = 140;

void chartRange(const Day& d, float& lo, float& hi) {
  lo = min(0.0f, d.lo);
  hi = max(d.hi, lo + 0.01f);
}

int barX(int hIdx) { return CH_X + hIdx * (CH_W / 24); }

void drawChart(const Day& d, int markHour) {
  float lo, hi;
  chartRange(d, lo, hi);
  auto yOf = [&](float v) { return CH_Y + CH_H - (int)((v - lo) / (hi - lo) * CH_H); };
  // ejes y rejilla
  for (int k = 0; k <= 4; k++) {
    float v = lo + (hi - lo) * k / 4;
    int y = yOf(v);
    lcd.drawFastHLine(CH_X, y, CH_W, k == 0 ? LINE : PANEL);
    char s[12];
    price(s, sizeof(s), v, 2);
    putRight(CH_X - 3, y - 3, s, DIM);
  }
  int y0 = yOf(0);
  int bw = CH_W / 24 - 2;
  for (int h = 0; h < 24; h++) {
    if (!d.has[h]) continue;
    int x = barX(h) + 1;
    int y = yOf(d.p[h]);
    uint16_t c = d.p[h] < 0 ? NEG : levelColor(level(d, d.p[h]));
    if (y < y0) lcd.fillRect(x, y, bw, y0 - y, c);
    else lcd.fillRect(x, y0, bw, max(1, y - y0), c);
    if (h == markHour) lcd.drawRect(x - 1, min(y, y0) - 2, bw + 2, abs(y0 - y) + 4, TXT);
  }
  lcd.drawFastHLine(CH_X, y0, CH_W, DIM);
  for (int h = 0; h < 24; h += 3) {
    char s[4];
    snprintf(s, sizeof(s), "%d", h);
    put(barX(h) + 2, CH_Y + CH_H + 4, s, DIM);
  }
}

void drawTooltip(const Day& d, int h) {
  lcd.fillRect(0, 190, W, 36, BG);
  char a[16], line[48];
  if (h >= 0 && d.has[h]) {
    price(a, sizeof(a), d.p[h]);
    snprintf(line, sizeof(line), "%02d:00-%02d:00  %s €/kWh  %s", h, (h + 1) % 24, a, levelName(level(d, d.p[h])));
    lcd.fillRoundRect(8, 192, 304, 18, 5, PANEL);
    put(16, 197, line, levelColor(level(d, d.p[h])), 1, PANEL);
  }
  char lo[12], av[12], hi[12];
  price(lo, sizeof(lo), d.lo);
  price(av, sizeof(av), d.avg);
  price(hi, sizeof(hi), d.hi);
  snprintf(line, sizeof(line), "mín %s   media %s   máx %s", lo, av, hi);
  putCenter(160, 214, line, DIM);
}

void drawDay() {
  clearBody();
  if (!today.count) { noData(lastError); return; }
  int h = currentHour();
  drawChart(today, h);
  drawTooltip(today, selectedBar >= 0 ? selectedBar : h);
}

void drawTomorrow() {
  clearBody();
  if (!tomorrow.count) {
    putCenter(160, 80, "Precios de mañana", TXT, 2);
    putCenter(160, 110, "REE los publica cada tarde, hacia las 20:15.", DIM);
    putCenter(160, 126, "La placa los busca sola desde esa hora.", DIM);
    if (today.count) {
      char a[12], line[48];
      price(a, sizeof(a), today.avg);
      snprintf(line, sizeof(line), "Media de hoy: %s €/kWh", a);
      putCenter(160, 160, line, ACC);
    }
    return;
  }
  drawChart(tomorrow, selectedBar);
  drawTooltip(tomorrow, selectedBar);
  if (selectedBar < 0 && today.count) {
    char line[48];
    float diff = (tomorrow.avg - today.avg) / fabsf(today.avg) * 100.0f;
    snprintf(line, sizeof(line), "Media %s que hoy: %+.0f%%", diff <= 0 ? "más baja" : "más alta", diff);
    lcd.fillRoundRect(8, 192, 304, 18, 5, PANEL);
    putCenter(160, 197, line, diff <= 0 ? CHEAP : DEAR, 1, PANEL);
  }
}

// ---------- Pantalla 3: PLANIFICA ----------
struct Preset { const char* name; int hours; };
const Preset PRESETS[4] = {{"Horno", 1}, {"Lavadora", 2}, {"Lavavajillas", 3}, {"Coche/termo", 4}};

void drawPlan() {
  clearBody();
  put(12, 34, "¿Cuándo enciendo...?", TXT, 2);
  for (int i = 0; i < 4; i++) {
    int x = 8 + i * 77;
    bool sel = PRESETS[i].hours == planHours;
    lcd.fillRoundRect(x, 56, 73, 40, 6, sel ? ACC : PANEL);
    putCenter(x + 36, 64, PRESETS[i].name, sel ? BG : TXT, 1, sel ? ACC : PANEL);
    char hs[8];
    snprintf(hs, sizeof(hs), "%d h", PRESETS[i].hours);
    putCenter(x + 36, 78, hs, sel ? BG : DIM, 1, sel ? ACC : PANEL);
  }
  int h = currentHour();
  if (h < 0 || !today.count) { noData(lastError); return; }

  // Horas disponibles desde ahora: resto de hoy + manana si ya esta publicado
  float seq[48];
  int n = 0;
  for (int k = h; k < 24; k++) seq[n++] = today.has[k] ? today.p[k] : NAN;
  if (tomorrow.count) for (int k = 0; k < 24; k++) seq[n++] = tomorrow.has[k] ? tomorrow.p[k] : NAN;
  int bestStart = -1;
  float bestAvg = 1e9f;
  for (int s = 0; s + planHours <= n; s++) {
    float sum = 0;
    bool ok = true;
    for (int k = 0; k < planHours; k++) { if (isnan(seq[s + k])) { ok = false; break; } sum += seq[s + k]; }
    if (ok && sum / planHours < bestAvg) { bestAvg = sum / planHours; bestStart = s; }
  }
  float nowAvg = 0;
  bool nowOk = planHours <= n;
  for (int k = 0; k < planHours && nowOk; k++) { if (isnan(seq[k])) nowOk = false; else nowAvg += seq[k]; }
  nowAvg /= planHours;

  lcd.fillRoundRect(8, 106, 304, 114, 10, PANEL);
  if (bestStart < 0) {
    putCenter(160, 150, "No quedan horas suficientes hoy", DIM, 1, PANEL);
    return;
  }
  int startAbs = h + bestStart;
  bool nextDay = startAbs >= 24;
  int sh = startAbs % 24, eh = (startAbs + planHours) % 24;
  char line[48], a[12];
  put(20, 116, bestStart == 0 ? "¡Enciéndelo ya!" : "Mejor momento:", DIM, 1, PANEL);
  snprintf(line, sizeof(line), "%02d:00 - %02d:00", sh, eh);
  lcd.setFont(&FreeSansBold24pt7b);
  lcd.setTextColor(CHEAP);
  lcd.setCursor(18, 166);
  lcd.print(line);
  lcd.setFont();
  put(20, 176, nextDay ? "mañana" : "hoy", ACC, 2, PANEL);
  price(a, sizeof(a), bestAvg);
  snprintf(line, sizeof(line), "media %s €/kWh", a);
  putRight(302, 180, line, TXT, 1, PANEL);
  if (nowOk && bestStart > 0 && nowAvg > 0) {
    float save = (nowAvg - bestAvg) / nowAvg * 100.0f;
    snprintf(line, sizeof(line), "%.0f%% más barato que empezar ahora", save);
    put(20, 202, line, CHEAP, 1, PANEL);
  } else if (bestStart > 0) {
    price(a, sizeof(a), nowAvg);
    snprintf(line, sizeof(line), "Empezar ahora: media %s €/kWh", a);
    put(20, 202, line, DIM, 1, PANEL);
  } else {
    put(20, 202, "Es la franja más barata disponible", CHEAP, 1, PANEL);
  }
}

// ---------- Pantalla 5: AJUSTES ----------
const Btn B_BRIGHT = {8, 128, 148, 30}, B_LED = {164, 128, 148, 30};
const Btn B_BEEP = {8, 164, 148, 30}, B_CAL = {164, 164, 148, 30};
const Btn B_WIFI = {8, 196, 304, 24};

bool inside(const Btn& b, int x, int y) { return x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h; }

void toggle(const Btn& b, const char* label, bool on) {
  lcd.fillRoundRect(b.x, b.y, b.w, b.h, 6, on ? CHEAP : PANEL);
  lcd.drawRoundRect(b.x, b.y, b.w, b.h, 6, on ? CHEAP : LINE);
  char s[32];
  snprintf(s, sizeof(s), "%s: %s", label, on ? "SÍ" : "NO");
  putCenter(b.x + b.w / 2, b.y + b.h / 2 - 4, s, on ? BG : TXT, 1, on ? CHEAP : PANEL);
}

void drawSettings() {
  clearBody();
  char line[64];
  lcd.fillRoundRect(8, 32, 304, 88, 8, PANEL);
  if (WiFi.status() == WL_CONNECTED) {
    snprintf(line, sizeof(line), "Wi-Fi: %s (%ld dBm)", WiFi.SSID().c_str(), WiFi.RSSI());
    put(16, 40, line, TXT, 1, PANEL);
    snprintf(line, sizeof(line), "IP: %s", WiFi.localIP().toString().c_str());
    put(16, 54, line, DIM, 1, PANEL);
  } else {
    put(16, 40, "Wi-Fi: sin conexión", DEAR, 1, PANEL);
  }
  if (fetchedAt) {
    uint32_t mins = (millis() - fetchedAt) / 60000;
    snprintf(line, sizeof(line), "Precios descargados hace %lu min", (unsigned long)mins);
  } else {
    snprintf(line, sizeof(line), "Precios: pendiente");
  }
  put(16, 68, line, DIM, 1, PANEL);
  if (lastError[0]) put(16, 82, lastError, MID, 1, PANEL);
  put(16, 96, "Fuente: REE · PVPC península, sin impuestos", DIM, 1, PANEL);
  put(16, 108, "Precio orientativo, no es tu factura", DIM, 1, PANEL);
  toggle(B_BRIGHT, "Brillo auto", autoBright);
  toggle(B_LED, "LED de precio", ledOn);
  toggle(B_BEEP, "Aviso sonoro", beepOn);
  lcd.fillRoundRect(B_CAL.x, B_CAL.y, B_CAL.w, B_CAL.h, 6, PANEL);
  lcd.drawRoundRect(B_CAL.x, B_CAL.y, B_CAL.w, B_CAL.h, 6, LINE);
  putCenter(B_CAL.x + B_CAL.w / 2, B_CAL.y + 11, "Recalibrar táctil", TXT, 1, PANEL);
  lcd.drawRoundRect(B_WIFI.x, B_WIFI.y, B_WIFI.w, B_WIFI.h, 6, DEAR);
  putCenter(160, B_WIFI.y + 8, "Cambiar Wi-Fi (mantén pulsado 2 s)", DEAR);
}

// ---------- Dibujo general ----------
void drawScreen() {
  drawHeader();
  switch (screen) {
    case S_NOW: drawNow(); break;
    case S_DAY: drawDay(); break;
    case S_PLAN: drawPlan(); break;
    case S_TOMORROW: drawTomorrow(); break;
    case S_SETTINGS: drawSettings(); break;
  }
  drawFooter();
}

// Transicion: barrido de una franja de color en la direccion del gesto.
void slideTo(int next, int dir) {
  for (int i = 0; i < 8; i++) {
    int x = dir > 0 ? W - (i + 1) * 40 : i * 40;
    lcd.fillRect(x, 26, 40, 200, PANEL);
    delay(8);
  }
  screen = (next + S_COUNT) % S_COUNT;
  selectedBar = -1;
  drawScreen();
  Serial.printf("MSR_KILOVATIO_SCREEN %s\n", SCREEN_NAMES[screen]);
}

// ---------- Portal Wi-Fi ----------
String htmlEscape(const String& s) {
  String o;
  for (char c : s) {
    if (c == '&') o += "&amp;";
    else if (c == '<') o += "&lt;";
    else if (c == '>') o += "&gt;";
    else if (c == '"') o += "&quot;";
    else if (c == '\'') o += "&#39;";
    else o += c;
  }
  return o;
}

const char PAGE_HEAD[] PROGMEM =
    "<!doctype html><html lang=es><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'>"
    "<title>Kilovatio MS</title><style>body{font-family:system-ui,sans-serif;background:#0b1220;color:#e8eef7;"
    "max-width:420px;margin:auto;padding:20px}h1{font-size:22px}input,select,button{width:100%;box-sizing:border-box;"
    "padding:12px;margin:6px 0 14px;border-radius:10px;border:1px solid #334;background:#152033;color:#fff;font-size:16px}"
    "button{background:#00b4ff;color:#001;font-weight:700;border:0}small{color:#9ab}</style>";

void handleRoot() {
  String page = FPSTR(PAGE_HEAD);
  page += "<h1>⚡ Kilovatio MS</h1><p>Elige tu Wi-Fi de casa (2,4 GHz). La clave se guarda solo en la placa.</p>"
          "<form method=post action=/save><label>Red</label><input name=s list=nets maxlength=32 required>"
          "<datalist id=nets>";
  page += scannedOptions;
  page += "</datalist><label>Contraseña</label><input name=p type=password maxlength=63>"
          "<button>Guardar y conectar</button></form><small>MS Robotika · AB MakerIDE</small>";
  web.send(200, "text/html; charset=utf-8", page);
}

void handleSave() {
  String s = web.arg("s"), p = web.arg("p");
  s.trim();
  if (s.length() < 1 || s.length() > 32 || (p.length() > 0 && (p.length() < 8 || p.length() > 63))) {
    web.send(400, "text/html; charset=utf-8", String(FPSTR(PAGE_HEAD)) +
             "<h1>Revisa los datos</h1><p>La red necesita nombre y la clave entre 8 y 63 caracteres.</p><a href=/>Volver</a>");
    return;
  }
  prefs.putString("ssid", s);
  prefs.putString("pass", p);
  web.send(200, "text/html; charset=utf-8", String(FPSTR(PAGE_HEAD)) +
           "<h1>Guardado</h1><p>La placa se reinicia y se conecta a <b>" + htmlEscape(s) + "</b>.</p>");
  Serial.println("MSR_KILOVATIO_WIFI_SAVED");
  delay(1200);
  ESP.restart();
}

static int qrX = 0, qrY = 0, qrScale = 1;
void drawQr(esp_qrcode_handle_t q) {
  int size = esp_qrcode_get_size(q);
  qrScale = max(1, 150 / (size + 4));
  int total = (size + 4) * qrScale;
  lcd.fillRect(qrX, qrY, total, total, 0xFFFF);
  for (int y = 0; y < size; y++)
    for (int x = 0; x < size; x++)
      if (esp_qrcode_get_module(q, x, y))
        lcd.fillRect(qrX + (x + 2) * qrScale, qrY + (y + 2) * qrScale, qrScale, qrScale, 0x0000);
}

void startPortal() {
  portalMode = true;
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP_STA);
  int n = WiFi.scanNetworks();
  scannedOptions = "";
  for (int i = 0; i < n && i < 15; i++) scannedOptions += "<option value=\"" + htmlEscape(WiFi.SSID(i)) + "\">";
  WiFi.scanDelete();
  char pass[9];
  snprintf(pass, sizeof(pass), "%08lu", (unsigned long)(esp_random() % 100000000UL));
  apPass = pass;
  WiFi.softAP("Kilovatio-MS", pass);
  dns.start(53, "*", WiFi.softAPIP());
  web.on("/", HTTP_GET, handleRoot);
  web.on("/save", HTTP_POST, handleSave);
  web.onNotFound([] { web.sendHeader("Location", "http://192.168.4.1/"); web.send(302, "text/plain", ""); });
  web.begin();

  lcd.fillScreen(BG);
  lcd.fillRect(0, 0, W, 26, PANEL);
  put(10, 9, "KILOVATIO MS · CONFIGURAR WI-FI", ACC, 1, PANEL);
  String qr = "WIFI:T:WPA;S:Kilovatio-MS;P:" + apPass + ";;";
  qrX = 8; qrY = 40;
  esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
  cfg.display_func = drawQr;
  cfg.max_qrcode_version = 6;
  esp_qrcode_generate(&cfg, qr.c_str());
  put(170, 44, "1. Escanea el QR", TXT);
  put(170, 58, "   con el móvil", TXT);
  put(170, 80, "2. Se abre la web;", TXT);
  put(170, 94, "   si no: 192.168.4.1", DIM);
  put(170, 116, "3. Elige tu red y", TXT);
  put(170, 130, "   escribe la clave", TXT);
  put(170, 156, "Red: Kilovatio-MS", ACC);
  put(170, 170, "Clave:", ACC);
  put(212, 166, pass, MID, 2);
  put(170, 196, "La clave de tu Wi-Fi", DIM);
  put(170, 208, "se queda en la placa.", DIM);
  setLed(false, false, true);
  Serial.println("MSR_KILOVATIO_PORTAL ap=Kilovatio-MS");
}

bool connectWifi() {
  String s = prefs.getString("ssid", ""), p = prefs.getString("pass", "");
  if (!s.length()) return false;
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("kilovatio-ms");
  WiFi.begin(s.c_str(), p.c_str());
  lcd.fillScreen(BG);
  putCenter(160, 96, "KILOVATIO MS", MID, 3);
  putCenter(160, 132, "Conectando a la Wi-Fi...", TXT);
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) {
    lcd.fillCircle(130 + (i % 5) * 15, 160, 4, ACC);
    if (i % 5 == 4) lcd.fillRect(120, 154, 80, 12, BG);
    delay(500);
  }
  return WiFi.status() == WL_CONNECTED;
}

// ---------- Gestos ----------
bool touching = false, longFired = false;
int downX = 0, downY = 0, lastX = 0, lastY = 0;
uint32_t downAt = 0, upAt = 0;

void onTap(int x, int y) {
  if (y > 220) return;
  if (screen == S_DAY || screen == S_TOMORROW) {
    const Day& d = screen == S_DAY ? today : tomorrow;
    if (!d.count) return;
    if (x >= CH_X && x < CH_X + CH_W && y >= CH_Y && y < CH_Y + CH_H + 12) {
      selectedBar = (x - CH_X) / (CH_W / 24);
      if (screen == S_DAY) drawTooltip(today, selectedBar);
      else drawScreen();
      beep(1800, 25);
    }
  } else if (screen == S_PLAN) {
    if (y >= 56 && y < 96) {
      int i = constrain((x - 8) / 77, 0, 3);
      planHours = PRESETS[i].hours;
      drawPlan();
      beep(1800, 25);
    }
  } else if (screen == S_SETTINGS) {
    if (inside(B_BRIGHT, x, y)) { autoBright = !autoBright; prefs.putBool("auto", autoBright); updateBrightness(); }
    else if (inside(B_LED, x, y)) { ledOn = !ledOn; prefs.putBool("led", ledOn); applyLed(); }
    else if (inside(B_BEEP, x, y)) { beepOn = !beepOn; prefs.putBool("beep", beepOn); beep(2200, 60); }
    else if (inside(B_CAL, x, y)) { calibrate(); }
    else return;
    drawScreen();
  }
}

void onLongPress(int x, int y) {
  if (screen == S_SETTINGS && inside(B_WIFI, x, y)) {
    prefs.remove("ssid");
    prefs.remove("pass");
    Serial.println("MSR_KILOVATIO_WIFI_FORGOTTEN");
    startPortal();
  }
}

void handleTouch() {
  int rx, ry;
  uint32_t t = millis();
  if (sampleRaw(rx, ry)) {
    int x, y;
    toScreen(rx, ry, x, y);
    if (!touching) { touching = true; longFired = false; downX = x; downY = y; downAt = t; }
    lastX = x; lastY = y; upAt = t;
    if (!longFired && t - downAt > 2000 && abs(x - downX) < 25 && abs(y - downY) < 25) {
      longFired = true;
      onLongPress(downX, downY);
    }
    return;
  }
  if (touching && t - upAt > 60) {
    touching = false;
    if (longFired || portalMode) return;
    int dx = lastX - downX, dy = lastY - downY;
    if (abs(dx) > 50 && abs(dx) > abs(dy)) slideTo(screen + (dx < 0 ? 1 : -1), dx < 0 ? 1 : -1);
    else if (abs(dx) < 15 && abs(dy) < 15) onTap(downX, downY);
  }
}

// ---------- Programa ----------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("MSR_KILOVATIO_BOOT profile=ESP32-2432S028R display=ILI9341 touch=XPT2046");
  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_B, OUTPUT);
  setLed(false, false, false);
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);  // microSD sin usar
  ledcAttach(PIN_BL, 5000, 8);
  ledcWrite(PIN_BL, 230);
  analogSetPinAttenuation(PIN_LDR, ADC_0db);

  lcdBus.begin(14, 12, 13, 15);
  lcd.begin(40000000);
  lcd.setRotation(1);
  lcd.cp437(true);
  touchBus.begin(25, 39, 32, 33);
  touch.begin(touchBus);
  touch.setRotation(1);

  prefs.begin("kilovatio", false);
  autoBright = prefs.getBool("auto", true);
  ledOn = prefs.getBool("led", true);
  beepOn = prefs.getBool("beep", false);
  planHours = 2;
  if (prefs.getBool("cal", false)) {
    ax = prefs.getFloat("ax"); bx = prefs.getFloat("bx"); cx = prefs.getFloat("cx");
    ay = prefs.getFloat("ay"); by = prefs.getFloat("by"); cy = prefs.getFloat("cy");
  } else {
    calibrate();
  }
  updateBrightness();

  if (!connectWifi()) { startPortal(); return; }
  Serial.printf("MSR_KILOVATIO_WIFI_OK rssi=%ld\n", WiFi.RSSI());
  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "es.pool.ntp.org", "pool.ntp.org", "time.google.com");
  putCenter(160, 180, "Sincronizando hora...", DIM);
  struct tm t;
  for (int i = 0; i < 40 && !getLocalTime(&t, 250); i++) delay(100);
  putCenter(160, 196, "Descargando precios de REE...", DIM);
  if (fetchDay(0, today)) fetchedAt = millis();
  lastFetchTry = millis();
  if (now(t)) {
    lastYmd = ymdOf(t);
    if (t.tm_hour >= 20 && fetchDay(1, tomorrow)) fetchedAt = millis();
    lastTomorrowTry = millis();
    lastHourShown = t.tm_hour;
  }
  applyLed();
  drawScreen();
}

void loop() {
  if (portalMode) {
    dns.processNextRequest();
    web.handleClient();
    delay(2);
    return;
  }
  handleTouch();
  uint32_t ms = millis();

  if (ms - lastBright > 1500) { lastBright = ms; updateBrightness(); }

  if (ms - lastMinute > 1000) {
    lastMinute = ms;
    struct tm t;
    if (now(t)) {
      static int lastMin = -1;
      if (t.tm_min != lastMin) { lastMin = t.tm_min; drawHeader(); }
      int ymd = ymdOf(t);
      if (ymd != lastYmd) {  // cambio de dia: manana pasa a ser hoy
        lastYmd = ymd;
        if (tomorrow.count && tomorrow.ymd == ymd) today = tomorrow;
        else today.count = 0;
        tomorrow.count = 0;
        lastFetchTry = 0;
      }
      if (t.tm_hour != lastHourShown) {
        lastHourShown = t.tm_hour;
        applyLed();
        if (today.has[t.tm_hour] && today.p[t.tm_hour] <= today.lo + 1e-6f) { beep(2400, 120); delay(160); beep(2400, 120); }
        if (screen != S_SETTINGS) drawScreen();
      }
      // Hoy: reintenta cada 2 min si falta. Manana: desde las 20:15, cada 15 min.
      if (!today.count && ms - lastFetchTry > 120000UL) {
        lastFetchTry = ms;
        if (WiFi.status() == WL_CONNECTED && fetchDay(0, today)) { fetchedAt = ms; applyLed(); drawScreen(); }
      }
      bool evening = t.tm_hour > 20 || (t.tm_hour == 20 && t.tm_min >= 15);
      if (evening && !tomorrow.count && ms - lastTomorrowTry > 900000UL) {
        lastTomorrowTry = ms;
        if (WiFi.status() == WL_CONNECTED && fetchDay(1, tomorrow)) {
          fetchedAt = ms;
          if (screen == S_TOMORROW || screen == S_PLAN) drawScreen();
        }
      }
    }
    if (WiFi.status() != WL_CONNECTED) {
      static uint32_t lastReconnect = 0;
      if (ms - lastReconnect > 30000) { lastReconnect = ms; WiFi.reconnect(); }
    }
  }
  delay(5);
}
