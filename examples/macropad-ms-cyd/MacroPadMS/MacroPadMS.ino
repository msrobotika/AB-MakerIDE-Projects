/*
  MacroPad MS · AB MakerIDE / MS Robotika
  Panel de atajos tactil para la CYD ESP32-2432S028R (ILI9341 320x240 en horizontal,
  tactil XPT2046). Dos formas de conectarlo, sin Wi-Fi:
    - Bluetooth: se empareja como teclado (BLE HID) y no hace falta instalar nada.
    - USB: para PCs sin Bluetooth. El programa agent/macropad_agent.py lee el cable
      USB y pulsa las teclas en Windows.

  Cuatro paginas de seis botones que se cambian deslizando, mas AJUSTES:
    MULTIMEDIA  anterior, play/pausa, siguiente, volumen -, silencio, volumen +
    REUNIONES   micro y camara de Teams, levantar la mano, micro y camara de Zoom, micro de Meet
    TRABAJO     copiar, pegar, deshacer, recorte de pantalla, escritorio, bloquear el PC
    DIRECTO     F13 a F18: asignalas en OBS o en tu programa (no chocan con nada)
  El LED RGB parpadea en azul mientras espera al PC y destella en verde al enviar.

  Librerias: Adafruit GFX, Adafruit ILI9341, XPT2046_Touchscreen (Paul Stoffregen),
  NimBLE-Arduino 2.x (h2zero). Placa: "ESP32 Dev Module" (esp32 3.x).
  Referencia de pines: github.com/witnessmenow/ESP32-Cheap-Yellow-Display/PINS.md
  Licencia MIT, como el resto de AB MakerIDE.
*/
#include <Arduino.h>
#include <SPI.h>
#include <Preferences.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
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
constexpr uint16_t CYAN = 0x05FF, ORANGE = 0xFC60, GREEN = 0x2E8A, MAGENTA = 0xE8DC, RED = 0xF8A6;

// ---------- Acciones ----------
enum Kind : uint8_t { NONE, KEY, MEDIA };
constexpr uint8_t M_CTRL = 0x01, M_SHIFT = 0x02, M_ALT = 0x04, M_GUI = 0x08;
// Uso de consumo (Consumer Page 0x0C)
constexpr uint16_t C_PLAY = 0xCD, C_NEXT = 0xB5, C_PREV = 0xB6, C_MUTE = 0xE2, C_VOLUP = 0xE9, C_VOLDN = 0xEA;

enum Icon : uint8_t { I_NONE, I_PREV, I_PLAY, I_NEXT, I_VOLDN, I_MUTE, I_VOLUP };

struct Action {
  const char* label;
  const char* hint;
  Kind kind;
  uint8_t mod;
  uint16_t code;   // tecla HID (KEY) o uso de consumo (MEDIA)
  Icon icon;
  bool repeat;     // se repite mientras se mantiene pulsado
};

struct Btn { int x, y, w, h; };  // aqui arriba para los prototipos automaticos de Arduino

struct Page {
  const char* name;
  uint16_t color;
  Action a[6];
};

