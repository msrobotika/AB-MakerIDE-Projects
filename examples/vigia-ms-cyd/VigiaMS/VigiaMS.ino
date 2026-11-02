/*
  Vigia MS · AB MakerIDE / MS Robotika
  Detector de rastreadores Bluetooth para la CYD ESP32-2432S028R (ILI9341 320x240 en
  horizontal, tactil XPT2046). Escucha de forma pasiva los anuncios Bluetooth de alrededor
  (no se conecta a nada ni envia nada) y reconoce los rastreadores mas comunes:
    - Apple Find My (AirTag y accesorios compatibles), y si estan lejos de su dueno
    - Samsung SmartTag
    - Tile
    - Rastreadores de la red Encontrar de Google (Find Hub)
  Si un rastreador sigue cerca durante los minutos que elijas, salta la alarma.

  Cinco pantallas que se cambian deslizando: VIGILANCIA, RADAR, LISTA, BUSCAR y AJUSTES.
  Limites: los rastreadores cambian de direccion Bluetooth para proteger la privacidad,
  asi que no se garantiza detectar todos ni seguirlos sin cortes. No sustituye a las
  alertas de rastreo del movil (iOS y Android) ni a la policia si te sientes en peligro.

  Librerias: Adafruit GFX, Adafruit ILI9341, XPT2046_Touchscreen (Paul Stoffregen),
  NimBLE-Arduino 2.x (h2zero). Placa: "ESP32 Dev Module" (esp32 3.x).
  Referencia de pines: github.com/witnessmenow/ESP32-Cheap-Yellow-Display/PINS.md
  Licencia MIT, como el resto de AB MakerIDE.
*/
#include <Arduino.h>
#include <SPI.h>
#include <Preferences.h>
#include <NimBLEDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

// ---------- Hardware de la CYD ----------
SPIClass lcdBus(HSPI), touchBus(VSPI);
Adafruit_ILI9341 lcd(&lcdBus, 2, 15, -1);
XPT2046_Touchscreen touch(33, 36);
constexpr int LED_R = 4, LED_G = 16, LED_B = 17;  // activo a nivel bajo
constexpr int PIN_BL = 21, PIN_LDR = 34, PIN_SPK = 26;
constexpr int W = 320, H = 240;

// ---------- Colores ----------
constexpr uint16_t BG = 0x0861, PANEL = 0x18E4, LINE = 0x31A6, TXT = 0xFFFF, DIM = 0x9CF3, INK = 0x0000;
constexpr uint16_t SAFE = 0x2E8A, WARN = 0xFE41, ALERT = 0xF8A6, ACC = 0x05FF;
constexpr uint16_t C_APPLE = 0xFFFF, C_SAMSUNG = 0x4C9F, C_TILE = 0xFD20, C_GOOGLE = 0x87F0;

// ---------- Tipos de rastreador ----------
enum TType : uint8_t { T_NONE, T_APPLE, T_SAMSUNG, T_TILE, T_GOOGLE };
enum Sep : uint8_t { S_NEAR_OWNER, S_SEPARATED, S_UNKNOWN };
const char* const TYPE_NAME[] = {"", "Apple Find My", "Samsung SmartTag", "Tile", "Google Encontrar"};
const uint16_t TYPE_COLOR[] = {DIM, C_APPLE, C_SAMSUNG, C_TILE, C_GOOGLE};

struct Btn { int x, y, w, h; };  // aqui arriba para los prototipos automaticos de Arduino

struct Adv {        // lo que el escaneo pasa al bucle principal
  uint64_t addr;
  int8_t rssi;
  uint8_t type;
  uint8_t sep;
};

struct Tracker {
  uint64_t addr;
  uint8_t id;       // numero corto para la pantalla: no se muestra la direccion
  uint8_t type;
  uint8_t sep;
  int8_t rssi;
  float rssiF;      // RSSI suavizado
  uint32_t first, last;
  bool ignored;
  bool used;
};

constexpr int MAX_TRACKERS = 16;
constexpr uint32_t FORGET_MS = 180000;    // se olvida tras 3 min sin oirlo
constexpr uint32_t ACTIVE_MS = 60000;     // "presente" si se oyo en el ultimo minuto
Tracker trackers[MAX_TRACKERS];
uint8_t nextId = 1;

// Dispositivos Bluetooth en general (solo para contarlos)
constexpr int MAX_SEEN = 96;
uint64_t seenAddr[MAX_SEEN];
uint32_t seenAt[MAX_SEEN];

