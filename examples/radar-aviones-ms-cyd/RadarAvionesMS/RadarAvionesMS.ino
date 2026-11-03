/*
  Radar Aviones MS · AB MakerIDE / MS Robotika
  Radar en directo de los aviones que vuelan cerca de ti, para la CYD ESP32-2432S028R.
  Radar circular con barrido + ficha del avión seleccionado (vuelo, modelo, altura,
  velocidad, distancia y rumbo). Toca un punto del radar para elegir avión; toca la
  ficha para pasar al siguiente. Mantén pulsado 3 s para cambiar Wi-Fi o ubicación.

  Datos: API pública de adsb.lol (ADS-B comunitario, licencia ODbL), sin cuenta ni clave.
  HTTPS validado con las raíces ISRG X1 y X2 (Let's Encrypt) incluidas abajo.
  Wi-Fi y ubicación: la primera vez crea la red "RadarAviones-MS" con un QR; desde el
  móvil eliges tu red y escribes tus coordenadas. Se guardan solo en la placa (NVS).
  Si la placa ya tenía la Wi-Fi o la calibración de Kilovatio MS, las reutiliza.

  La pantalla no muestra tus coordenadas: solo la posición de los aviones respecto a ti.
  Posiciones aproximadas (ADS-B con unos segundos de retraso). No es para navegación.

  Librerías: Adafruit GFX, Adafruit ILI9341, XPT2046_Touchscreen (Paul Stoffregen),
  ArduinoJson 7. Placa: "ESP32 Dev Module" (esp32 3.x). Licencia MIT.
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
#include <qrcode.h>

// ---------- Hardware de la CYD ----------
SPIClass lcdBus(HSPI), touchBus(VSPI);
Adafruit_ILI9341 lcd(&lcdBus, 2, 15, -1);
XPT2046_Touchscreen touch(33, 36);
constexpr int LED_R = 4, LED_G = 16, LED_B = 17;  // activo a nivel bajo
constexpr int PIN_BL = 21;
constexpr int W = 320, H = 240;

// ---------- Colores ----------
constexpr uint16_t BG = 0x0000, PANEL = 0x0861, LINE = 0x0240, TXT = 0xFFFF, DIM = 0x8C71;
constexpr uint16_t GRN = 0x07E0, GRN2 = 0x03E0, AMB = 0xFD20, ACC = 0x07FF, RED = 0xF8A6;

// ---------- Radar ----------
constexpr int RANGE_NM = 25;                  // radio de búsqueda en millas náuticas
constexpr float RANGE_KM = RANGE_NM * 1.852f; // ~46 km
constexpr int RS = 200, RCX = 100, RCY = 100, RR = 96;  // lienzo 200x200, centro y radio
constexpr int RX0 = 4, RY0 = 32;              // posición del lienzo en pantalla
constexpr int MAXP = 40;
constexpr uint32_t FETCH_MS = 10000, STALE_MS = 60000;

const char ROOT_CA[] PROGMEM = R"PEM(-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQsw
CQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2gg
R3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00
MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVTMSkwJwYDVQQKEyBJbnRlcm5ldCBT
ZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNSRyBSb290IFgyMHYw
EAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0HttwW
+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9
ItgKbppbd9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0T
AQH/BAUwAwEB/zAdBgNVHQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZI
zj0EAwMDaAAwZQIwe3lORlCEwkSHRhtFcP9Ymd70/aTSVaYgLXTWNLxBo1BfASdW
tL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5U6VR5CmD1/iQMVtCnwr1
/q4AaOeMSQ+2b1tbFfLn
-----END CERTIFICATE-----)PEM";

struct Plane {
  char flight[10], type[6];
  float x, y;      // km al este y al norte de ti
  float altM;      // metros; -1 = en tierra
  float kmh, track;
  float glow;      // brillo del punto tras pasar el barrido (1 = recién visto)
};
Plane planes[MAXP];
int nPlanes = 0, selected = -1;
char selFlight[10] = "";

// Se reserva en setup(): 80 KB a nivel global agotan la RAM antes de que arranque loop().
GFXcanvas16* radarBuf = nullptr;
Preferences prefs;
WebServer web(80);
DNSServer dns;
bool portalMode = false;
String scannedOptions;
float homeLat = 40.4168f, homeLon = -3.7038f;  // Madrid centro por defecto
float ax = 0, bx = 0, cx = 0, ay = 0, by = 0, cy = 0;
float sweep = 0;
uint32_t lastFetch = 0, fetchedAt = 0, lastFrame = 0, lastPanel = 0;
char lastError[40] = "";

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
void put(Adafruit_GFX& g, int x, int y, const char* s, uint16_t fg, uint8_t size = 1, int32_t bg = -1) {
  g.setFont();
  for (const uint8_t* p = (const uint8_t*)s; *p; x += 6 * size) g.drawChar(x, y, mapUtf8(p), fg, bg < 0 ? fg : (uint16_t)bg, size);
}
void put(int x, int y, const char* s, uint16_t fg, uint8_t size = 1, int32_t bg = -1) { put(lcd, x, y, s, fg, size, bg); }
void putCenter(int cxm, int y, const char* s, uint16_t fg, uint8_t size = 1, int32_t bg = -1) { put(cxm - textWidth(s, size) / 2, y, s, fg, size, bg); }

void setLed(bool r, bool g, bool b) { digitalWrite(LED_R, !r); digitalWrite(LED_G, !g); digitalWrite(LED_B, !b); }

// ---------- Geometría ----------
// Distancia (NM) y rumbo desde ti (grados, 0 = norte) a km este/norte.
void polarToXY(float dstNm, float dirDeg, float& x, float& y) {
  float d = dstNm * 1.852f, a = dirDeg * DEG_TO_RAD;
  x = d * sinf(a); y = d * cosf(a);
}
// Avance estimado entre descargas: velocidad y rumbo constantes durante dt segundos.
void deadReckon(Plane& p, float dt) {
  if (p.altM < 0 || p.kmh <= 0) return;
  float d = p.kmh * dt / 3600.0f, a = p.track * DEG_TO_RAD;
  p.x += d * sinf(a); p.y += d * cosf(a);
}
bool selfTest() {
  Plane p = {};
  polarToXY(10, 90, p.x, p.y);          // 10 NM al este
  p.kmh = 360 * 1.852f; p.track = 270;   // 360 nudos hacia el oeste
  deadReckon(p, 60);                     // 1 minuto = 6 NM
  float dist = sqrtf(p.x * p.x + p.y * p.y) / 1.852f;
  return fabsf(dist - 4.0f) < 0.01f && p.x > 0 && fabsf(p.y) < 0.01f;
}

// ---------- Descarga (adsb.lol) ----------
bool fetchPlanesOnce();
bool fetchPlanes() {
  bool ok = fetchPlanesOnce();
  if (!ok) Serial.printf("MSR_RADAR_ERROR %s heap=%u\n", lastError, (unsigned)ESP.getFreeHeap());
  return ok;
}
bool fetchPlanesOnce() {
  char url[96];
  snprintf(url, sizeof(url), "https://api.adsb.lol/v2/point/%.4f/%.4f/%d", homeLat, homeLon, RANGE_NM);
  WiFiClientSecure client;
  client.setCACert(ROOT_CA);
  client.setTimeout(10);
  HTTPClient http;
  http.setTimeout(10000);
  if (!http.begin(client, url)) { strlcpy(lastError, "No se pudo abrir HTTPS", sizeof(lastError)); return false; }
  http.setUserAgent("RadarAviones-MS/1.0 (CYD; github.com/msrobotika)");
  int code = http.GET();
  if (code != 200) { snprintf(lastError, sizeof(lastError), "adsb.lol respondió %d", code); http.end(); return false; }

  JsonDocument filter;
  JsonObject f = filter["ac"][0].to<JsonObject>();
  for (const char* k : {"flight", "t", "r", "alt_baro", "gs", "track", "dst", "dir"}) f[k] = true;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (err) { snprintf(lastError, sizeof(lastError), "JSON: %s", err.c_str()); return false; }

  int n = 0;
  for (JsonObject a : doc["ac"].as<JsonArray>()) {
    if (n >= MAXP) break;
    if (a["dst"].isNull() || a["dir"].isNull()) continue;  // sin posición
    Plane& p = planes[n];
    const char* fl = a["flight"] | (a["r"] | "?");
    strlcpy(p.flight, fl, sizeof(p.flight));
    for (int i = strlen(p.flight) - 1; i >= 0 && p.flight[i] == ' '; i--) p.flight[i] = 0;
    strlcpy(p.type, a["t"] | "", sizeof(p.type));
    polarToXY(a["dst"].as<float>(), a["dir"].as<float>(), p.x, p.y);
    p.altM = a["alt_baro"].is<int>() ? a["alt_baro"].as<int>() * 0.3048f : -1;  // "ground" = en tierra
    p.kmh = (a["gs"] | 0.0f) * 1.852f;
    p.track = a["track"] | 0.0f;
    p.glow = 0;
    n++;
  }
  nPlanes = n;
  // Mantener el avión elegido entre descargas; si se fue, elegir el más cercano.
  selected = -1;
  for (int i = 0; i < n; i++) if (!strcmp(planes[i].flight, selFlight)) selected = i;
  if (selected < 0) selectNearest(0, 0, 1e9f);
  lastError[0] = 0;
  Serial.printf("MSR_RADAR_PLANES n=%d\n", n);
  return true;
}

void selectIndex(int i) {
  selected = i;
  strlcpy(selFlight, i >= 0 ? planes[i].flight : "", sizeof(selFlight));
}
void selectNearest(float x, float y, float maxKm2) {
  int best = -1;
  float bd = maxKm2;
  for (int i = 0; i < nPlanes; i++) {
    float d = sq(planes[i].x - x) + sq(planes[i].y - y);
    if (d < bd) { bd = d; best = i; }
  }
  if (best >= 0 || maxKm2 > 1e8f) selectIndex(best);
}

// ---------- Dibujo ----------
void toCanvas(float xKm, float yKm, int& px, int& py) {
  px = RCX + lroundf(xKm / RANGE_KM * RR);
  py = RCY - lroundf(yKm / RANGE_KM * RR);
}

void drawRadar() {
  radarBuf->fillScreen(BG);
  for (int k = 1; k <= 3; k++) radarBuf->drawCircle(RCX, RCY, RR * k / 3, LINE);
  radarBuf->drawFastHLine(RCX - RR, RCY, 2 * RR, LINE);
  radarBuf->drawFastVLine(RCX, RCY - RR, 2 * RR, LINE);
  put(*radarBuf, RCX - 2, 2, "N", GRN2);
  char s[8];
  snprintf(s, sizeof(s), "%d", (int)lroundf(RANGE_KM / 3));
  put(*radarBuf, RCX + RR / 3 + 2, RCY + 3, s, GRN2);
  snprintf(s, sizeof(s), "%dkm", (int)lroundf(RANGE_KM));
  put(*radarBuf, RCX + RR - 22, RCY + 3, s, GRN2);
  // Barrido con estela
  for (int i = 0; i < 14; i++) {
    float a = (sweep - i * 2.5f) * DEG_TO_RAD;
    uint8_t g = 63 - i * 4;
    radarBuf->drawLine(RCX, RCY, RCX + RR * sinf(a), RCY - RR * cosf(a), (uint16_t)(g << 5));
  }
  // Tú, en el centro
  radarBuf->fillTriangle(RCX - 4, RCY + 3, RCX + 4, RCY + 3, RCX, RCY - 3, ACC);
  // Aviones
  for (int i = 0; i < nPlanes; i++) {
    Plane& p = planes[i];
    int px, py;
    toCanvas(p.x, p.y, px, py);
    if (sq(px - RCX) + sq(py - RCY) > RR * RR) continue;
    float ang = atan2f(p.x, p.y) * RAD_TO_DEG;
    if (ang < 0) ang += 360;
    float diff = fmodf(sweep - ang + 360, 360);
    if (diff < 8) p.glow = 1;  // el barrido acaba de pasar
    uint16_t c = p.altM < 0 ? DIM : (p.glow > 0.5f ? GRN : GRN2);
    if (i == selected) {
      radarBuf->drawCircle(px, py, 6, AMB);
      int hx = px + 10 * sinf(p.track * DEG_TO_RAD), hy = py - 10 * cosf(p.track * DEG_TO_RAD);
      radarBuf->drawLine(px, py, hx, hy, AMB);
      put(*radarBuf, constrain(px + 7, 0, RS - 48), constrain(py - 10, 0, RS - 8), p.flight, AMB);
      c = AMB;
    }
    radarBuf->fillRect(px - 2, py - 2, 5, 5, c);
  }
  lcd.drawRGBBitmap(RX0, RY0, radarBuf->getBuffer(), RS, RS);
}

void drawHeader() {
  lcd.fillRect(0, 0, W, 26, PANEL);
  lcd.drawFastHLine(0, 26, W, GRN2);
  put(8, 6, "RADAR AVIONES", GRN, 2, PANEL);
  put(176, 10, "MS ROBOTIKA", DIM, 1, PANEL);
  put(250, 10, WiFi.status() == WL_CONNECTED ? "EN VIVO" : "SIN WIFI", WiFi.status() == WL_CONNECTED ? GRN : RED, 1, PANEL);
}

void drawPanel() {
  const int x0 = 210, w = 106;
  lcd.fillRect(x0, 30, W - x0, 210, BG);  // hasta el borde: un texto largo no deja restos
  lcd.drawRoundRect(x0, 32, w, 150, 6, GRN2);
  char line[32];
  if (selected < 0 || selected >= nPlanes) {
    put(x0 + 8, 44, nPlanes ? "Toca un avión" : "Sin aviones", DIM);
    put(x0 + 8, 58, nPlanes ? "en el radar" : "a la vista", DIM);
  } else {
    Plane& p = planes[selected];
    put(x0 + 6, 40, p.flight, AMB, 2);
    put(x0 + 6, 60, p.type[0] ? p.type : "modelo ?", DIM);
    put(x0 + 6, 78, "ALTURA", DIM);
    if (p.altM < 0) strlcpy(line, "en tierra", sizeof(line));
    else snprintf(line, sizeof(line), "%d m", (int)(lroundf(p.altM / 10) * 10));
    put(x0 + 6, 88, line, TXT, 2);
    put(x0 + 6, 110, "VELOCIDAD", DIM);
    snprintf(line, sizeof(line), "%d km/h", (int)lroundf(p.kmh));
    put(x0 + 6, 120, line, TXT);
    put(x0 + 6, 136, "DISTANCIA", DIM);
    snprintf(line, sizeof(line), "%.1f km", sqrtf(p.x * p.x + p.y * p.y));
    put(x0 + 6, 146, line, TXT);
    static const char* const DIRS[8] = {"N", "NE", "E", "SE", "S", "SO", "O", "NO"};
    snprintf(line, sizeof(line), "rumbo %s", DIRS[(int)((p.track + 22.5f) / 45) % 8]);
    put(x0 + 6, 164, line, ACC);
  }
  snprintf(line, sizeof(line), "%d aviones", nPlanes);
  put(x0 + 4, 190, line, GRN);
  snprintf(line, sizeof(line), "radio %d km", (int)lroundf(RANGE_KM));
  put(x0 + 4, 202, line, DIM);
  if (lastError[0]) { char e[18]; strlcpy(e, lastError, sizeof(e)); put(x0 + 4, 216, e, RED); }
  else if (fetchedAt) { snprintf(line, sizeof(line), "hace %lus", (unsigned long)((millis() - fetchedAt) / 1000)); put(x0 + 4, 216, line, DIM); }
  put(x0 + 4, 228, "datos: adsb.lol", DIM);
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

// Calibración guardada: la propia o, si no hay, la de Kilovatio MS (misma rotación).
bool loadCalibration() {
  for (const char* ns : {"radaravion", "kilovatio"}) {
    Preferences p;
    if (!p.begin(ns, true)) continue;
    bool ok = p.getBool("cal", false);
    if (ok) { ax = p.getFloat("ax"); bx = p.getFloat("bx"); cx = p.getFloat("cx"); ay = p.getFloat("ay"); by = p.getFloat("by"); cy = p.getFloat("cy"); }
    p.end();
    if (ok) return true;
  }
  return false;
}

// ---------- Portal Wi-Fi y ubicación ----------
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
    "<title>Radar Aviones MS</title><style>body{font-family:system-ui,sans-serif;background:#04110a;color:#e8f7ee;"
    "max-width:420px;margin:auto;padding:20px}h1{font-size:22px}input,button{width:100%;box-sizing:border-box;"
    "padding:12px;margin:6px 0 14px;border-radius:10px;border:1px solid #2a4;background:#0b2215;color:#fff;font-size:16px}"
    "button{background:#22dd66;color:#012;font-weight:700;border:0}small{color:#9cb}</style>";

void handleRoot() {
  String page = FPSTR(PAGE_HEAD);
  page += "<h1>✈️ Radar Aviones MS</h1><form method=post action=/save>"
          "<label>Wi-Fi de casa (2,4 GHz)</label><input name=s list=nets maxlength=32 placeholder='vacío = no cambiar'><datalist id=nets>";
  page += scannedOptions;
  page += "</datalist><label>Contraseña</label><input name=p type=password maxlength=63>"
          "<label>Latitud</label><input name=la inputmode=decimal required value='" + String(homeLat, 4) + "'>"
          "<label>Longitud</label><input name=lo inputmode=decimal required value='" + String(homeLon, 4) + "'>"
          "<small>En Google Maps, mantén pulsado tu zona y copia los dos números. Basta con 2 decimales "
          "(unos 1 km de precisión). Se guardan solo en la placa y no salen en pantalla.</small>"
          "<button>Guardar</button></form><small>MS Robotika · AB MakerIDE · datos de adsb.lol</small>";
  web.send(200, "text/html; charset=utf-8", page);
}

float parseCoord(String v) { v.trim(); v.replace(',', '.'); return v.length() ? v.toFloat() : NAN; }

void handleSave() {
  String s = web.arg("s"), p = web.arg("p");
  s.trim();
  float la = parseCoord(web.arg("la")), lo = parseCoord(web.arg("lo"));
  bool badWifi = s.length() > 32 || (s.length() && p.length() > 0 && (p.length() < 8 || p.length() > 63));
  bool badPos = isnan(la) || isnan(lo) || la < -90 || la > 90 || lo < -180 || lo > 180 || (la == 0 && lo == 0);
  bool noWifi = !s.length() && !prefs.isKey("ssid");
  if (badWifi || badPos || noWifi) {
    web.send(400, "text/html; charset=utf-8", String(FPSTR(PAGE_HEAD)) +
             "<h1>Revisa los datos</h1><p>Hace falta tu Wi-Fi (clave de 8 a 63 caracteres) y unas coordenadas válidas, "
             "por ejemplo 40.42 y -3.70.</p><a href=/>Volver</a>");
    return;
  }
  if (s.length()) { prefs.putString("ssid", s); prefs.putString("pass", p); }
  prefs.putFloat("lat", la);
  prefs.putFloat("lon", lo);
  web.send(200, "text/html; charset=utf-8", String(FPSTR(PAGE_HEAD)) + "<h1>Guardado</h1><p>La placa se reinicia.</p>");
  Serial.println("MSR_RADAR_SETTINGS_SAVED");
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
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP_STA);
  int n = WiFi.scanNetworks();
  scannedOptions = "";
  for (int i = 0; i < n && i < 15; i++) scannedOptions += "<option value=\"" + htmlEscape(WiFi.SSID(i)) + "\">";
  WiFi.scanDelete();
  char pass[9];
  snprintf(pass, sizeof(pass), "%08lu", (unsigned long)(esp_random() % 100000000UL));
  WiFi.softAP("RadarAviones-MS", pass);
  dns.start(53, "*", WiFi.softAPIP());
  web.on("/", HTTP_GET, handleRoot);
  web.on("/save", HTTP_POST, handleSave);
  web.onNotFound([] { web.sendHeader("Location", "http://192.168.4.1/"); web.send(302, "text/plain", ""); });
  web.begin();

  lcd.fillScreen(BG);
  lcd.fillRect(0, 0, W, 26, PANEL);
  put(10, 9, "RADAR AVIONES · WI-FI Y UBICACIÓN", GRN, 1, PANEL);
  String qr = "WIFI:T:WPA;S:RadarAviones-MS;P:" + String(pass) + ";;";
  qrX = 8; qrY = 40;
  esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
  cfg.display_func = drawQr;
  cfg.max_qrcode_version = 6;
  esp_qrcode_generate(&cfg, qr.c_str());
  put(170, 44, "1. Escanea el QR", TXT);
  put(170, 58, "   con el móvil", TXT);
  put(170, 80, "2. Se abre la web;", TXT);
  put(170, 94, "   si no: 192.168.4.1", DIM);
  put(170, 116, "3. Pon tu Wi-Fi y", TXT);
  put(170, 130, "   tu ubicación", TXT);
  put(170, 156, "Red: RadarAviones-MS", ACC);
  put(170, 170, "Clave:", ACC);
  put(212, 166, pass, AMB, 2);
  setLed(false, false, true);
  Serial.println("MSR_RADAR_PORTAL ap=RadarAviones-MS");
}

// Wi-Fi guardada: la propia o, si no hay, la de Kilovatio MS.
bool connectWifi() {
  String s = prefs.getString("ssid", ""), p = prefs.getString("pass", "");
  if (!s.length()) {
    Preferences k;
    if (k.begin("kilovatio", true)) { s = k.getString("ssid", ""); p = k.getString("pass", ""); k.end(); }
  }
  if (!s.length()) return false;
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("radar-aviones-ms");
  WiFi.begin(s.c_str(), p.c_str());
  lcd.fillScreen(BG);
  putCenter(160, 96, "RADAR AVIONES", GRN, 3);
  putCenter(160, 132, "Conectando a la Wi-Fi...", TXT);
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) {
    lcd.fillCircle(130 + (i % 5) * 15, 160, 4, GRN);
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
  if (x < RX0 + RS && y > RY0) {  // radar: el avión más cercano al dedo (hasta ~5 km)
    float kx = (x - RX0 - RCX) * RANGE_KM / RR, ky = -(y - RY0 - RCY) * RANGE_KM / RR;
    selectNearest(kx, ky, 25.0f);
  } else if (x >= 210 && y < 182 && nPlanes) {  // ficha: siguiente avión, del más cercano al más lejano
    float cur = selected >= 0 ? sq(planes[selected].x) + sq(planes[selected].y) : -1;
    int next = -1;
    float nd = 1e9f;
    for (int i = 0; i < nPlanes; i++) {
      float d = sq(planes[i].x) + sq(planes[i].y);
      if (d > cur + 1e-4f && d < nd) { nd = d; next = i; }
    }
    if (next < 0) selectNearest(0, 0, 1e9f); else selectIndex(next);
  }
  drawPanel();
}

void handleTouch() {
  int rx, ry;
  uint32_t t = millis();
  if (sampleRaw(rx, ry)) {
    int x, y;
    toScreen(rx, ry, x, y);
    if (!touching) { touching = true; longFired = false; downX = x; downY = y; downAt = t; }
    upAt = t;
    if (!longFired && t - downAt > 3000) { longFired = true; startPortal(); }
    return;
  }
  if (touching && t - upAt > 60) {
    touching = false;
    if (!longFired && !portalMode) onTap(downX, downY);
  }
}

// ---------- Programa ----------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("MSR_RADAR_BOOT selftest=%s\n", selfTest() ? "OK" : "FAIL");
  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_B, OUTPUT);
  setLed(false, false, false);
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);  // microSD sin usar
  pinMode(PIN_BL, OUTPUT);
  digitalWrite(PIN_BL, HIGH);

  lcdBus.begin(14, 12, 13, 15);
  lcd.begin(40000000);
  lcd.setRotation(1);
  lcd.cp437(true);
  touchBus.begin(25, 39, 32, 33);
  touch.begin(touchBus);
  touch.setRotation(1);

  prefs.begin("radaravion", false);
  homeLat = prefs.getFloat("lat", homeLat);
  homeLon = prefs.getFloat("lon", homeLon);
  if (!loadCalibration()) calibrate();
  radarBuf = new GFXcanvas16(RS, RS);
  if (!radarBuf->getBuffer()) { putCenter(160, 110, "Sin memoria para el radar", RED, 2); while (true) delay(1000); }

  if (!connectWifi()) { startPortal(); return; }
  Serial.printf("MSR_RADAR_WIFI_OK rssi=%ld\n", WiFi.RSSI());
  lcd.fillScreen(BG);
  drawHeader();
  if (fetchPlanes()) fetchedAt = millis();
  lastFetch = millis();
  drawPanel();
}

void loop() {
  if (portalMode) { dns.processNextRequest(); web.handleClient(); delay(2); return; }
  handleTouch();
  uint32_t ms = millis();

  if (ms - lastFetch > FETCH_MS) {
    lastFetch = ms;
    // shortcut: la descarga bloquea ~1 s y el barrido se para un instante; si molesta, pasarla a otra tarea FreeRTOS.
    if (WiFi.status() == WL_CONNECTED && fetchPlanes()) fetchedAt = millis();
    else if (WiFi.status() != WL_CONNECTED) { strlcpy(lastError, "Sin Wi-Fi", sizeof(lastError)); WiFi.reconnect(); }
    if (fetchedAt && millis() - fetchedAt > STALE_MS) { nPlanes = 0; selected = -1; }  // no enseñar datos viejos
    drawHeader();
    drawPanel();
    lastPanel = millis();
  }

  if (ms - lastFrame >= 40) {
    float dt = (ms - lastFrame) / 1000.0f;
    lastFrame = ms;
    sweep = fmodf(sweep + 4.0f, 360);
    for (int i = 0; i < nPlanes; i++) { deadReckon(planes[i], dt); planes[i].glow *= 0.985f; }
    drawRadar();
  }
  if (ms - lastPanel > 1000) { lastPanel = ms; drawPanel(); }
  delay(2);
}