const Page PAGES[] = {
  {"MULTIMEDIA", CYAN, {
    {"Anterior", "", MEDIA, 0, C_PREV, I_PREV, false},
    {"Play/Pausa", "", MEDIA, 0, C_PLAY, I_PLAY, false},
    {"Siguiente", "", MEDIA, 0, C_NEXT, I_NEXT, false},
    {"Volumen -", "", MEDIA, 0, C_VOLDN, I_VOLDN, true},
    {"Silencio", "", MEDIA, 0, C_MUTE, I_MUTE, false},
    {"Volumen +", "", MEDIA, 0, C_VOLUP, I_VOLUP, true}}},
  {"REUNIONES", ORANGE, {
    {"Micro Teams", "Ctrl+Mayús+M", KEY, M_CTRL | M_SHIFT, 0x10, I_NONE, false},
    {"Cámara Teams", "Ctrl+Mayús+O", KEY, M_CTRL | M_SHIFT, 0x12, I_NONE, false},
    {"Mano Teams", "Ctrl+Mayús+K", KEY, M_CTRL | M_SHIFT, 0x0E, I_NONE, false},
    {"Micro Zoom", "Alt+A", KEY, M_ALT, 0x04, I_NONE, false},
    {"Cámara Zoom", "Alt+V", KEY, M_ALT, 0x19, I_NONE, false},
    {"Micro Meet", "Ctrl+D", KEY, M_CTRL, 0x07, I_NONE, false}}},
  {"TRABAJO", GREEN, {
    {"Copiar", "Ctrl+C", KEY, M_CTRL, 0x06, I_NONE, false},
    {"Pegar", "Ctrl+V", KEY, M_CTRL, 0x19, I_NONE, false},
    {"Deshacer", "Ctrl+Z", KEY, M_CTRL, 0x1D, I_NONE, true},
    {"Recorte", "Win+Mayús+S", KEY, M_GUI | M_SHIFT, 0x16, I_NONE, false},
    {"Escritorio", "Win+D", KEY, M_GUI, 0x07, I_NONE, false},
    {"Bloquear PC", "Win+L", KEY, M_GUI, 0x0F, I_NONE, false}}},
  {"DIRECTO", MAGENTA, {
    {"Escena 1", "F13", KEY, 0, 0x68, I_NONE, false},
    {"Escena 2", "F14", KEY, 0, 0x69, I_NONE, false},
    {"Escena 3", "F15", KEY, 0, 0x6A, I_NONE, false},
    {"Grabar", "F16", KEY, 0, 0x6B, I_NONE, false},
    {"Directo", "F17", KEY, 0, 0x6C, I_NONE, false},
    {"Marcador", "F18", KEY, 0, 0x6D, I_NONE, false}}},
};
constexpr int ACTION_PAGES = sizeof(PAGES) / sizeof(PAGES[0]);
constexpr int PAGE_COUNT = ACTION_PAGES + 1;  // + AJUSTES
const char* const SETTINGS_NAME = "AJUSTES";

// ---------- HID por Bluetooth ----------
// Report 1: teclado (modificadores, reservado, 6 teclas). Report 2: consumo (16 bits).
const uint8_t REPORT_MAP[] = {
  0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x85, 0x01,
  0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02,
  0x95, 0x01, 0x75, 0x08, 0x81, 0x01,
  0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x25, 0x73, 0x05, 0x07, 0x19, 0x00, 0x29, 0x73, 0x81, 0x00,
  0xC0,
  0x05, 0x0C, 0x09, 0x01, 0xA1, 0x01, 0x85, 0x02,
  0x15, 0x00, 0x26, 0xFF, 0x03, 0x19, 0x00, 0x2A, 0xFF, 0x03, 0x75, 0x10, 0x95, 0x01, 0x81, 0x00,
  0xC0,
};

// El nucleo esp32 3.x libera la memoria de Bluetooth al arrancar si cree que nadie la usa.
// NimBLE-Arduino no se lo indica, asi que lo declaramos aqui; sin esto la placa se reinicia.
extern "C" bool btInUse(void) { return true; }

NimBLEServer* server = nullptr;
NimBLEHIDDevice* hid = nullptr;
NimBLECharacteristic* kbInput = nullptr;
NimBLECharacteristic* mediaInput = nullptr;
volatile bool connected = false;
volatile bool statusDirty = true;

class ServerEvents : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*, NimBLEConnInfo&) override {
    connected = true;
    statusDirty = true;
    Serial.println("MSR_MACROPAD_CONNECTED");
  }
  void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int reason) override {
    connected = false;
    statusDirty = true;
    Serial.printf("MSR_MACROPAD_DISCONNECTED reason=%d\n", reason);
    NimBLEDevice::startAdvertising();
  }
  void onAuthenticationComplete(NimBLEConnInfo& info) override {
    Serial.printf("MSR_MACROPAD_PAIRED encrypted=%d bonded=%d\n", info.isEncrypted(), info.isBonded());
  }
};