QueueHandle_t advQueue;
volatile uint32_t advCount = 0;

// El nucleo esp32 3.x libera la memoria de Bluetooth al arrancar si cree que nadie la usa.
// NimBLE-Arduino no se lo indica, asi que lo declaramos aqui; sin esto la placa se reinicia.
extern "C" bool btInUse(void) { return true; }

// ---------- Clasificacion (se ejecuta en la tarea de Bluetooth) ----------
uint16_t uuid16(const NimBLEUUID& u) {
  if (u.bitSize() != 16) return 0;
  const uint8_t* v = u.getValue();
  return v[0] | (v[1] << 8);
}

void classify(const NimBLEAdvertisedDevice* d, uint8_t& type, uint8_t& sep) {
  type = T_NONE;
  sep = S_UNKNOWN;
  for (uint8_t i = 0; i < d->getManufacturerDataCount(); i++) {
    std::string m = d->getManufacturerData(i);
    // Apple (0x004C), anuncio Find My (0x12). Longitud 0x19: lejos de su dueno; 0x02: con su dueno.
    if (m.size() >= 4 && (uint8_t)m[0] == 0x4C && (uint8_t)m[1] == 0x00 && (uint8_t)m[2] == 0x12) {
      type = T_APPLE;
      sep = (uint8_t)m[3] >= 0x19 ? S_SEPARATED : S_NEAR_OWNER;
      return;
    }
  }
  for (uint8_t i = 0; i < d->getServiceDataCount(); i++) {
    uint16_t u = uuid16(d->getServiceDataUUID(i));
    if (u == 0xFD5A) { type = T_SAMSUNG; return; }
    if (u == 0xFEED || u == 0xFEEC) { type = T_TILE; return; }
    if (u == 0xFEAA) {
      std::string s = d->getServiceData(i);
      // Red Encontrar de Google: trama 0x40 normal, 0x41 modo de proteccion contra rastreo.
      if (!s.empty() && ((uint8_t)s[0] == 0x40 || (uint8_t)s[0] == 0x41)) {
        type = T_GOOGLE;
        sep = (uint8_t)s[0] == 0x41 ? S_SEPARATED : S_UNKNOWN;
        return;
      }
    }
  }
  for (uint8_t i = 0; i < d->getServiceUUIDCount(); i++) {
    uint16_t u = uuid16(d->getServiceUUID(i));
    if (u == 0xFEED || u == 0xFEEC) { type = T_TILE; return; }
  }
}

class ScanEvents : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* d) override {
    Adv a;
    a.addr = (uint64_t)d->getAddress();
    a.rssi = d->getRSSI();
    classify(d, a.type, a.sep);
    advCount = advCount + 1;
    xQueueSend(advQueue, &a, 0);  // si la cola esta llena se pierde este anuncio, no pasa nada
  }
  void onScanEnd(const NimBLEScanResults&, int) override {
    NimBLEDevice::getScan()->start(0, false, true);  // escaneo continuo
  }
};

void startScan() {
  advQueue = xQueueCreate(64, sizeof(Adv));
  NimBLEDevice::init("");
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setScanCallbacks(new ScanEvents(), true);  // queremos cada anuncio, tambien repetidos
  scan->setActiveScan(false);                      // pasivo: no pedimos nada a los dispositivos
  scan->setDuplicateFilter(0);
  scan->setInterval(100);
  scan->setWindow(90);
  scan->setMaxResults(0);                          // no guardar resultados: los procesamos al vuelo
  scan->start(0, false, true);
  Serial.println("MSR_VIGIA_SCANNING passive=1");
}

// ---------- Estado ----------
Preferences prefs;
int screen = 0;
constexpr int SCREENS = 5;
const char* const SCREEN_NAMES[SCREENS] = {"VIGILANCIA", "RADAR", "LISTA", "BUSCAR", "AJUSTES"};
const uint8_t ALERT_OPTIONS[] = {1, 5, 10, 20};
uint8_t alertMin = 10;
bool soundOn = true, autoBright = true;
int selected = -1;          // indice del rastreador elegido en BUSCAR
bool alarmOn = false, lastAlarmOn = false;
float ax = 0, bx = 0, cx = 0, ay = 0, by = 0, cy = 0;
uint32_t lastDraw = 0, lastBright = 0, nextBeep = 0, sweep = 0;

// ---------- Texto con acentos (fuente clasica CP437) ----------
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
  if (c == 0xC2) {
    uint8_t d = *p++;
    return d == 0xBF ? 0xA8 : d == 0xA1 ? 0xAD : d == 0xB7 ? 0xFA : '?';
  }
  while ((*p & 0xC0) == 0x80) p++;
  return '?';
}

