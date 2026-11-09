/*
  CazaCámaras MS · AB MakerIDE / MS Robotika
  Detector pasivo de posibles cámaras para la CYD ESP32-2432S028R (ILI9341 320x240
  en horizontal, táctil XPT2046). Escucha WiFi y Bluetooth sin conectarse a nada:
    - Fabricantes de cámaras y chips IoT, mediante OUI reales de IEEE
    - Subida sostenida compatible con vídeo, sin leer el contenido
    - Redes de cámara y nombres Bluetooth, sin mostrar sus nombres reales
  BUSCAR ayuda con una barra de señal y pitidos de frío / caliente.

  Seis pantallas al deslizar: ESCANEO, RADAR, LISTA, BUSCAR, CONSEJOS y AJUSTES.
  Límites: no ve cámaras que solo graban en SD, cámaras por cable ni WiFi de 5 GHz.
  Una cámara vecina también aparece. Una señal o una marca no prueban que haya
  una cámara oculta. Revisa la habitación y pide ayuda si encuentras una.

  Librerías: Adafruit GFX, Adafruit ILI9341, XPT2046_Touchscreen (Paul Stoffregen),
  NimBLE-Arduino 2.x (h2zero). Placa: "ESP32 Dev Module" (esp32 3.x), huge_app.
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
#include <WiFi.h>
#include <esp_wifi.h>
#include "Reglas.h"
#include "TablaOui.h"

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

struct Btn { int x, y, w, h; };
struct Frame {
  uint32_t at;
  uint8_t mac[6];
  uint16_t len;
  int8_t rssi;
  uint8_t channel, fc0, fc1;
};
struct Adv { uint64_t addr; uint32_t at; int8_t rssi; Confidence hint; };
struct Target {
  uint64_t addr = 0;
  uint32_t id = 0, first = 0, last = 0, presentSince = 0, netAt = 0;
  const OuiEntry* oui = nullptr;
  int8_t pattern = -1, rssi = -100, best = -127;
  float rssiF = -100;
  uint8_t channel = 0;
  Confidence confidence = NINGUNA;
  bool used = false, ignored = false, announced = false;
  TrafficWindow traffic;
  TrafficRun run;
  uint32_t visitFrames = 0, visitData = 0, visitUp = 0;
};

constexpr int MAX_WIFI = 64, MAX_BLE = 16, MAX_TARGETS = MAX_WIFI + MAX_BLE;
constexpr int MAX_SEEN = 96, RING_SIZE = 256;
constexpr uint32_t ACTIVE_MS = 60000, FORGET_MS = 180000, HOP_MS = 250;
Target targets[MAX_TARGETS];
uint32_t nextId = 1;
uint64_t seenAddr[MAX_SEEN] = {};
uint32_t seenAt[MAX_SEEN] = {};
bool seenUsed[MAX_SEEN] = {};
Frame frameRing[RING_SIZE];
portMUX_TYPE ringMux = portMUX_INITIALIZER_UNLOCKED;
uint16_t ringHead = 0, ringTail = 0;
uint32_t wifiDrops = 0, bleDrops = 0;
bool accepting = false;
QueueHandle_t advQueue = nullptr;

Preferences prefs;
int screen = 0, selected = -1;
constexpr int SCREENS = 6;
const char* const SCREEN_NAMES[SCREENS] = {"ESCANEO", "RADAR", "LISTA", "BUSCAR", "CONSEJOS", "AJUSTES"};
bool soundOn = true, autoBright = true, alarmOn = false;
bool wifiOK = false, bleOK = false, sniffing = false, scanning = false, scanFailed = false;
bool tableFull = false, paused = false, prefsOK = false;
uint8_t channel = 1;
uint32_t dwellAt = 0, lastScan = 0, scanAt = 0, lastBleTry = 0;
uint32_t lastDraw = 0, lastBright = 0, nextBeep = 0, sweep = 0;
float ax = 0, bx = 0, cx = 0, ay = 0, by = 0, cy = 0;
int rowIndex[6] = {-1, -1, -1, -1, -1, -1};
uint32_t rowId[6] = {};
bool touching = false;
int downX = 0, downY = 0, lastX = 0, lastY = 0;
uint32_t upAt = 0;
constexpr int ROW_Y = 32, ROW_H = 30;
const Btn B_MINE = {64, 187, 192, 30};
const Btn B_SOUND = {8, 112, 148, 30}, B_BRIGHT = {164, 112, 148, 30}, B_CAL = {8, 150, 304, 30};

// El núcleo 3.x debe conservar la memoria de Bluetooth para NimBLE-Arduino.
extern "C" bool btInUse(void) { return true; }

// Los ayudantes de texto, brillo, sonido y táctil de Vigía van a continuación.
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
    if (prefsOK) {
      prefs.putFloat("ax", ax); prefs.putFloat("bx", bx); prefs.putFloat("cx", cx);
      prefs.putFloat("ay", ay); prefs.putFloat("by", by); prefs.putFloat("cy", cy);
      prefs.putBool("cal", true);
    }
    return;
  }
}



// ---------- Recepción: sin dibujar, imprimir ni guardar paquetes ----------
void wifiRx(void* buffer, wifi_promiscuous_pkt_type_t type) {
  if (!buffer || (type != WIFI_PKT_MGMT && type != WIFI_PKT_DATA)) return;
  const auto* p = static_cast<const wifi_promiscuous_pkt_t*>(buffer);
  if (p->rx_ctrl.rx_state != 0 || p->rx_ctrl.sig_len < 28) return;
  const uint8_t* h = p->payload;
  if ((h[0] & 3) || (h[10] & 1)) return;
  Frame f;
  f.at = millis();
  memcpy(f.mac, h + 10, 6);  // addr2: transmisor, no destinatario
  f.len = p->rx_ctrl.sig_len - 4;  // sin FCS
  f.rssi = p->rx_ctrl.rssi;
  f.channel = p->rx_ctrl.channel;
  f.fc0 = h[0]; f.fc1 = h[1];
  portENTER_CRITICAL(&ringMux);
  if (accepting) {
    uint16_t next = (ringHead + 1) % RING_SIZE;
    if (next == ringTail) ++wifiDrops;
    else { frameRing[ringHead] = f; ringHead = next; }
  }
  portEXIT_CRITICAL(&ringMux);
}

class ScanEvents : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* d) override {
    Adv a;
    // Tipo de dirección incluido para no mezclar direcciones públicas y aleatorias.
    a.addr = (uint64_t)d->getAddress() | ((uint64_t)d->getAddressType() << 48);
    a.at = millis(); a.rssi = d->getRSSI();
    a.hint = d->haveName() ? bleHint(d->getName().c_str()) : NINGUNA;
    if (xQueueSend(advQueue, &a, 0) != pdTRUE) {
      portENTER_CRITICAL(&ringMux); ++bleDrops; portEXIT_CRITICAL(&ringMux);
    }
  }
};
ScanEvents scanEvents;

uint64_t macKey(const uint8_t* mac) {
  uint64_t key = 0;
  for (int i = 0; i < 6; ++i) key = (key << 8) | mac[i];
  return key;
}
bool isActive(const Target& t, uint32_t now) { return t.used && now - t.last < ACTIVE_MS; }
bool hasNetwork(const Target& t, uint32_t now) { return t.pattern >= 0 && now - t.netAt < ACTIVE_MS; }
bool hasVideo(const Target& t, uint32_t now) { return t.run.recent(now); }
bool candidate(int index, uint32_t now) {
  const Target& t = targets[index];
  return isActive(t, now) && (index >= MAX_WIFI || hasNetwork(t, now) ||
         ouiSuspect(t.confidence, hasVideo(t, now), t.last - t.presentSince));
}
bool suspect(int index, uint32_t now) { return candidate(index, now) && !targets[index].ignored; }
Confidence confidenceOf(const Target& t, uint32_t now) {
  Confidence c = t.confidence;
  if (hasNetwork(t, now) && c < MEDIA) c = MEDIA;
  if (hasVideo(t, now) && c < BAJA) c = BAJA;
  return c;
}
bool probable(int index, uint32_t now) {
  return suspect(index, now) && (targets[index].confidence == ALTA || hasVideo(targets[index], now));
}
const char* vendorOf(const Target& t) { return t.oui ? t.oui->vendor : "Chip genérico"; }
void targetLabel(int index, uint32_t now, char* out, size_t size) {
  const Target& t = targets[index];
  if (index >= MAX_WIFI) snprintf(out, size, "Pista Bluetooth");
  else if (hasNetwork(t, now)) snprintf(out, size, "Red de cámara tipo %s", NET_PATTERNS[t.pattern]);
  else snprintf(out, size, "%s", t.confidence <= BAJA ? "Chip genérico" : vendorOf(t));
}
const char* proximity(float rssi) {
  return rssi > -55 ? "muy cerca" : rssi > -70 ? "cerca" : rssi > -85 ? "a media distancia" : "lejos";
}
int countActive(uint32_t now) {
  int n = 0;
  for (int i = 0; i < MAX_TARGETS; ++i) if (suspect(i, now)) ++n;
  return n;
}
int countWifi(uint32_t now) {
  int n = 0;
  for (int i = 0; i < MAX_WIFI; ++i) if (isActive(targets[i], now)) ++n;
  return n;
}
int countNetworks(uint32_t now) {
  int n = 0;
  for (int i = 0; i < MAX_WIFI; ++i) if (targets[i].used && hasNetwork(targets[i], now)) ++n;
  return n;
}
int countDevices(uint32_t now) {
  int n = 0;
  for (int i = 0; i < MAX_SEEN; ++i) if (seenUsed[i] && now - seenAt[i] < 30000) ++n;
  return n;
}
int priorityOf(int index, uint32_t now) {
  return (probable(index, now) ? 1000 : 0) + (int)confidenceOf(targets[index], now) * 100
         + (int)targets[index].rssiF + 120 - (targets[index].ignored ? 2000 : 0);
}
int worstTarget(uint32_t now) {
  int best = -1;
  for (int i = 0; i < MAX_TARGETS; ++i)
    if (suspect(i, now) && (best < 0 || priorityOf(i, now) > priorityOf(best, now))) best = i;
  return best;
}
void reportNew(uint32_t now) {
  for (int i = 0; i < MAX_TARGETS; ++i) {
    Target& t = targets[i];
    if (!candidate(i, now) || t.announced) continue;
    if (!t.id) t.id = nextId++;  // solo se numeran las pistas, no todo el vecindario
    t.announced = true;
    // Serie: solo fabricante, RSSI y confianza; nunca nombres ni direcciones.
    Serial.printf("%s | %d dBm | %s\n", i >= MAX_WIFI ? "Bluetooth" : vendorOf(t),
                  t.rssi, CONF_NAME[confidenceOf(t, now)]);
    if (!t.ignored && screen != 3) beep(1800, 40);
  }
}

int findSlot(uint64_t addr, bool ble, uint32_t now) {
  int start = ble ? MAX_WIFI : 0, end = ble ? MAX_TARGETS : MAX_WIFI;
  int freeSlot = -1, victim = -1;
  for (int i = start; i < end; ++i) {
    Target& t = targets[i];
    if (t.used && t.addr == addr) return i;
    if (freeSlot < 0 && (!t.used || (now - t.last > FORGET_MS && !t.ignored && i != selected))) freeSlot = i;
    // Con muchos vecinos la tabla se llena: se sacrifica el aparato normal visto hace más tiempo.
    if (t.used && !t.ignored && i != selected && !candidate(i, now) && (victim < 0 || now - t.last > now - targets[victim].last)) victim = i;
  }
  if (freeSlot < 0) freeSlot = victim;
  if (freeSlot < 0) { tableFull = true; return -1; }
  Target& t = targets[freeSlot];
  t = Target{};
  t.used = true; t.addr = addr;
  t.first = t.last = t.presentSince = now;
  return freeSlot;
}
void updateSignal(Target& t, int8_t rssi, uint32_t at) {
  if (at - t.last > 6000) t.presentSince = at;
  t.rssiF = t.best == -127 ? rssi : t.rssiF * 0.7f + rssi * 0.3f;
  t.rssi = rssi;
  if (rssi > t.best) t.best = rssi;
  t.last = at;
}
void ingest(const Frame& a) {
  if (a.channel != channel || millis() - a.at > 1500) return;
  uint64_t key = macKey(a.mac);
  if (!key) return;
  int slot = findSlot(key, false, a.at);
  if (slot < 0) return;
  Target& t = targets[slot];
  if (t.channel && t.channel != a.channel) t.run.reset();
  t.channel = a.channel;
  t.oui = lookupOui(a.mac);
  t.confidence = t.oui ? t.oui->confidence : NINGUNA;
  // No perder la confianza que dio el nombre de su red de configuración.
  if (hasNetwork(t, a.at) && NET_CONF[t.pattern] > t.confidence) t.confidence = NET_CONF[t.pattern];
  updateSignal(t, a.rssi, a.at);
  t.traffic.add(a.at, a.len);
  ++t.visitFrames;
  if (payloadData(a.fc0)) ++t.visitData;
  if (uplinkData(a.fc0, a.fc1)) t.visitUp += a.len;
}
void drainWifi() {
  for (int n = 0; n < RING_SIZE; ++n) {
    Frame a;
    portENTER_CRITICAL(&ringMux);
    bool available = ringTail != ringHead;
    if (available) { a = frameRing[ringTail]; ringTail = (ringTail + 1) % RING_SIZE; }
    portEXIT_CRITICAL(&ringMux);
    if (!available) break;
    ingest(a);
  }
}
void ingestBle(const Adv& a) {
  int seen = -1, oldest = 0;
  for (int i = 0; i < MAX_SEEN; ++i) {
    if (seenUsed[i] && seenAddr[i] == a.addr) { seen = i; break; }
    if (!seenUsed[i] || a.at - seenAt[i] > a.at - seenAt[oldest]) oldest = i;
  }
  if (seen < 0) seen = oldest;
  seenUsed[seen] = true; seenAddr[seen] = a.addr; seenAt[seen] = a.at;
  if (a.hint == NINGUNA) return;
  int slot = findSlot(a.addr, true, a.at);
  if (slot < 0) return;
  Target& t = targets[slot];
  t.confidence = a.hint;
  updateSignal(t, a.rssi, a.at);
}
void resetTraffic() {
  for (int i = 0; i < MAX_WIFI; ++i) {
    Target& t = targets[i];
    t.run.reset(); t.visitFrames = t.visitData = t.visitUp = 0;
  }
}
void finishDwell(uint32_t now) {
  uint32_t elapsed = now - dwellAt;
  for (int i = 0; i < MAX_WIFI; ++i) {
    Target& t = targets[i];
    if (t.used && t.channel == channel)
      t.run.sample(now, t.visitUp, t.visitFrames, t.visitData, elapsed);
    t.visitFrames = t.visitData = t.visitUp = 0;
  }
}

// ---------- Radios: ningún camino llama a connect, begin o transmisión ----------
void stopSniffer() {
  portENTER_CRITICAL(&ringMux); accepting = false; portEXIT_CRITICAL(&ringMux);
  if (sniffing && esp_wifi_set_promiscuous(false) != ESP_OK) wifiOK = false;
  sniffing = false;
  drainWifi();
}
void resumeSniffer() {
  if (!wifiOK || paused || scanning) return;
  if (esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE) != ESP_OK) { wifiOK = false; return; }
  dwellAt = millis();
  portENTER_CRITICAL(&ringMux); accepting = true; portEXIT_CRITICAL(&ringMux);
  sniffing = esp_wifi_set_promiscuous(true) == ESP_OK;
  if (!sniffing) {
    portENTER_CRITICAL(&ringMux); accepting = false; portEXIT_CRITICAL(&ringMux);
    wifiOK = false;
  }
}
void ingestNetworks(int n, uint32_t now) {
  for (int i = 0; i < n; ++i) {
    // SSID solo temporal para clasificar; no se copia a ninguna tabla ni salida.
    int pattern = cameraPattern(WiFi.SSID(i).c_str());
    if (pattern < 0) continue;
    const uint8_t* mac = WiFi.BSSID(i);
    if (!mac) continue;
    int slot = findSlot(macKey(mac), false, now);
    if (slot < 0) continue;
    Target& t = targets[slot];
    t.oui = lookupOui(mac);
    t.confidence = t.oui ? t.oui->confidence : NINGUNA;
    if (NET_CONF[pattern] > t.confidence) t.confidence = NET_CONF[pattern];
    t.pattern = pattern; t.netAt = now;
    uint8_t ch = WiFi.channel(i);
    if (t.channel && t.channel != ch) t.run.reset();
    t.channel = ch;
    updateSignal(t, WiFi.RSSI(i), now);
  }
}
void cancelNetworkScan() {
  if (!scanning) return;
  esp_wifi_scan_stop();
  WiFi.scanDelete();
  scanning = false;
  lastScan = millis();
  resetTraffic();
}
void startRadios() {
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  wifiOK = WiFi.mode(WIFI_STA);
  if (wifiOK) {
    wifi_config_t empty = {};
    wifi_country_t country = {};
    memcpy(country.cc, "ES", 3);
    country.schan = 1; country.nchan = 13; country.policy = WIFI_COUNTRY_POLICY_MANUAL;
    wifi_promiscuous_filter_t filter = {};
    filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA;
    wifiOK = esp_wifi_set_storage(WIFI_STORAGE_RAM) == ESP_OK &&
             esp_wifi_set_config(WIFI_IF_STA, &empty) == ESP_OK &&
             esp_wifi_set_country(&country) == ESP_OK &&
             esp_wifi_set_ps(WIFI_PS_NONE) == ESP_OK &&
             esp_wifi_set_promiscuous_filter(&filter) == ESP_OK &&
             esp_wifi_set_promiscuous_rx_cb(wifiRx) == ESP_OK;
  }
  resumeSniffer();
  lastScan = millis() - 17000;  // primer barrido de redes tras la primera vuelta
  advQueue = xQueueCreate(64, sizeof(Adv));
  if (advQueue && NimBLEDevice::init("")) {
    NimBLEScan* scan = NimBLEDevice::getScan();
    scan->setScanCallbacks(&scanEvents, true);
    scan->setActiveScan(false);
    scan->setDuplicateFilter(0);
    scan->setInterval(100);
    scan->setWindow(45);  // deja tiempo a WiFi en la radio compartida
    scan->setMaxResults(0);
    bleOK = scan->start(0, false, true);
  }
}
void pauseRadios() {
  paused = true;
  cancelNetworkScan(); stopSniffer(); resetTraffic();
  if (NimBLEDevice::isInitialized()) NimBLEDevice::getScan()->stop();
  if (advQueue) xQueueReset(advQueue);
}
void radioTick(uint32_t now) {
  if (paused) return;
  static uint32_t lastDrops = 0;
  uint32_t drops;
  portENTER_CRITICAL(&ringMux); drops = wifiDrops; portEXIT_CRITICAL(&ringMux);
  if (drops != lastDrops) { resetTraffic(); lastDrops = drops; }
  if (NimBLEDevice::isInitialized()) {
    bleOK = NimBLEDevice::getScan()->isScanning();
    if (!bleOK && now - lastBleTry > 2000) {
      lastBleTry = now;
      bleOK = NimBLEDevice::getScan()->start(0, false, true);
    }
  }
  if (!wifiOK) return;
  bool findMode = screen == 3;
  uint8_t wanted = findMode && selected >= 0 && selected < MAX_WIFI ? targets[selected].channel : 0;
  if (findMode && scanning) { cancelNetworkScan(); resumeSniffer(); }
  if (scanning) {
    int n = WiFi.scanComplete();
    if (n != WIFI_SCAN_RUNNING) {
      scanFailed = n < 0;
      if (n >= 0) ingestNetworks(n, now);
      WiFi.scanDelete(); scanning = false;
      resumeSniffer();
    } else if (now - scanAt > 6000) {
      cancelNetworkScan(); scanFailed = true; resumeSniffer();
    }
    return;
  }
  if (!findMode && now - lastScan >= 20000) {
    stopSniffer(); resetTraffic();
    if (!wifiOK) return;
    lastScan = scanAt = millis(); scanning = true;
    // Asíncrono, redes ocultas incluidas, PASIVO, 120 ms por canal, todos los canales.
    int n = WiFi.scanNetworks(true, true, true, 120, 0);
    if (n != WIFI_SCAN_RUNNING) {
      scanFailed = n < 0;
      if (n >= 0) ingestNetworks(n, millis());
      WiFi.scanDelete(); scanning = false; resumeSniffer();
    }
    return;
  }
  if (wanted && wanted != channel) {
    stopSniffer(); resetTraffic(); channel = wanted; resumeSniffer();
  } else if (sniffing && now - dwellAt >= HOP_MS) {
    stopSniffer();
    finishDwell(millis());
    if (!wanted) channel = channel % 13 + 1;
    resumeSniffer();
  }
}

// ---------- Marco común de Vigía ----------
void drawHeader() {
  lcd.fillRect(0, 0, W, 26, PANEL);
  uint16_t c = alarmOn ? ALERT : ACC;
  lcd.drawCircle(15, 13, 7, c); lcd.fillCircle(15, 13, 3, c);
  put(28, 9, "CAZACÁMARAS MS", TXT, 1, PANEL);
  putRight(312, 9, SCREEN_NAMES[screen], c, 1, PANEL);
}
void drawFooter() {
  lcd.fillRect(0, 226, W, 14, BG);
  int x0 = 160 - (SCREENS * 14) / 2;
  for (int i = 0; i < SCREENS; ++i) {
    if (i == screen) lcd.fillRoundRect(x0 + i * 14, 230, 10, 5, 2, alarmOn ? ALERT : ACC);
    else lcd.fillCircle(x0 + i * 14 + 5, 232, 2, LINE);
  }
}
void clearBody() { lcd.fillRect(0, 26, W, 200, BG); }
void rssiBars(int x, int y, float rssi, uint16_t c) {
  int bars = rssi > -55 ? 4 : rssi > -70 ? 3 : rssi > -85 ? 2 : 1;
  for (int i = 0; i < 4; ++i) lcd.fillRect(x + i * 5, y + 9 - i * 3, 3, 3 + i * 3, i < bars ? c : LINE);
}
bool inside(const Btn& b, int x, int y) { return x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h; }
void flatButton(const Btn& b, const char* label, bool on) {
  uint16_t face = on ? SAFE : PANEL;
  lcd.fillRoundRect(b.x, b.y, b.w, b.h, 6, face);
  lcd.drawRoundRect(b.x, b.y, b.w, b.h, 6, on ? SAFE : LINE);
  putCenter(b.x + b.w / 2, b.y + b.h / 2 - 4, label, on ? INK : TXT, 1, face);
}

// ---------- Pantalla 1: ESCANEO ----------
void drawProgress() {
  lcd.fillRect(8, 193, 304, 31, BG);
  char s[56];
  if (scanning) snprintf(s, sizeof(s), "Buscando redes de cámara...");
  else if (scanFailed) snprintf(s, sizeof(s), "Falló redes · se reintentará");
  else if (tableFull) snprintf(s, sizeof(s), "Tabla llena · puede faltar alguna pista");
  else snprintf(s, sizeof(s), "Canal %u / 13 · solo escucha", channel);
  put(10, 194, s, scanFailed || tableFull ? WARN : DIM);
  for (int i = 0; i < 13; ++i) lcd.fillRoundRect(10 + i * 23, 211, 20, 5, 2,
      !scanning && i < channel ? ACC : LINE);
}
void drawWatch(uint32_t now) {
  clearBody();
  int n = countActive(now);
  uint16_t c = alarmOn ? ALERT : n ? WARN : SAFE;
  lcd.fillRoundRect(10, 34, 300, 96, 12, PANEL);
  lcd.fillRoundRect(10, 34, 300, 8, 4, c);
  char s[64];
  if (!wifiOK || !bleOK) {
    putCenter(160, 56, "Revisa el escaneo", WARN, 2, PANEL);
    putCenter(160, 87, "Fallo de radio: mira AJUSTES", TXT, 1, PANEL);
  } else if (alarmOn) {
    putCenter(160, 55, "¡Cámara probable!", ALERT, 3, PANEL);
    putCenter(160, 91, "Toca en LISTA y entra en BUSCAR", TXT, 1, PANEL);
  } else if (n) {
    snprintf(s, sizeof(s), n == 1 ? "%d posible" : "%d posibles", n);
    putCenter(160, 50, s, WARN, 3, PANEL);
    putCenter(160, 80, n == 1 ? "cámara" : "cámaras", WARN, 3, PANEL);
    putCenter(160, 111, "Son pistas: revisa la habitación", DIM, 1, PANEL);
  } else {
    putCenter(160, 58, "Nada sospechoso", SAFE, 3, PANEL);
    putCenter(160, 96, "Sigue revisando con la linterna", DIM, 1, PANEL);
  }
  const int values[] = {countWifi(now), countNetworks(now), countDevices(now)};
  const char* const labels[] = {"disp. WiFi", "redes cámara", "Bluetooth"};
  for (int i = 0; i < 3; ++i) {
    int x = 10 + i * 102;
    lcd.fillRoundRect(x, 140, 96, 46, 8, PANEL);
    snprintf(s, sizeof(s), "%d", values[i]);
    putCenter(x + 48, 147, s, TXT, 2, PANEL);
    putCenter(x + 48, 174, labels[i], DIM, 1, PANEL);
  }
  drawProgress();
}

// ---------- Pantalla 2: RADAR, dibujo de Vigía ----------
constexpr int RCX = 110, RCY = 126, RCR = 90;
void drawRadarFrame() {
  clearBody();
  put(210, 40, "Centro: tú", DIM);
  put(210, 57, "La señal cambia", DIM);
  put(210, 69, "con las paredes.", DIM);
  put(210, 88, "El ángulo es", DIM);
  put(210, 100, "decorativo.", DIM);
  lcd.fillCircle(217, 132, 4, ALERT); put(229, 128, "Probable", TXT);
  lcd.fillCircle(217, 154, 4, WARN); put(229, 150, "Posible", TXT);
  put(210, 180, "Sin metros ni", DIM);
  put(210, 192, "dirección real.", DIM);
}
void drawRadarDots(uint32_t now) {
  lcd.fillRect(8, 28, 198, 196, BG);
  for (int r = RCR; r > 0; r -= RCR / 3) lcd.drawCircle(RCX, RCY, r, LINE);
  lcd.drawFastHLine(RCX - RCR, RCY, RCR * 2, LINE);
  lcd.drawFastVLine(RCX, RCY - RCR, RCR * 2, LINE);
  float a = (sweep % 360) * DEG_TO_RAD;
  lcd.drawLine(RCX, RCY, RCX + cosf(a) * (RCR - 2), RCY + sinf(a) * (RCR - 2), SAFE);
  lcd.fillCircle(RCX, RCY, 4, ACC);
  for (int i = 0; i < MAX_TARGETS; ++i) {
    const Target& t = targets[i];
    if (!suspect(i, now)) continue;
    float d = constrain((-t.rssiF - 40) / 60.0f, 0.08f, 1.0f) * (RCR - 20);
    float ang = (t.id * 137 % 360) * DEG_TO_RAD;
    int x = RCX + cosf(ang) * d, y = RCY + sinf(ang) * d;
    lcd.fillCircle(x, y, 5, probable(i, now) ? ALERT : WARN);
    char s[12]; snprintf(s, sizeof(s), "#%lu", (unsigned long)t.id);
    put(constrain(x + 7, 10, 204 - textWidth(s, 1)), y - 3, s, TXT);
  }
}

// ---------- Pantalla 3: LISTA ----------
void drawList(uint32_t now) {
  clearBody();
  for (int r = 0; r < 6; ++r) { rowIndex[r] = -1; rowId[r] = 0; }
  int rows = 0;
  for (; rows < 6; ++rows) {
    int best = -1;
    for (int i = 0; i < MAX_TARGETS; ++i) {
      if (!candidate(i, now)) continue;
      bool used = false;
      for (int r = 0; r < rows; ++r) if (rowIndex[r] == i) used = true;
      if (!used && (best < 0 || priorityOf(i, now) > priorityOf(best, now))) best = i;
    }
    if (best < 0) break;
    rowIndex[rows] = best;
    const Target& t = targets[best]; rowId[rows] = t.id;
    int y = ROW_Y + rows * ROW_H;
    uint16_t c = t.ignored ? DIM : probable(best, now) ? ALERT : WARN;
    lcd.fillRoundRect(6, y, 308, ROW_H - 3, 6, PANEL);
    lcd.fillRoundRect(6, y, 5, ROW_H - 3, 3, c);
    char label[64], s[80];
    targetLabel(best, now, label, sizeof(label));
    snprintf(s, sizeof(s), "#%lu %s", (unsigned long)t.id, label);
    put(17, y + 4, s, TXT, 1, PANEL);
    snprintf(s, sizeof(s), "%s%s%s", CONF_NAME[confidenceOf(t, now)],
             hasVideo(t, now) ? " · vídeo" : "", t.ignored ? " · ignorado" : "");
    put(17, y + 15, s, c, 1, PANEL);
    rssiBars(285, y + 11, t.rssiF, c);
  }
  if (!rows) {
    putCenter(160, 95, "Nada que buscar", TXT, 2);
    putCenter(160, 127, "La lista se rellena con las pistas", DIM);
  } else put(8, 216, "Toca uno para buscarlo · hasta 6 pistas", DIM);
}

// ---------- Pantalla 4: BUSCAR ----------
bool freshSelected(uint32_t now) {
  return selected >= 0 && targets[selected].used && now - targets[selected].last < 6000;
}
void drawFindLive(uint32_t now) {
  if (selected < 0) return;
  const Target& t = targets[selected];
  bool fresh = freshSelected(now);
  int pct = fresh ? constrain((int)((t.rssiF + 100) * 100 / 60), 0, 100) : 0;
  uint16_t c = pct > 70 ? ALERT : pct > 40 ? WARN : SAFE;
  lcd.fillRect(0, 56, W, 36, BG);
  if (fresh) putCenter(160, 58, pct > 70 ? "¡CALIENTE!" : pct > 40 ? "TEMPLADO" : "FRÍO", c, 4);
  else putCenter(160, 65, "Sin señal reciente", DIM, 2);
  lcd.fillRoundRect(14, 103, 292, 24, 5, BG);
  if (pct) lcd.fillRoundRect(14, 103, max(6, 292 * pct / 100), 24, 5, c);
  char s[80];
  if (fresh) snprintf(s, sizeof(s), "Señal %d dBm · %s", (int)t.rssiF, proximity(t.rssiF));
  else snprintf(s, sizeof(s), "Muévete despacio y espera unos segundos");
  lcd.fillRect(0, 135, W, 14, BG);
  putCenter(160, 137, s, fresh ? TXT : DIM);
  if (selected < MAX_WIFI) {
    uint32_t frames, bytes; t.traffic.totals(now, frames, bytes);
    snprintf(s, sizeof(s), "Canal fijo %u · %lu tramas · %lu kB/10 s", t.channel,
             (unsigned long)frames, (unsigned long)(bytes / 1000));
  } else snprintf(s, sizeof(s), "Bluetooth · sin canal WiFi fijo");
  lcd.fillRect(0, 153, W, 13, BG); putCenter(160, 155, s, DIM);
}
void drawFind(uint32_t now) {
  clearBody();
  if (selected < 0) selected = worstTarget(now);
  if (selected < 0) {
    putCenter(160, 90, "Nada que buscar", TXT, 2);
    putCenter(160, 122, "Selecciona una pista en LISTA", DIM);
    return;
  }
  const Target& t = targets[selected];
  char label[64], s[88]; targetLabel(selected, now, label, sizeof(label));
  snprintf(s, sizeof(s), "#%lu %s", (unsigned long)t.id, label);
  put(12, 34, s, ACC);
  lcd.drawRoundRect(12, 101, 296, 28, 6, LINE);
  drawFindLive(now);
  putCenter(160, 173, "Más pitidos: más señal, sin medir metros", DIM);
  flatButton(B_MINE, t.ignored ? "Ignorado · volver a avisar" : "Es mío / ignorar", t.ignored);
}

// ---------- Pantalla 5: CONSEJOS ----------
void drawTips() {
  clearBody();
  put(10, 34, "Apaga la luz. Con la linterna del móvil,", TXT);
  put(10, 47, "busca reflejos de lentes pequeñas.", TXT);
  put(10, 69, "Mira detectores de humo, cargadores USB,", TXT);
  put(10, 82, "relojes, enchufes y espejos.", TXT);
  put(10, 104, "Solo ve cámaras que emiten por WiFi", WARN);
  put(10, 117, "o Bluetooth. No ve SD ni cámaras por cable.", WARN);
  put(10, 139, "Una cámara vecina también puede salir.", DIM);
  put(10, 152, "Usa BUSCAR: ¿crece la señal en tu cuarto?", DIM);
  put(10, 174, "Si encuentras una, no la toques. Haz foto", ALERT);
  put(10, 187, "y avisa a la plataforma y a la policía.", ALERT);
  putCenter(160, 208, "Si necesitas ayuda urgente: 112", WARN);
}

// ---------- Pantalla 6: AJUSTES ----------
void drawSettings() {
  clearBody();
  lcd.fillRoundRect(8, 32, 304, 72, 8, PANEL);
  char s[80];
  snprintf(s, sizeof(s), "WiFi: %s · Bluetooth: %s", wifiOK ? "escucha" : "FALLO", bleOK ? "escucha" : "FALLO");
  put(16, 40, s, wifiOK && bleOK ? TXT : ALERT, 1, PANEL);
  uint32_t wd, bd;
  portENTER_CRITICAL(&ringMux); wd = wifiDrops; bd = bleDrops; portEXIT_CRITICAL(&ringMux);
  snprintf(s, sizeof(s), "Cola llena: WiFi %lu · BLE %lu", (unsigned long)wd, (unsigned long)bd);
  put(16, 55, s, wd || bd ? WARN : DIM, 1, PANEL);
  put(16, 70, tableFull ? "Tabla llena: puede faltar alguna pista" : "Solo guarda ajustes; detecciones en RAM", tableFull ? WARN : DIM, 1, PANEL);
  put(16, 85, scanFailed ? "Falló el barrido de redes; se reintentará" : "No conecta ni pide datos a otros equipos", scanFailed ? WARN : DIM, 1, PANEL);
  snprintf(s, sizeof(s), "Sonido: %s", soundOn ? "SÍ" : "NO"); flatButton(B_SOUND, s, soundOn);
  snprintf(s, sizeof(s), "Brillo auto: %s", autoBright ? "SÍ" : "NO"); flatButton(B_BRIGHT, s, autoBright);
  flatButton(B_CAL, "Recalibrar táctil", false);
  putCenter(160, 192, prefsOK ? "Ajustes y calibración guardados en la placa" : "No se pueden guardar los ajustes", prefsOK ? DIM : WARN);
  putCenter(160, 209, "La señal no demuestra una cámara oculta", DIM);
}

// ---------- Dibujo y gestos de Vigía ----------
void drawScreen(uint32_t now) {
  drawHeader();
  switch (screen) {
    case 0: drawWatch(now); break;
    case 1: drawRadarFrame(); drawRadarDots(now); break;
    case 2: drawList(now); break;
    case 3: drawFind(now); break;
    case 4: drawTips(); break;
    case 5: drawSettings(); break;
  }
  drawFooter();
}
void slideTo(int next, int dir) {
  for (int i = 0; i < 8; ++i) {
    int x = dir > 0 ? W - (i + 1) * 40 : i * 40;
    lcd.fillRect(x, 26, 40, 200, PANEL); delay(7);
  }
  screen = (next + SCREENS) % SCREENS;
  drawScreen(millis());
}
void onTap(int x, int y) {
  uint32_t now = millis();
  if (screen == 2) {
    int r = (y - ROW_Y) / ROW_H;
    if (y >= ROW_Y && r >= 0 && r < 6 && rowIndex[r] >= 0 && targets[rowIndex[r]].id == rowId[r]) {
      selected = rowIndex[r]; nextBeep = now; slideTo(3, 1);
    }
  } else if (screen == 3 && selected >= 0 && inside(B_MINE, x, y)) {
    targets[selected].ignored = !targets[selected].ignored;
    drawScreen(now);
  } else if (screen == 5) {
    if (inside(B_SOUND, x, y)) {
      soundOn = !soundOn;
      if (prefsOK) prefs.putBool("sound", soundOn);
      if (!soundOn) noTone(PIN_SPK); else beep(2200, 50);
    } else if (inside(B_BRIGHT, x, y)) {
      autoBright = !autoBright;
      if (prefsOK) prefs.putBool("auto", autoBright);
      updateBrightness();
    } else if (inside(B_CAL, x, y)) {
      pauseRadios(); noTone(PIN_SPK); calibrate();
      paused = false; resumeSniffer();
    } else return;
    drawScreen(millis());
  }
}
void handleTouch() {
  int rx, ry;
  uint32_t t = millis();
  if (sampleRaw(rx, ry)) {
    int x, y; toScreen(rx, ry, x, y);
    if (!touching) { touching = true; downX = x; downY = y; }
    lastX = x; lastY = y; upAt = t; return;
  }
  if (touching && t - upAt > 60) {
    touching = false;
    int dx = lastX - downX, dy = lastY - downY;
    if (abs(dx) > 50 && abs(dx) > abs(dy)) slideTo(screen + (dx < 0 ? 1 : -1), dx < 0 ? 1 : -1);
    else if (abs(dx) < 15 && abs(dy) < 15) onTap(downX, downY);
  }
}
uint32_t viewSignature(uint32_t now) {
  uint32_t h = 2166136261u;
  auto mix = [&](uint32_t v) { h = (h ^ v) * 16777619u; };
  mix(screen); mix(alarmOn); mix(wifiOK); mix(bleOK); mix(scanFailed); mix(tableFull);
  if (screen == 5) {
    portENTER_CRITICAL(&ringMux); mix(wifiDrops); mix(bleDrops); portEXIT_CRITICAL(&ringMux);
  }
  mix(countWifi(now)); mix(countNetworks(now)); mix(countDevices(now));
  for (int i = 0; i < MAX_TARGETS; ++i) {
    const Target& t = targets[i];
    if (!candidate(i, now)) continue;
    mix(t.id); mix(t.ignored); mix(confidenceOf(t, now)); mix(hasVideo(t, now));
    mix((int)t.rssiF / 8); mix(hasNetwork(t, now));
  }
  return h;
}

// ---------- Programa ----------
void setup() {
  Serial.begin(115200);
  delay(200);
  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_B, OUTPUT);
  setLed(false, false, false);
  pinMode(5, OUTPUT); digitalWrite(5, HIGH);  // microSD sin usar
  ledcAttach(PIN_BL, 5000, 8); ledcWrite(PIN_BL, 230);
  analogSetPinAttenuation(PIN_LDR, ADC_0db);
  lcdBus.begin(14, 12, 13, 15); lcd.begin(40000000); lcd.setRotation(1); lcd.cp437(true);
  touchBus.begin(25, 39, 32, 33); touch.begin(touchBus); touch.setRotation(1);
  prefsOK = prefs.begin("cazacam", false);
  if (prefsOK) { soundOn = prefs.getBool("sound", true); autoBright = prefs.getBool("auto", true); }
  if (prefsOK && prefs.getBool("cal", false)) {
    ax = prefs.getFloat("ax"); bx = prefs.getFloat("bx"); cx = prefs.getFloat("cx");
    ay = prefs.getFloat("ay"); by = prefs.getFloat("by"); cy = prefs.getFloat("cy");
    if (!isfinite(ax) || !isfinite(bx) || !isfinite(cx) || !isfinite(ay) || !isfinite(by) || !isfinite(cy)
        || fabsf(ax * by - bx * ay) < 0.00001f) calibrate();
  } else calibrate();
  updateBrightness(); startRadios(); drawScreen(millis());
}
void loop() {
  drainWifi();
  Adv a;
  int budget = 64;
  if (advQueue) while (budget-- && xQueueReceive(advQueue, &a, 0) == pdTRUE) ingestBle(a);
  reportNew(millis());
  handleTouch();
  uint32_t now = millis();
  radioTick(now);
  now = millis();
  alarmOn = false;
  for (int i = 0; i < MAX_TARGETS; ++i) if (probable(i, now)) alarmOn = true;
  reportNew(now);
  bool pulse = now % 2000 < 80;
  if (!wifiOK || !bleOK) setLed((now / 500) % 2, (now / 500) % 2, false);
  else if (alarmOn) setLed((now / 300) % 2, false, false);
  else { bool any = countActive(now) > 0; setLed(false, pulse && !any, pulse && any); }
  if (screen == 3 && freshSelected(now) && !targets[selected].ignored && (int32_t)(now - nextBeep) >= 0) {
    float r = targets[selected].rssiF;
    int gap = constrain((int)map((long)r, -100, -40, 1400, 90), 90, 1400);
    beep(1900, 18); nextBeep = now + gap;
  } else if (screen != 3 && alarmOn && (int32_t)(now - nextBeep) >= 0) {
    beep(2600, 120); nextBeep = now + 4000;
  }
  uint32_t period = screen == 1 ? 120 : 400;
  if (!touching && now - lastDraw > period) {
    lastDraw = now;
    static uint32_t lastSig = 0;
    uint32_t sig = viewSignature(now);
    if (screen == 1) { sweep += 12; drawRadarDots(now); if (sig != lastSig) drawHeader(); }
    else if (sig != lastSig) drawScreen(now);
    else if (screen == 3) drawFindLive(now);
    else if (screen == 0) drawProgress();
    lastSig = sig;
  }
  if (now - lastBright > 1500) { lastBright = now; updateBrightness(); }
  delay(4);
}