bool startBle() {
  if (!NimBLEDevice::init("MacroPad MS")) {
    Serial.println("MSR_MACROPAD_BLE_INIT_FAILED");
    return false;
  }
  NimBLEDevice::setSecurityAuth(true, false, true);  // emparejado guardado, sin PIN, conexion segura
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerEvents());
  server->advertiseOnDisconnect(true);
  hid = new NimBLEHIDDevice(server);
  kbInput = hid->getInputReport(1);
  mediaInput = hid->getInputReport(2);
  hid->setManufacturer("MS Robotika");
  hid->setPnp(0x02, 0xE502, 0xA111, 0x0100);
  hid->setHidInfo(0x00, 0x01);
  hid->setReportMap((uint8_t*)REPORT_MAP, sizeof(REPORT_MAP));
  hid->setBatteryLevel(100);
  hid->startServices();
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->setAppearance(HID_KEYBOARD);
  adv->addServiceUUID(hid->getHidService()->getUUID());
  adv->setName("MacroPad MS");
  adv->enableScanResponse(true);
  adv->start();
  Serial.println("MSR_MACROPAD_ADVERTISING name=\"MacroPad MS\"");
  return true;
}

// ---------- Modo USB ----------
// El programa del PC manda "MSR_HELLO" cada 2 s; si lo hemos oido hace menos de 5 s,
// los atajos salen por el cable como una linea de texto que el programa convierte en teclas.
uint32_t lastHello = 0;
bool usbAgent() { return lastHello && millis() - lastHello < 5000; }

void pollUsb() {
  static char buf[32];
  static uint8_t len = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      buf[len] = 0;
      if (len && strcmp(buf, "MSR_HELLO") == 0) {
        if (!usbAgent()) statusDirty = true;
        lastHello = millis();
        Serial.println("MSR_MACROPAD_READY");
      }
      len = 0;
    } else if (len < sizeof(buf) - 1) {
      buf[len++] = c;
    } else {
      len = 0;  // linea demasiado larga: se descarta
    }
  }
}

bool sendAction(const Action& act) {
  if (!connected) {
    if (!usbAgent()) return false;
    Serial.printf("MSR_KEY kind=%s mod=%u code=%u\n", act.kind == MEDIA ? "media" : "key", act.mod, act.code);
    return true;
  }
  if (act.kind == KEY) {
    uint8_t down[8] = {act.mod, 0, (uint8_t)act.code, 0, 0, 0, 0, 0};
    uint8_t up[8] = {0};
    kbInput->notify(down, sizeof(down));
    delay(15);
    kbInput->notify(up, sizeof(up));
  } else if (act.kind == MEDIA) {
    uint8_t down[2] = {(uint8_t)(act.code & 0xFF), (uint8_t)(act.code >> 8)};
    uint8_t up[2] = {0, 0};
    mediaInput->notify(down, sizeof(down));
    delay(15);
    mediaInput->notify(up, sizeof(up));
  }
  return true;
}

// ---------- Estado ----------
Preferences prefs;
int page = 0;
bool autoBright = true, clickOn = true;
float ax = 0, bx = 0, cx = 0, ay = 0, by = 0, cy = 0;
uint32_t ledOffAt = 0, lastBlink = 0, lastBright = 0;
uint32_t sentCount = 0;

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

// ---------- LED, brillo y clic ----------
void setLed(bool r, bool g, bool b) {
  digitalWrite(LED_R, r ? LOW : HIGH);
  digitalWrite(LED_G, g ? LOW : HIGH);
  digitalWrite(LED_B, b ? LOW : HIGH);
}

void flashLed(bool r, bool g, bool b, uint32_t ms) {
  setLed(r, g, b);
  ledOffAt = millis() + ms;
}