int textWidth(const char* s, uint8_t size) {
  int n = 0;
  for (const uint8_t* p = (const uint8_t*)s; *p;) { mapUtf8(p); n++; }
  return n * 6 * size;
}

void put(int x, int y, const char* s, uint16_t fg, uint8_t size = 1, int32_t bg = -1) {
  int x0 = x;
  for (const uint8_t* p = (const uint8_t*)s; *p;) {
    uint8_t ch = mapUtf8(p);
    lcd.drawChar(x0, y, ch, fg, bg < 0 ? fg : (uint16_t)bg, size);
    x0 += 6 * size;
  }
}

void putCenter(int cxm, int y, const char* s, uint16_t fg, uint8_t size = 1, int32_t bg = -1) {
  put(cxm - textWidth(s, size) / 2, y, s, fg, size, bg);
}

void putRight(int xr, int y, const char* s, uint16_t fg, uint8_t size = 1, int32_t bg = -1) {
  put(xr - textWidth(s, size), y, s, fg, size, bg);
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
  if (soundOn) tone(PIN_SPK, freq, ms);
}

// ---------- Rastreadores ----------
uint32_t presentFor(const Tracker& t) { return (t.last - t.first) / 1000; }
bool isActive(const Tracker& t, uint32_t now) { return t.used && now - t.last < ACTIVE_MS; }
bool canAlert(const Tracker& t) { return t.sep != S_NEAR_OWNER && !t.ignored; }

void ingest(const Adv& a, uint32_t now) {
  // Recuento general de dispositivos Bluetooth (ultimos 30 s)
  int slot = -1, oldest = 0;
  for (int i = 0; i < MAX_SEEN; i++) {
    if (seenAddr[i] == a.addr) { slot = i; break; }
    if (seenAt[i] < seenAt[oldest]) oldest = i;
  }
  if (slot < 0) { slot = oldest; seenAddr[slot] = a.addr; }
  seenAt[slot] = now;

  if (a.type == T_NONE) return;
  int free = -1;
  for (int i = 0; i < MAX_TRACKERS; i++) {
    Tracker& t = trackers[i];
    if (t.used && t.addr == a.addr) {
      t.rssi = a.rssi;
      t.rssiF = t.rssiF * 0.7f + a.rssi * 0.3f;
      t.last = now;
      t.sep = a.sep;
      return;
    }
    if (!t.used && free < 0) free = i;
  }
  if (free < 0) return;  // tabla llena: se descarta hasta que se olvide alguno
  Tracker& t = trackers[free];
  t = Tracker{a.addr, nextId++, a.type, a.sep, a.rssi, (float)a.rssi, now, now, false, true};
  if (nextId == 0) nextId = 1;
  Serial.printf("MSR_VIGIA_TRACKER id=%u type=\"%s\" sep=%u rssi=%d\n", t.id, TYPE_NAME[t.type], t.sep, t.rssi);
  beep(1800, 40);
}

void housekeeping(uint32_t now) {
  for (int i = 0; i < MAX_TRACKERS; i++) {
    Tracker& t = trackers[i];
    if (t.used && now - t.last > FORGET_MS) {
      Serial.printf("MSR_VIGIA_GONE id=%u minutes=%lu\n", t.id, (unsigned long)(presentFor(t) / 60));
      if (selected == i) selected = -1;
      t.used = false;
    }
  }
}

int countActive(uint32_t now, int type = -1) {
  int n = 0;
  for (auto& t : trackers)
    if (isActive(t, now) && (type < 0 || t.type == type)) n++;
  return n;
}

int countDevices(uint32_t now) {
  int n = 0;
  for (int i = 0; i < MAX_SEEN; i++)
    if (seenAddr[i] && now - seenAt[i] < 30000) n++;
  return n;
}

int worstTracker(uint32_t now) {  // el que mas tiempo lleva cerca y puede alertar
  int best = -1;
  for (int i = 0; i < MAX_TRACKERS; i++) {
    const Tracker& t = trackers[i];
    if (!isActive(t, now) || !canAlert(t)) continue;
    if (best < 0 || presentFor(t) > presentFor(trackers[best])) best = i;
  }
  return best;
}

const char* proximity(float rssi) {
  return rssi > -55 ? "muy cerca" : rssi > -70 ? "cerca" : rssi > -85 ? "a media distancia" : "lejos";
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
      lcd.drawFastHLine(sx[i] - 10, sy[i], 21, WARN);
      lcd.drawFastVLine(sx[i], sy[i] - 10, 21, WARN);
      lcd.drawCircle(sx[i], sy[i], 8, WARN);
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
      putCenter(160, 160, "Repite las tres cruces", ALERT);
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
    Serial.println("MSR_VIGIA_CALIBRATION_SAVED");
    return;
  }
}

// ---------- Marco comun ----------
void drawHeader() {
  lcd.fillRect(0, 0, W, 26, PANEL);
  uint16_t c = alarmOn ? ALERT : ACC;
  lcd.drawCircle(15, 13, 7, c);
  lcd.fillCircle(15, 13, 3, c);
  put(28, 9, "VIGÍA MS", TXT, 1, PANEL);
  put(84, 9, SCREEN_NAMES[screen], c, 1, PANEL);
  char s[24];
  snprintf(s, sizeof(s), "%d disp. BLE", countDevices(millis()));
  putRight(312, 9, s, DIM, 1, PANEL);
}

void drawFooter() {
  lcd.fillRect(0, 226, W, 14, BG);
  int x0 = 160 - (SCREENS * 14) / 2;
  for (int i = 0; i < SCREENS; i++) {
    if (i == screen) lcd.fillRoundRect(x0 + i * 14, 230, 10, 5, 2, alarmOn ? ALERT : ACC);
    else lcd.fillCircle(x0 + i * 14 + 5, 232, 2, LINE);
  }
}

void clearBody() { lcd.fillRect(0, 26, W, 200, BG); }

void rssiBars(int x, int y, float rssi, uint16_t c) {
  int bars = rssi > -55 ? 4 : rssi > -70 ? 3 : rssi > -85 ? 2 : 1;
  for (int i = 0; i < 4; i++) lcd.fillRect(x + i * 5, y + 9 - i * 3, 3, 3 + i * 3, i < bars ? c : LINE);
}

// ---------- Pantalla 1: VIGILANCIA ----------
void drawWatch(uint32_t now) {
  clearBody();
  int all = countActive(now);
  int n = 0;  // solo los que podrian estar siguiendote (no los que estan con su dueno ni los tuyos)
  for (auto& t : trackers)
    if (isActive(t, now) && canAlert(t)) n++;
  int w = worstTracker(now);
  uint16_t c = alarmOn ? ALERT : (n ? WARN : SAFE);
  lcd.fillRoundRect(10, 34, 300, 104, 12, PANEL);
  lcd.fillRoundRect(10, 34, 300, 8, 4, c);
  // Icono: escudo (todo bien) o exclamacion (atencion)
  int ix = 52, iy = 92;
  lcd.fillCircle(ix, iy, 26, c);
  if (alarmOn || n) {
    lcd.fillRoundRect(ix - 4, iy - 16, 8, 20, 3, INK);
    lcd.fillCircle(ix, iy + 11, 4, INK);
  } else {
    lcd.drawLine(ix - 11, iy, ix - 3, iy + 9, INK);
    lcd.drawLine(ix - 11, iy + 1, ix - 3, iy + 10, INK);
    lcd.drawLine(ix - 3, iy + 9, ix + 12, iy - 8, INK);
    lcd.drawLine(ix - 3, iy + 10, ix + 12, iy - 7, INK);
  }
  char line[48];
  if (alarmOn && w >= 0) {
    put(92, 58, "¡ATENCIÓN!", ALERT, 2, PANEL);
    snprintf(line, sizeof(line), "%s #%u", TYPE_NAME[trackers[w].type], trackers[w].id);
    put(92, 82, line, TXT, 1, PANEL);
    snprintf(line, sizeof(line), "lleva %lu min cerca de ti", (unsigned long)(presentFor(trackers[w]) / 60));
    put(92, 96, line, TXT, 1, PANEL);
    put(92, 116, "Desliza a BUSCAR para encontrarlo", DIM, 1, PANEL);
  } else if (n) {
    snprintf(line, sizeof(line), "%d sin su dueño", n);
    put(92, 58, line, WARN, 2, PANEL);
    put(92, 84, "Vigilando si alguno te acompaña", TXT, 1, PANEL);
    snprintf(line, sizeof(line), "Alarma si sigue %u min contigo", alertMin);
    put(92, 100, line, DIM, 1, PANEL);
  } else {
    put(92, 58, "Todo tranquilo", SAFE, 2, PANEL);
    if (all) {
      snprintf(line, sizeof(line), "%d rastreador%s con su dueño cerca", all, all == 1 ? "" : "es");
      put(92, 84, line, TXT, 1, PANEL);
      put(92, 100, "Es normal: no te están siguiendo", DIM, 1, PANEL);
    } else {
      put(92, 84, "Ningún rastreador cerca", TXT, 1, PANEL);
      put(92, 100, "Escuchando el Bluetooth...", DIM, 1, PANEL);
    }
  }
  // Recuento por tipo
  const uint8_t order[4] = {T_APPLE, T_SAMSUNG, T_TILE, T_GOOGLE};
  const char* const shortName[4] = {"Apple", "Samsung", "Tile", "Google"};
  for (int i = 0; i < 4; i++) {
    int x = 10 + i * 76;
    lcd.fillRoundRect(x, 148, 72, 52, 8, PANEL);
    lcd.fillRoundRect(x + 8, 154, 56, 3, 1, TYPE_COLOR[order[i]]);
    snprintf(line, sizeof(line), "%d", countActive(now, order[i]));
    putCenter(x + 36, 164, line, TXT, 2, PANEL);
    putCenter(x + 36, 186, shortName[i], DIM, 1, PANEL);
  }
  put(10, 208, "Solo escucha: no se conecta ni envía nada", DIM);
}