void updateBrightness() {
  int level = 230;
  if (autoBright) {
    int v = analogRead(PIN_LDR);  // con 0 dB: luz fuerte ~0, oscuridad sube
    level = constrain(map(v, 0, 450, 255, 35), 35, 255);
  }
  ledcWrite(PIN_BL, level);
}

void click() {
  if (clickOn) tone(PIN_SPK, 2600, 12);
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
      putCenter(160, 100, "CALIBRAR TÁCTIL", CYAN, 2);
      putCenter(160, 128, "Toca el centro de la cruz", TXT);
      lcd.drawFastHLine(sx[i] - 10, sy[i], 21, ORANGE);
      lcd.drawFastVLine(sx[i], sy[i] - 10, 21, ORANGE);
      lcd.drawCircle(sx[i], sy[i], 8, ORANGE);
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
      putCenter(160, 160, "Repite las tres cruces", RED);
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
    Serial.println("MSR_MACROPAD_CALIBRATION_SAVED");
    return;
  }
}

// ---------- Iconos multimedia ----------
void drawIcon(Icon ic, int x, int y, uint16_t c) {
  switch (ic) {
    case I_PLAY:
      lcd.fillTriangle(x - 12, y - 13, x - 12, y + 13, x + 6, y, c);
      lcd.fillRect(x + 9, y - 12, 4, 24, c);
      lcd.fillRect(x + 16, y - 12, 4, 24, c);
      break;
    case I_NEXT:
      lcd.fillTriangle(x - 16, y - 12, x - 16, y + 12, x, y, c);
      lcd.fillTriangle(x - 2, y - 12, x - 2, y + 12, x + 14, y, c);
      lcd.fillRect(x + 14, y - 12, 4, 24, c);
      break;
    case I_PREV:
      lcd.fillTriangle(x + 16, y - 12, x + 16, y + 12, x, y, c);
      lcd.fillTriangle(x + 2, y - 12, x + 2, y + 12, x - 14, y, c);
      lcd.fillRect(x - 18, y - 12, 4, 24, c);
      break;
    case I_VOLDN: case I_MUTE: case I_VOLUP:
      lcd.fillRect(x - 18, y - 6, 8, 12, c);
      lcd.fillTriangle(x - 10, y - 6, x - 10, y + 6, x + 2, y - 14, c);
      lcd.fillTriangle(x - 10, y + 6, x + 2, y + 14, x + 2, y - 14, c);
      if (ic == I_MUTE) {
        for (int k = -1; k <= 1; k++) {
          lcd.drawLine(x + 7 + k, y - 8, x + 19 + k, y + 8, c);
          lcd.drawLine(x + 19 + k, y - 8, x + 7 + k, y + 8, c);
        }
      } else {
        lcd.fillRect(x + 8, y - 2, 12, 4, c);
        if (ic == I_VOLUP) lcd.fillRect(x + 12, y - 6, 4, 12, c);
      }
      break;
    default:
      break;
  }
}

// ---------- Rejilla de botones ----------
constexpr int GX = 8, GY = 32, BW = 98, BH = 88, GAP = 7;

void cellRect(int i, int& x, int& y) {
  x = GX + (i % 3) * (BW + GAP);
  y = GY + (i / 3) * (BH + GAP);
}

int cellAt(int x, int y) {
  for (int i = 0; i < 6; i++) {
    int bx0, by0;
    cellRect(i, bx0, by0);
    if (x >= bx0 && x < bx0 + BW && y >= by0 && y < by0 + BH) return i;
  }
  return -1;
}