// ---------- Pantalla 2: RADAR ----------
constexpr int RCX = 110, RCY = 126, RCR = 92;

void drawRadarFrame() {
  clearBody();
  for (int r = RCR; r > 0; r -= RCR / 3) lcd.drawCircle(RCX, RCY, r, LINE);
  lcd.drawFastHLine(RCX - RCR, RCY, RCR * 2, LINE);
  lcd.drawFastVLine(RCX, RCY - RCR, RCR * 2, LINE);
  lcd.fillCircle(RCX, RCY, 4, ACC);
  put(212, 40, "Centro: tú", DIM);
  put(212, 54, "Distancia según", DIM);
  put(212, 66, "la señal (RSSI);", DIM);
  put(212, 78, "el ángulo es", DIM);
  put(212, 90, "decorativo.", DIM);
  const uint8_t order[4] = {T_APPLE, T_SAMSUNG, T_TILE, T_GOOGLE};
  for (int i = 0; i < 4; i++) {
    lcd.fillCircle(218, 120 + i * 18, 4, TYPE_COLOR[order[i]]);
    const char* const legend[4] = {"Apple", "Samsung", "Tile", "Google"};
    put(228, 116 + i * 18, legend[i], TXT);
  }
}

void drawRadarDots(uint32_t now) {
  // Barrido: borra y redibuja solo el interior
  lcd.fillCircle(RCX, RCY, RCR - 1, BG);
  for (int r = RCR; r > 0; r -= RCR / 3) lcd.drawCircle(RCX, RCY, r, LINE);
  lcd.drawFastHLine(RCX - RCR, RCY, RCR * 2, LINE);
  lcd.drawFastVLine(RCX, RCY - RCR, RCR * 2, LINE);
  float a = (sweep % 360) * DEG_TO_RAD;
  lcd.drawLine(RCX, RCY, RCX + cosf(a) * (RCR - 2), RCY + sinf(a) * (RCR - 2), SAFE);
  lcd.fillCircle(RCX, RCY, 4, ACC);
  for (auto& t : trackers) {
    if (!isActive(t, now)) continue;
    float d = constrain((-(t.rssiF) - 40) / 60.0f, 0.08f, 1.0f) * (RCR - 8);
    float ang = (t.addr % 360) * DEG_TO_RAD;  // angulo fijo por rastreador, decorativo
    int x = RCX + cosf(ang) * d, y = RCY + sinf(ang) * d;
    uint16_t c = (alarmOn && canAlert(t) && presentFor(t) >= alertMin * 60u) ? ALERT : TYPE_COLOR[t.type];
    lcd.fillCircle(x, y, 6, c);
    char s[6];
    snprintf(s, sizeof(s), "%u", t.id);
    put(x + 8, y - 3, s, TXT);
  }
}

// ---------- Pantalla 3: LISTA ----------
constexpr int ROW_Y = 32, ROW_H = 31;
int rowIndex[6];