void drawButton(int i, bool pressed) {
  const Page& pg = PAGES[page];
  const Action& a = pg.a[i];
  int x, y;
  cellRect(i, x, y);
  uint16_t face = pressed ? pg.color : PANEL;
  uint16_t fg = pressed ? INK : TXT;
  lcd.fillRoundRect(x, y, BW, BH, 10, face);
  lcd.drawRoundRect(x, y, BW, BH, 10, pressed ? TXT : pg.color);
  lcd.fillRoundRect(x + 10, y + 6, BW - 20, 3, 1, pressed ? INK : pg.color);
  if (a.icon != I_NONE) {
    drawIcon(a.icon, x + BW / 2, y + 38, pressed ? INK : pg.color);
    putCenter(x + BW / 2, y + 68, a.label, fg, 1, face);
  } else {
    // Etiqueta en dos lineas si tiene espacio: "Micro Teams" -> "Micro" / "Teams"
    const char* sp = strchr(a.label, ' ');
    if (sp && textWidth(a.label, 2) > BW - 8) {
      char first[16];
      size_t n = min((size_t)(sp - a.label), sizeof(first) - 1);
      memcpy(first, a.label, n);
      first[n] = 0;
      putCenter(x + BW / 2, y + 20, first, fg, 2, face);
      putCenter(x + BW / 2, y + 40, sp + 1, fg, 2, face);
    } else {
      putCenter(x + BW / 2, y + 30, a.label, fg, 2, face);
    }
    putCenter(x + BW / 2, y + 68, a.hint, pressed ? INK : DIM, 1, face);
  }
}

// ---------- Marco comun ----------
const char* pageName() { return page < ACTION_PAGES ? PAGES[page].name : SETTINGS_NAME; }
uint16_t pageColor() { return page < ACTION_PAGES ? PAGES[page].color : DIM; }

void btIcon(int x, int y, uint16_t c) {
  lcd.drawLine(x, y + 4, x + 8, y + 12, c);
  lcd.drawLine(x, y + 12, x + 8, y + 4, c);
  lcd.drawLine(x + 4, y, x + 4, y + 16, c);
  lcd.drawLine(x + 4, y, x + 8, y + 4, c);
  lcd.drawLine(x + 4, y + 16, x + 8, y + 12, c);
}

void drawHeader() {
  lcd.fillRect(0, 0, W, 26, PANEL);
  lcd.fillRoundRect(8, 6, 14, 14, 3, pageColor());
  for (int r = 0; r < 2; r++)
    for (int c = 0; c < 2; c++) lcd.fillRect(10 + c * 6, 8 + r * 6, 4, 4, BG);
  put(28, 9, "MACROPAD MS", TXT, 1, PANEL);
  put(100, 9, pageName(), pageColor(), 1, PANEL);
  btIcon(298, 5, connected ? CYAN : LINE);
  const char* st = connected ? "PC Bluetooth" : (usbAgent() ? "PC por USB" : "Esperando PC");
  putRight(292, 9, st, (connected || usbAgent()) ? CYAN : ORANGE, 1, PANEL);
}

void drawFooter() {
  lcd.fillRect(0, 226, W, 14, BG);
  int x0 = 160 - (PAGE_COUNT * 14) / 2;
  for (int i = 0; i < PAGE_COUNT; i++) {
    uint16_t c = i < ACTION_PAGES ? PAGES[i].color : DIM;
    if (i == page) lcd.fillRoundRect(x0 + i * 14, 230, 10, 5, 2, c);
    else lcd.fillCircle(x0 + i * 14 + 5, 232, 2, LINE);
  }
}

// ---------- AJUSTES ----------
const Btn B_CLICK = {8, 120, 148, 30}, B_BRIGHT = {164, 120, 148, 30};
const Btn B_CAL = {8, 158, 148, 30}, B_FORGET = {164, 158, 148, 30};

bool inside(const Btn& b, int x, int y) { return x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h; }

void flatButton(const Btn& b, const char* label, bool on, uint16_t edge) {
  uint16_t face = on ? GREEN : PANEL;
  lcd.fillRoundRect(b.x, b.y, b.w, b.h, 6, face);
  lcd.drawRoundRect(b.x, b.y, b.w, b.h, 6, on ? GREEN : edge);
  putCenter(b.x + b.w / 2, b.y + b.h / 2 - 4, label, on ? INK : TXT, 1, face);
}

void drawSettings() {
  lcd.fillRect(0, 26, W, 200, BG);
  lcd.fillRoundRect(8, 32, 304, 80, 8, PANEL);
  char line[48];
  put(16, 40, connected ? "Bluetooth: PC conectado" : "Bluetooth: esperando al PC", connected ? CYAN : ORANGE, 1, PANEL);
  putRight(304, 40, usbAgent() ? "USB: programa activo" : "USB: sin programa", usbAgent() ? CYAN : DIM, 1, PANEL);
  put(16, 54, "Nombre en el PC: MacroPad MS", TXT, 1, PANEL);
  snprintf(line, sizeof(line), "Equipos emparejados: %d", NimBLEDevice::getNumBonds());
  put(16, 68, line, DIM, 1, PANEL);
  snprintf(line, sizeof(line), "Atajos enviados desde el arranque: %lu", (unsigned long)sentCount);
  put(16, 82, line, DIM, 1, PANEL);
  put(16, 96, "Windows: Configuración > Bluetooth > Agregar", DIM, 1, PANEL);
  char s[32];
  snprintf(s, sizeof(s), "Sonido clic: %s", clickOn ? "SÍ" : "NO");
  flatButton(B_CLICK, s, clickOn, LINE);
  snprintf(s, sizeof(s), "Brillo auto: %s", autoBright ? "SÍ" : "NO");
  flatButton(B_BRIGHT, s, autoBright, LINE);
  flatButton(B_CAL, "Recalibrar táctil", false, LINE);
  flatButton(B_FORGET, "Olvidar PCs (2 s)", false, RED);
  putCenter(160, 200, "F13-F18 de DIRECTO: asígnalas en OBS", DIM);
  putCenter(160, 212, "(Ajustes > Atajos de teclado)", DIM);
}

void drawPage() {
  drawHeader();
  if (page < ACTION_PAGES) {
    lcd.fillRect(0, 26, W, 200, BG);
    for (int i = 0; i < 6; i++) drawButton(i, false);
  } else {
    drawSettings();
  }
  drawFooter();
}

// Transicion: barrido de la franja del color de la pagina en el sentido del gesto.
void slideTo(int next, int dir) {
  int np = (next + PAGE_COUNT) % PAGE_COUNT;
  uint16_t c = np < ACTION_PAGES ? PAGES[np].color : DIM;
  for (int i = 0; i < 8; i++) {
    int x = dir > 0 ? W - (i + 1) * 40 : i * 40;
    lcd.fillRect(x, 26, 40, 200, c);
    delay(7);
  }
  page = (next + PAGE_COUNT) % PAGE_COUNT;
  drawPage();
  Serial.printf("MSR_MACROPAD_PAGE %s\n", pageName());
}

// ---------- Gestos ----------
bool touching = false, longFired = false;
int downX = 0, downY = 0, lastX = 0, lastY = 0, downCell = -1;
uint32_t downAt = 0, upAt = 0, nextRepeat = 0;

void fire(int cell) {
  const Action& a = PAGES[page].a[cell];
  if (sendAction(a)) {
    sentCount++;
    click();
    flashLed(false, true, false, 90);
    Serial.printf("MSR_MACROPAD_SENT page=%s action=\"%s\"\n", PAGES[page].name, a.label);
  } else {
    flashLed(true, false, false, 200);
    lcd.fillRect(0, 226, W, 14, BG);
    putCenter(160, 229, "Empareja por Bluetooth o abre el programa USB", ORANGE);
    statusDirty = true;
  }
}

void onSettingsTap(int x, int y) {
  if (inside(B_CLICK, x, y)) { clickOn = !clickOn; prefs.putBool("click", clickOn); click(); }
  else if (inside(B_BRIGHT, x, y)) { autoBright = !autoBright; prefs.putBool("auto", autoBright); updateBrightness(); }
  else if (inside(B_CAL, x, y)) { calibrate(); }
  else return;
  drawPage();
}