void drawList(uint32_t now) {
  clearBody();
  int rows = 0;
  for (int i = 0; i < MAX_TRACKERS && rows < 6; i++) {
    const Tracker& t = trackers[i];
    if (!isActive(t, now)) continue;
    int y = ROW_Y + rows * ROW_H;
    rowIndex[rows++] = i;
    lcd.fillRoundRect(6, y, 308, ROW_H - 3, 6, PANEL);
    lcd.fillRoundRect(6, y, 6, ROW_H - 3, 3, TYPE_COLOR[t.type]);
    char s[48];
    snprintf(s, sizeof(s), "#%u %s", t.id, TYPE_NAME[t.type]);
    put(18, y + 5, s, TXT, 1, PANEL);
    const char* st = t.ignored ? "marcado como tuyo"
                   : t.sep == S_NEAR_OWNER ? "con su dueño cerca"
                   : t.sep == S_SEPARATED ? "LEJOS de su dueño" : "estado desconocido";
    uint16_t sc = t.ignored ? DIM : t.sep == S_SEPARATED ? ALERT : t.sep == S_NEAR_OWNER ? SAFE : WARN;
    put(18, y + 16, st, sc, 1, PANEL);
    snprintf(s, sizeof(s), "%lu min", (unsigned long)(presentFor(t) / 60));
    putRight(270, y + 5, s, TXT, 1, PANEL);
    putRight(270, y + 16, proximity(t.rssiF), DIM, 1, PANEL);
    rssiBars(282, y + 7, t.rssiF, TYPE_COLOR[t.type]);
  }
  for (int r = rows; r < 6; r++) rowIndex[r] = -1;
  if (!rows) {
    putCenter(160, 100, "Ningún rastreador cerca", TXT, 2);
    putCenter(160, 130, "La lista se rellena sola al detectarlos", DIM);
  } else {
    put(8, 216, "Toca uno para buscarlo", DIM);
  }
}

// ---------- Pantalla 4: BUSCAR ----------
const Btn B_MINE = {170, 186, 142, 30};

bool inside(const Btn& b, int x, int y) { return x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h; }

// Parte que cambia con la senal: se redibuja sin borrar el resto de la pantalla.
void drawFindLive() {
  if (selected < 0) return;
  const Tracker& t = trackers[selected];
  int pct = constrain((int)((t.rssiF + 100) * 100 / 60), 0, 100);
  uint16_t c = pct > 70 ? ALERT : pct > 40 ? WARN : SAFE;
  const char* hint = pct > 70 ? "¡CALIENTE!" : pct > 40 ? "TEMPLADO" : "FRÍO";
  lcd.fillRect(0, 52, W, 38, BG);
  putCenter(160, 56, hint, c, 4);
  lcd.fillRoundRect(14, 102, 292, 22, 5, BG);
  lcd.fillRoundRect(14, 102, max(6, 292 * pct / 100), 22, 5, c);
  char s[48];
  snprintf(s, sizeof(s), "Señal %d dBm · %s", (int)t.rssiF, proximity(t.rssiF));
  lcd.fillRect(0, 134, W, 12, BG);
  putCenter(160, 136, s, TXT);
}

// Resumen de lo que se ve en pantalla: solo se redibuja si cambia.
uint32_t viewSignature(uint32_t now) {
  uint32_t h = 2166136261u ^ (alarmOn ? 7 : 0) ^ ((uint32_t)screen << 24);
  auto mix = [&](uint32_t v) { h = (h ^ v) * 16777619u; };
  mix(countDevices(now));
  for (auto& t : trackers) {
    if (!isActive(t, now)) continue;
    mix(t.id); mix(t.sep); mix(t.ignored); mix(presentFor(t) / 60);
    mix(t.rssiF > -55 ? 4 : t.rssiF > -70 ? 3 : t.rssiF > -85 ? 2 : 1);
  }
  if (screen == 4) mix(advCount / 50);
  return h;
}

void drawFind(uint32_t now) {
  clearBody();
  if (selected < 0 || !isActive(trackers[selected], now)) {
    int w = worstTracker(now);
    if (w < 0) for (int i = 0; i < MAX_TRACKERS; i++) if (isActive(trackers[i], now)) { w = i; break; }
    selected = w;
  }
  if (selected < 0) {
    putCenter(160, 90, "Nada que buscar", TXT, 2);
    putCenter(160, 120, "Cuando aparezca un rastreador, acércate", DIM);
    putCenter(160, 134, "y esta pantalla te guiará con pitidos", DIM);
    return;
  }
  const Tracker& t = trackers[selected];
  char s[48];
  snprintf(s, sizeof(s), "#%u %s", t.id, TYPE_NAME[t.type]);
  put(12, 34, s, TYPE_COLOR[t.type], 1);
  lcd.drawRoundRect(12, 100, 296, 26, 6, LINE);
  drawFindLive();
  put(12, 158, "Muévete despacio: cuanto más rápido", DIM);
  put(12, 170, "pita, más cerca está. Revisa bolsos,", DIM);
  put(12, 182, "bolsillos, coche y forros.", DIM);
  lcd.fillRoundRect(B_MINE.x, B_MINE.y, B_MINE.w, B_MINE.h, 6, t.ignored ? SAFE : PANEL);
  lcd.drawRoundRect(B_MINE.x, B_MINE.y, B_MINE.w, B_MINE.h, 6, t.ignored ? SAFE : LINE);
  putCenter(B_MINE.x + B_MINE.w / 2, B_MINE.y + 11, t.ignored ? "Es mío: sin alarma" : "Es mío, no avisar",
            t.ignored ? INK : TXT, 1, t.ignored ? SAFE : PANEL);
}