void handleTouch() {
  int rx, ry;
  uint32_t t = millis();
  if (sampleRaw(rx, ry)) {
    int x, y;
    toScreen(rx, ry, x, y);
    if (!touching) {
      touching = true;
      longFired = false;
      downX = x; downY = y; downAt = t;
      downCell = page < ACTION_PAGES ? cellAt(x, y) : -1;
      if (downCell >= 0) drawButton(downCell, true);
      nextRepeat = t + 450;
    }
    lastX = x; lastY = y; upAt = t;
    bool still = abs(x - downX) < 25 && abs(y - downY) < 25;
    // Repeticion para volumen y deshacer mientras se mantiene pulsado
    if (downCell >= 0 && still && PAGES[page].a[downCell].repeat && t >= nextRepeat) {
      fire(downCell);
      longFired = true;
      nextRepeat = t + 140;
    }
    if (page == ACTION_PAGES && !longFired && still && t - downAt > 2000 && inside(B_FORGET, downX, downY)) {
      longFired = true;
      NimBLEDevice::deleteAllBonds();
      Serial.println("MSR_MACROPAD_BONDS_CLEARED");
      if (connected) server->disconnect(server->getPeerInfo(0).getConnHandle());
      drawPage();
    }
    return;
  }
  if (touching && t - upAt > 60) {
    touching = false;
    int dx = lastX - downX, dy = lastY - downY;
    bool swipe = abs(dx) > 50 && abs(dx) > abs(dy);
    if (downCell >= 0) {
      drawButton(downCell, false);
      if (!swipe && !longFired && abs(dx) < 25 && abs(dy) < 25) fire(downCell);
    }
    if (swipe) slideTo(page + (dx < 0 ? 1 : -1), dx < 0 ? 1 : -1);
    else if (page == ACTION_PAGES && !longFired && abs(dx) < 15 && abs(dy) < 15) onSettingsTap(downX, downY);
  }
}

// ---------- Programa ----------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("MSR_MACROPAD_BOOT profile=ESP32-2432S028R display=ILI9341 touch=XPT2046 link=BLE-HID");
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

  prefs.begin("macropad", false);
  clickOn = prefs.getBool("click", true);
  autoBright = prefs.getBool("auto", true);
  if (prefs.getBool("cal", false)) {
    ax = prefs.getFloat("ax"); bx = prefs.getFloat("bx"); cx = prefs.getFloat("cx");
    ay = prefs.getFloat("ay"); by = prefs.getFloat("by"); cy = prefs.getFloat("cy");
  } else {
    calibrate();
  }
  updateBrightness();
  if (!startBle()) {
    lcd.fillScreen(BG);
    putCenter(160, 100, "No arranca el Bluetooth", RED, 2);
    putCenter(160, 130, "Revisa el núcleo esp32 y NimBLE-Arduino", TXT);
    while (true) delay(1000);
  }
  drawPage();
}

void loop() {
  pollUsb();
  handleTouch();
  uint32_t ms = millis();
  static bool lastUsb = false;
  if (usbAgent() != lastUsb) { lastUsb = usbAgent(); statusDirty = true; }
  bool linked = connected || lastUsb;
  if (statusDirty && !touching) {
    statusDirty = false;
    drawHeader();
    if (page == ACTION_PAGES) drawSettings();
    drawFooter();
  }
  if (ledOffAt && ms >= ledOffAt) { ledOffAt = 0; setLed(false, false, false); }
  if (!linked && !ledOffAt && ms - lastBlink > 700) {  // parpadeo azul mientras espera al PC
    lastBlink = ms;
    static bool on = false;
    on = !on;
    setLed(false, false, on);
  }
  if (linked && !ledOffAt && ms - lastBlink > 700) { lastBlink = ms; setLed(false, false, false); }
  if (ms - lastBright > 1500) { lastBright = ms; updateBrightness(); }
  delay(4);
}