// ---------- Pantalla 5: AJUSTES ----------
const Btn B_TIME = {8, 112, 148, 30}, B_SOUND = {164, 112, 148, 30};
const Btn B_BRIGHT = {8, 150, 148, 30}, B_CAL = {164, 150, 148, 30};

void flatButton(const Btn& b, const char* label, bool on) {
  uint16_t face = on ? SAFE : PANEL;
  lcd.fillRoundRect(b.x, b.y, b.w, b.h, 6, face);
  lcd.drawRoundRect(b.x, b.y, b.w, b.h, 6, on ? SAFE : LINE);
  putCenter(b.x + b.w / 2, b.y + b.h / 2 - 4, label, on ? INK : TXT, 1, face);
}

void drawSettings(uint32_t now) {
  clearBody();
  lcd.fillRoundRect(8, 32, 304, 74, 8, PANEL);
  char s[56];
  snprintf(s, sizeof(s), "Anuncios oídos: %lu", (unsigned long)advCount);
  put(16, 40, s, TXT, 1, PANEL);
  snprintf(s, sizeof(s), "Dispositivos BLE (30 s): %d · rastreadores: %d", countDevices(now), countActive(now));
  put(16, 54, s, DIM, 1, PANEL);
  put(16, 68, "Escucha pasiva: no conecta ni envía nada.", DIM, 1, PANEL);
  put(16, 82, "No sustituye a las alertas del móvil.", DIM, 1, PANEL);
  snprintf(s, sizeof(s), "Alarma tras %u min", alertMin);
  flatButton(B_TIME, s, false);
  snprintf(s, sizeof(s), "Sonido: %s", soundOn ? "SÍ" : "NO");
  flatButton(B_SOUND, s, soundOn);
  snprintf(s, sizeof(s), "Brillo auto: %s", autoBright ? "SÍ" : "NO");
  flatButton(B_BRIGHT, s, autoBright);
  flatButton(B_CAL, "Recalibrar táctil", false);
  putCenter(160, 190, "Toca «Alarma» para cambiar los minutos", DIM);
  putCenter(160, 204, "Si te sientes en peligro, llama al 112", WARN);
}

// ---------- Dibujo general ----------
void drawScreen(uint32_t now) {
  drawHeader();
  switch (screen) {
    case 0: drawWatch(now); break;
    case 1: drawRadarFrame(); drawRadarDots(now); break;
    case 2: drawList(now); break;
    case 3: drawFind(now); break;
    case 4: drawSettings(now); break;
  }
  drawFooter();
}

void slideTo(int next, int dir) {
  for (int i = 0; i < 8; i++) {
    int x = dir > 0 ? W - (i + 1) * 40 : i * 40;
    lcd.fillRect(x, 26, 40, 200, PANEL);
    delay(7);
  }
  screen = (next + SCREENS) % SCREENS;
  drawScreen(millis());
  Serial.printf("MSR_VIGIA_SCREEN %s\n", SCREEN_NAMES[screen]);
}

// ---------- Gestos ----------
bool touching = false;
int downX = 0, downY = 0, lastX = 0, lastY = 0;
uint32_t upAt = 0;

void onTap(int x, int y) {
  uint32_t now = millis();
  if (screen == 2) {
    int r = (y - ROW_Y) / ROW_H;
    if (y >= ROW_Y && r >= 0 && r < 6 && rowIndex[r] >= 0) {
      selected = rowIndex[r];
      slideTo(3, 1);
    }
  } else if (screen == 3 && selected >= 0 && inside(B_MINE, x, y)) {
    trackers[selected].ignored = !trackers[selected].ignored;
    Serial.printf("MSR_VIGIA_IGNORED id=%u value=%d\n", trackers[selected].id, trackers[selected].ignored);
    drawScreen(now);
  } else if (screen == 4) {
    if (inside(B_TIME, x, y)) {
      int i = 0;
      while (i < 4 && ALERT_OPTIONS[i] != alertMin) i++;
      alertMin = ALERT_OPTIONS[(i + 1) % 4];
      prefs.putUChar("min", alertMin);
    } else if (inside(B_SOUND, x, y)) { soundOn = !soundOn; prefs.putBool("sound", soundOn); beep(2200, 50); }
    else if (inside(B_BRIGHT, x, y)) { autoBright = !autoBright; prefs.putBool("auto", autoBright); updateBrightness(); }
    else if (inside(B_CAL, x, y)) { calibrate(); }
    else return;
    drawScreen(now);
  }
}

void handleTouch() {
  int rx, ry;
  uint32_t t = millis();
  if (sampleRaw(rx, ry)) {
    int x, y;
    toScreen(rx, ry, x, y);
    if (!touching) { touching = true; downX = x; downY = y; }
    lastX = x; lastY = y; upAt = t;
    return;
  }
  if (touching && t - upAt > 60) {
    touching = false;
    int dx = lastX - downX, dy = lastY - downY;
    if (abs(dx) > 50 && abs(dx) > abs(dy)) slideTo(screen + (dx < 0 ? 1 : -1), dx < 0 ? 1 : -1);
    else if (abs(dx) < 15 && abs(dy) < 15) onTap(downX, downY);
  }
}

// ---------- Programa ----------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("MSR_VIGIA_BOOT profile=ESP32-2432S028R display=ILI9341 touch=XPT2046 scan=BLE-passive");
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

  prefs.begin("vigia", false);
  alertMin = prefs.getUChar("min", 10);
  soundOn = prefs.getBool("sound", true);
  autoBright = prefs.getBool("auto", true);
  if (prefs.getBool("cal", false)) {
    ax = prefs.getFloat("ax"); bx = prefs.getFloat("bx"); cx = prefs.getFloat("cx");
    ay = prefs.getFloat("ay"); by = prefs.getFloat("by"); cy = prefs.getFloat("cy");
  } else {
    calibrate();
  }
  updateBrightness();
  startScan();
  drawScreen(millis());
}

void loop() {
  uint32_t now = millis();
  Adv a;
  int budget = 40;
  while (budget-- && xQueueReceive(advQueue, &a, 0) == pdTRUE) ingest(a, now);
  handleTouch();

  // Alarma
  int w = worstTracker(now);
  alarmOn = w >= 0 && presentFor(trackers[w]) >= alertMin * 60u;
  if (alarmOn != lastAlarmOn) {
    lastAlarmOn = alarmOn;
    if (alarmOn) Serial.printf("MSR_VIGIA_ALARM id=%u type=\"%s\" minutes=%lu\n", trackers[w].id,
                             TYPE_NAME[trackers[w].type], (unsigned long)(presentFor(trackers[w]) / 60));
    drawScreen(now);
  }
  if (alarmOn) {
    bool on = (now / 300) % 2;
    setLed(on, false, false);
    if (screen != 3 && now >= nextBeep) { beep(2600, 120); nextBeep = now + 4000; }
  } else {
    int n = countActive(now);
    bool pulse = (now % 2000) < 80;
    setLed(false, pulse && !n, pulse && n);  // latido verde: tranquilo; azul: hay rastreadores
  }

  // Pitidos de "frio / caliente" en BUSCAR
  if (screen == 3 && selected >= 0 && isActive(trackers[selected], now) && now >= nextBeep) {
    float r = trackers[selected].rssiF;
    int gap = constrain((int)map((long)r, -100, -40, 1400, 90), 90, 1400);
    beep(1900, 18);
    nextBeep = now + gap;
  }

  // Refresco de pantalla
  uint32_t period = screen == 1 ? 120 : 400;
  if (!touching && now - lastDraw > period) {
    lastDraw = now;
    housekeeping(now);
    static uint32_t lastSig = 0;
    uint32_t sig = viewSignature(now);
    if (screen == 1) { sweep += 12; drawRadarDots(now); if (sig != lastSig) drawHeader(); }
    else if (screen == 3 && selected >= 0 && isActive(trackers[selected], now)) drawFindLive();
    if (screen != 1 && sig != lastSig && !(screen == 3 && selected >= 0)) drawScreen(now);
    lastSig = sig;
  }
  if (now - lastBright > 1500) { lastBright = now; updateBrightness(); }
  delay(4);
}
